# 医院挂号系统 - 改进建议清单

> 分析日期：2026-06-17  
> 覆盖范围：C++ 后端、MySQL 持久层、Vue3 前端、工程配置  
> **最后更新**：2026-06-17 P0 阶段修复完成

## 总体印象

项目整体架构已经成型，采用了分层设计：

- **DI 容器**（`src/di/Container.h`）解耦模块依赖
- **Repository 接口抽象**（`include/repository/`）支持数据库切换
- **Service 层**（`src/service/AppointmentService.cpp`）承载业务逻辑
- **Controller 层**（`src/api/ApiController.cpp`）处理 HTTP API
- **Vue3 + Vite + TypeScript 前端**（`web/`）实现三端分离

但在**安全、并发一致性、类型严谨性和工程规范**上仍有较多可提升空间。以下按优先级分类列出。

---

## 高优先级：安全与稳定性

### 1. `MySQLAppointmentRepository` 存在 SQL 注入风险 ✅ 已修复

**位置**：`src/repository/mysql/MySQLAppointmentRepository.cpp`

该文件中大量直接拼接 SQL 字符串，且未调用 `escape()` 进行转义：

- `findById` / `findByDoctor` / `findByPatient` / `remove`：直接拼接 `int64_t`
- `save` / `update`：把枚举字符串直接拼进 SQL

虽然当前 `patient_id`、`doctor_id` 是整数类型，但 `status`、`priority` 等枚举字符串如果未来被外部输入影响，就会产生注入风险。相比之下，`MySQLPatientRepository` 和 `MySQLDoctorRepository` 已经实现了 `escape()` 转义。

**建议**：
- 统一使用 MySQL 预处理语句（`mysql_stmt_prepare` / `mysql_stmt_bind_param`）
- 或者至少对 `status` / `priority` 做白名单校验，禁止字符串拼接

**修复状态**：✅ 已完成（2026-06-17）
- 所有方法已改为预处理语句
- 涉及方法：`findById`、`save`、`update`、`remove`、`findByDoctor`、`findByPatient`、`countWaitingByDoctor`

### 2. 业务逻辑不是真正的事务

**位置**：`src/service/AppointmentService.cpp` 中的 `makeAppointmentInternal`

当前流程：

1. `appointmentRepo_->save(apt)`
2. `doctorRepo_->update(*doctor)`
3. 如果第 2 步失败，调用 `appointmentRepo_->remove(apt.id)` 回滚

这不是数据库事务，中间进程崩溃或并发冲突时，容易出现“医生名额被占但无挂号记录”或“有挂号记录但医生名额未扣”的数据不一致。

**建议**：
- 在 `MySQLConnectionPool` 之上增加“事务连接”支持
- Service 层在同一个 `MYSQL` 连接上执行 `BEGIN / COMMIT / ROLLBACK`
- 保证 `save` 和 `update` 原子化

### 3. 全局单锁 `serviceMutex_` 造成并发瓶颈

**位置**：`src/service/AppointmentService.h` / `AppointmentService.cpp`

`AppointmentService` 使用一把大锁保护所有写操作，导致所有挂号、取消、叫号串行执行。随着科室和医生数量增加，会成为明显性能瓶颈。

**建议**：
- 对医生维度进行细粒度加锁（`std::mutex per doctor_id`）
- 或者将并发控制下沉到数据库层，利用 `SELECT ... FOR UPDATE` + 事务实现抢号

### 4. 内存队列与数据库状态不一致 ✅ 已修复

**位置**：`src/service/AppointmentService.cpp` 中的 `doctorQueues_`

`callNextPatient` 使用进程内内存优先队列缓存各医生的等待患者，存在以下问题：

- 系统重启后队列丢失
- 多实例部署时各实例队列互不可见
- 数据库中状态已变，但内存队列可能未同步

**建议**：
- 叫号逻辑以数据库为唯一真相源
- 取消内存队列缓存，每次从数据库查询等待中的挂号
- 或使用 Redis 等分布式队列替代内存队列

**修复状态**：✅ 已完成（2026-06-17）
- 删除 `doctorQueues_` 内存队列和 `getOrCreateQueue()` 方法
- `callNextPatient()` 改为每次从数据库查询

---

## 中优先级：业务正确性

### 5. `current_patients` 字段语义与实际逻辑不符

**位置**：`include/model/Doctor.h`、`sql/schema.sql`

`doctors.current_patients` 被当作“当前未完成挂号数”使用：

- `makeAppointment` 时 `++`
- `callNextPatient` / `cancelAppointment` 时 `--`

但字段名暗示“已接诊人数”。如果后续需要统计“今日已完成接诊数”，就会产生冲突。

**建议**：
- 改名为 `waiting_count` 或 `active_count`
- 或者拆分为两个字段：
  - `waiting_count`：当前等待/就诊中人数（用于负载判断）
  - `completed_count`：今日已完成人数

### 6. 缺少 `InProgress` 状态流转 ✅ 已修复

**位置**：`src/service/AppointmentService.cpp` 中的 `callNextPatient`

叫号时直接标记为 `Completed`：

```cpp
next->status = AppointmentStatus::Completed;
```

缺少”就诊中”中间状态，无法区分”已叫号正在就诊”和”医生已结束诊疗”。

**建议**：
- 叫号时改为 `InProgress`
- 增加”结束就诊”接口，将状态改为 `Completed`
- 等待时间预估和队列显示据此调整

**修复状态**：✅ 已完成（2026-06-17）
- `callNextPatient()` 状态改为 `InProgress`
- 新增 `completeAppointment()` 方法
- 新增 `POST /api/appointments/:id/complete` 接口

### 7. 等待时间预估前后端不一致

**后端**：`src/service/AppointmentService.cpp` 中的 `estimateWaitTime`
**前端**：`web/src/views/screen/index.vue`

后端提供了 `/api/appointments/:id/wait_time` 接口，使用优先队列计算前面人数 × 10 分钟。但大屏直接在前端用 `(index + 1) * 10` 计算，没有调用后端接口。

**建议**：
- 大屏调用后端 wait_time 接口
- 或者将统一算法抽到前后端共享工具中（若项目发展壮大，可考虑 monorepo 共享包）

### 8. `queue_number` 全局自增，不支持按医生/按日重置

**位置**：`src/repository/mysql/MySQLAppointmentRepository.cpp` 中的 `getNextQueueNumber`

当前排队号是整个表全局自增：

```sql
SELECT COALESCE(MAX(queue_number), 0) + 1 FROM appointments
```

不同医生共享流水号，且不会按天清零，不符合医院“每个诊室每日独立叫号”的实际需求。

**建议**：
- 按 `doctor_id + DATE(created_at)` 生成每日独立排队号
- 或者使用 `doctor_id + 日期` 的复合键来维护排队号序列

---

## 前端工程化与类型安全

### 9. TypeScript 类型大量为 `any`

**位置**：`web/src/views/patient/index.vue`、`web/src/views/doctor/index.vue`、`web/src/views/screen/index.vue`

多个视图组件中：

```ts
const doctors = ref<any[]>([])
const patientList = ref<any[]>([])
const queue = ref<any[]>([])
```

失去了 TypeScript 的类型检查和 IDE 提示优势。

**建议**：
- 在 `web/src/types/` 下定义 `Patient`、`Doctor`、`Appointment` 接口
- 替换所有 `any[]` 为具体类型

### 10. 请求拦截器把所有错误都弹窗

**位置**：`web/src/utils/request.ts`

响应拦截器对任何失败请求都调用 `Swal.fire()`：

```ts
Swal.fire({
  icon: 'error',
  title: '系统提示',
  text: errorMsg,
  ...
})
```

医生端和大屏都有定时轮询，一旦网络抖动就会反复弹窗，体验很差。

**建议**：
- 增加 `silent: boolean` 请求配置
- 自动轮询请求设置为静默模式
- 业务层自行决定是否弹窗

### 11. `baseURL` 配置不清晰

**位置**：`web/src/utils/request.ts`

```ts
baseURL: import.meta.env.DEV ? '' : ''
```

两个分支都是空字符串，虽然 Vite proxy 能让 `/api` 走代理，但生产环境依赖后端托管，配置意图不明确。

**建议**：
- 明确 `baseURL: '/api'`
- 后端 `/api` 路由与静态资源分开
- 在 Vite 配置中保持 `/api -> http://localhost:8080` 代理

### 12. 患者注册逻辑每次都新建

**位置**：`web/src/views/patient/index.vue` 中的 `findOrCreatePatient`

该函数没有先查询已有患者，每次输入姓名都会生成随机手机号和身份证并创建新记录。

**建议**：
- 先调用查询接口（如按手机号或姓名查询）
- 只有患者不存在时才创建
- 在界面上提供“选择已有患者”和“新建患者”的明确区分

---

## 工程与运维

### 13. `.gitignore` 不完整

**位置**：`.gitignore`

当前未排除：

- `node_modules/`
- `web/node_modules/.tmp/`
- `public/assets/`（Vite 构建输出）
- 根目录下的 `hospital_system` 可执行文件
- `CMakeFiles/`、`build/` 等已在列但不够完整

**建议**：补充上述条目，避免把构建产物和依赖提交到仓库。

### 14. 根目录存在 `node_modules` 和 `package-lock.json`

根目录下有 `node_modules/` 和 `package-lock.json`，看起来是不小心在根目录执行了 `npm install`。

**建议**：删除根目录下的 `node_modules/`、`package-lock.json`，前端依赖统一放在 `web/` 目录。

### 15. 缺少 `README.md`

项目没有 README，新成员无法快速了解如何编译、启动、配置数据库。

**建议**：补充 README，包含：

- 环境依赖（CMake、MySQL、C++17 编译器、Node.js）
- 数据库初始化命令
- 后端启动方式与环境变量（`MYSQL_HOST`、`MYSQL_PASSWORD`、`SERVER_PORT` 等）
- 前端开发/构建命令
- API 速览与 curl 示例

### 16. 缺少单元测试和接口测试

项目中没有任何测试代码。

**建议**：
- C++ 层：对 `AppointmentService`、`PriorityQueueStrategy`、`DI Container` 加单元测试（GoogleTest）
- 前端：对 `request.ts` 和关键 Vue 组件做基础测试（Vitest + Vue Test Utils）
- 接口层：用 Postman/Newman 或 Python `pytest` 做端到端 API 测试

### 17. Controller 异常处理重复

**位置**：`src/api/ApiController.cpp`

每个 handler 都写几乎一样的 `try/catch` 块，重复代码多，维护成本高。

**建议**：
- 在 `httplib::Server` 层增加异常处理中间件
- 或在 `registerRoutes` 时用统一包装函数处理所有路由的异常
- 统一错误响应格式 `{ "error": "..." }`

### 18. 日志硬编码到 `std::cout/std::cerr`

**位置**：`src/main.cpp`、`src/api/ApiController.cpp`、`src/db/MySQLConnectionPool.cpp`

所有日志直接输出到控制台，没有日志级别、不支持输出到文件、无法按需关闭。

**建议**：
- 引入轻量级日志库（如 `spdlog`）
- 区分 `info/warn/error/debug` 级别
- 支持文件落盘和按大小轮转

---

## 建议的下一步

如果只能先做 3 件事，建议按以下顺序：

1. **修复 `MySQLAppointmentRepository` 的 SQL 注入**：改用预处理语句或白名单校验，消除安全风险。
2. **把 Service 层改为数据库事务**：确保挂号和更新医生名额原子化，避免数据不一致。
3. **统一前端类型 + 改进错误拦截器**：减少 `any`，避免自动轮询弹窗干扰用户体验。

---

## 附录：关键文件清单

| 模块 | 文件路径 |
|------|----------|
| CMake 构建 | `CMakeLists.txt` |
| 入口程序 | `src/main.cpp` |
| DI 容器 | `src/di/Container.h` |
| MySQL 连接池 | `include/db/MySQLConnectionPool.h`、`src/db/MySQLConnectionPool.cpp` |
| Service 层 | `src/service/AppointmentService.h`、`src/service/AppointmentService.cpp` |
| API 控制器 | `src/api/ApiController.h`、`src/api/ApiController.cpp` |
| MySQL Repository | `src/repository/mysql/MySQLPatientRepository.cpp`、`MySQLDoctorRepository.cpp`、`MySQLAppointmentRepository.cpp` |
| 优先队列 | `src/algorithm/PriorityQueueStrategy.h`、`PriorityQueueStrategy.cpp` |
| 前端入口 | `web/src/main.ts` |
| 前端请求 | `web/src/utils/request.ts` |
| 前端视图 | `web/src/views/Home.vue`、`patient/index.vue`、`doctor/index.vue`、`screen/index.vue` |
| 前端样式 | `web/src/assets/main.scss` |
| 数据库脚本 | `sql/schema.sql` |


---

## 附录：补充问题（Code Review 额外发现）

以下为 Code Review 过程中发现的、原改进建议清单中未提及的问题。

### 19. cancelAppointment 未检查 Completed 状态 ✅ 已修复

**位置**：src/service/AppointmentService.cpp:81-96

**问题**：当前只检查 Cancelled 状态，未检查 Completed。已完成的挂号仍可被取消，导致医生 current_patients 被错误减少。

**建议**：在取消前增加判断：
```cpp
if (apt->status == AppointmentStatus::Completed) {
    return Result<void>::failure("该挂号已完成，无法取消");
}
```

**修复状态**：✅ 已完成（2026-06-17）
- 已在 `cancelAppointment()` 中添加 Completed 状态检查

### 20. 前端依赖后端未提供的 patient_name / doctor_name / department 字段 ✅ 已修复

**位置**：
- web/src/views/doctor/index.vue:80 - result.patient_name
- web/src/views/screen/index.vue:57 - item.patient_name
- web/src/views/patient/index.vue:120 - result.doctor_name, result.department

**问题**：后端 API 返回的 JSON 只包含 patient_id / doctor_id，前端读取不存在的字段会显示 undefined 或空。

**建议**：后端 JOIN 患者/医生表返回名称，或前端用已有列表做二次匹配。

**修复状态**：✅ 已完成（2026-06-17）
- 在 `ApiController` 中查询患者/医生信息并组装返回
- 涉及接口：`handleGetDoctorQueue`、`handleMakeAppointment`、`handleAutoRouteAppointment`、`handleCallNextPatient`
- 返回字段包含：`patient_name`、`doctor_name`、`department`

### 21. std::stoll / std::stoi 未分类处理异常

**位置**：src/api/ApiController.cpp 多处

**问题**：URL 参数不是有效数字时，虽然被 std::exception 捕获，但返回 400 且错误信息不友好，可能暴露内部异常。

**建议**：
# 0 "<stdin>"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "<stdin>"

### 22. Result<T>::value() 失败时访问会崩溃

**位置**：include/common/Result.h:42-43

**问题**：ok() 为 false 时调用 value() 会抛出 std::bad_optional_access。

**建议**：
# 0 "<stdin>"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "<stdin>"

### 23. screen/index.vue 状态过滤逻辑硬编码且不完整

**位置**：web/src/views/screen/index.vue:75

**问题**：同时判断 'waiting' || 0 || 'WAITING'，说明前端不确定后端返回格式；未处理 in_progress 状态。

**建议**：与后端约定明确的状态字符串，统一判断逻辑。

### 24. main.cpp 中 std::atoi 对无效输入返回 0

**位置**：src/main.cpp:55-60

**问题**：MYSQL_PORT=abc 时 std::atoi 返回 0，导致 MySQL 连接失败且错误信息不明确。

**建议**：
# 0 "<stdin>"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "<stdin>"

### 25. estimateWaitTime() 未加锁

**位置**：src/service/AppointmentService.cpp:225-260

**问题**：读操作未加 serviceMutex_，可能读到 makeAppointment / cancelAppointment 的中间状态。

**建议**：使用数据库事务隔离级别或在 Service 层增加读锁。

### 26. doctor/index.vue 中 current_patients 被当作等待人数

**位置**：web/src/views/doctor/index.vue:32

**问题**：totalWaiting 用 doc.current_patients 累加，但该字段表示已接诊/负载而非正在等待人数。

**建议**：后端提供专门的统计接口，或前端只展示负载比例。

---

## 附录：补充问题（Code Review 额外发现）

以下为 Code Review 过程中发现的、原改进建议清单中未提及的问题。

### 19. cancelAppointment 未检查 Completed 状态

**位置**：src/service/AppointmentService.cpp:81-96

**问题**：当前只检查 Cancelled 状态，未检查 Completed。已完成的挂号仍可被取消，导致医生 current_patients 被错误减少。

**建议**：在取消前增加判断：
```cpp
if (apt->status == AppointmentStatus::Completed) {
    return Result<void>::failure("该挂号已完成，无法取消");
}
```

### 20. 前端依赖后端未提供的 patient_name / doctor_name / department 字段

**位置**：
- web/src/views/doctor/index.vue:80 - result.patient_name
- web/src/views/screen/index.vue:57 - item.patient_name
- web/src/views/patient/index.vue:120 - result.doctor_name, result.department

**问题**：后端 API 返回的 JSON 只包含 patient_id / doctor_id，前端读取不存在的字段会显示 undefined 或空。

**建议**：后端 JOIN 患者/医生表返回名称，或前端用已有列表做二次匹配。

### 21. std::stoll / std::stoi 未分类处理异常

**位置**：src/api/ApiController.cpp 多处

**问题**：URL 参数不是有效数字时，虽然被 std::exception 捕获，但返回 400 且错误信息不友好，可能暴露内部异常。

**建议**：
```cpp
try {
    int64_t id = std::stoll(req.matches[1]);
} catch (const std::invalid_argument&) {
    setErrorResponse(res, 400, "ID 格式无效");
} catch (const std::out_of_range&) {
    setErrorResponse(res, 400, "ID 超出范围");
}
```

### 22. Result<T>::value() 失败时访问会崩溃

**位置**：include/common/Result.h:42-43

**问题**：ok() 为 false 时调用 value() 会抛出 std::bad_optional_access。

**建议**：
```cpp
const T& value() const {
    if (!ok_) throw std::runtime_error("访问失败结果的值: " + error_message_);
    return value_.value();
}
```

### 23. screen/index.vue 状态过滤逻辑硬编码且不完整

**位置**：web/src/views/screen/index.vue:75

**问题**：同时判断 'waiting' || 0 || 'WAITING'，说明前端不确定后端返回格式；未处理 in_progress 状态。

**建议**：与后端约定明确的状态字符串，统一判断逻辑。

### 24. main.cpp 中 std::atoi 对无效输入返回 0

**位置**：src/main.cpp:55-60

**问题**：MYSQL_PORT=abc 时 std::atoi 返回 0，导致 MySQL 连接失败且错误信息不明确。

**建议**：
```cpp
if (auto portStr = std::getenv("MYSQL_PORT")) {
    config.port = std::stoul(portStr);
}
```

### 25. estimateWaitTime() 未加锁

**位置**：src/service/AppointmentService.cpp:225-260

**问题**：读操作未加 serviceMutex_，可能读到 makeAppointment / cancelAppointment 的中间状态。

**建议**：使用数据库事务隔离级别或在 Service 层增加读锁。

### 26. doctor/index.vue 中 current_patients 被当作等待人数

**位置**：web/src/views/doctor/index.vue:32

**问题**：totalWaiting 用 doc.current_patients 累加，但该字段表示已接诊/负载而非正在等待人数。

**建议**：后端提供专门的统计接口，或前端只展示负载比例。
---

## 附录：补充问题（Code Review 额外发现）

以下为 Code Review 过程中发现的、原改进建议清单中未提及的问题。

### 19. cancelAppointment 未检查 Completed 状态

**位置**：src/service/AppointmentService.cpp:81-96

**问题**：当前只检查 Cancelled 状态，未检查 Completed。已完成的挂号仍可被取消，导致医生 current_patients 被错误减少。

**建议**：在取消前增加判断：
```cpp
if (apt->status == AppointmentStatus::Completed) {
    return Result<void>::failure("该挂号已完成，无法取消");
}
```

### 20. 前端依赖后端未提供的 patient_name / doctor_name / department 字段

**位置**：
- web/src/views/doctor/index.vue:80 - result.patient_name
- web/src/views/screen/index.vue:57 - item.patient_name
- web/src/views/patient/index.vue:120 - result.doctor_name, result.department

**问题**：后端 API 返回的 JSON 只包含 patient_id / doctor_id，前端读取不存在的字段会显示 undefined 或空。

**建议**：后端 JOIN 患者/医生表返回名称，或前端用已有列表做二次匹配。

### 21. std::stoll / std::stoi 未分类处理异常

**位置**：src/api/ApiController.cpp 多处

**问题**：URL 参数不是有效数字时，虽然被 std::exception 捕获，但返回 400 且错误信息不友好，可能暴露内部异常。

**建议**：
```cpp
try {
    int64_t id = std::stoll(req.matches[1]);
} catch (const std::invalid_argument&) {
    setErrorResponse(res, 400, "ID 格式无效");
} catch (const std::out_of_range&) {
    setErrorResponse(res, 400, "ID 超出范围");
}
```

### 22. Result<T>::value() 失败时访问会崩溃

**位置**：include/common/Result.h:42-43

**问题**：ok() 为 false 时调用 value() 会抛出 std::bad_optional_access。

**建议**：
```cpp
const T& value() const {
    if (!ok_) throw std::runtime_error("访问失败结果的值: " + error_message_);
    return value_.value();
}
```

### 23. screen/index.vue 状态过滤逻辑硬编码且不完整

**位置**：web/src/views/screen/index.vue:75

**问题**：同时判断 'waiting' || 0 || 'WAITING'，说明前端不确定后端返回格式；未处理 in_progress 状态。

**建议**：与后端约定明确的状态字符串，统一判断逻辑。

### 24. main.cpp 中 std::atoi 对无效输入返回 0

**位置**：src/main.cpp:55-60

**问题**：MYSQL_PORT=abc 时 std::atoi 返回 0，导致 MySQL 连接失败且错误信息不明确。

**建议**：
```cpp
if (auto portStr = std::getenv("MYSQL_PORT")) {
    config.port = std::stoul(portStr);
}
```

### 25. estimateWaitTime() 未加锁

**位置**：src/service/AppointmentService.cpp:225-260

**问题**：读操作未加 serviceMutex_，可能读到 makeAppointment / cancelAppointment 的中间状态。

**建议**：使用数据库事务隔离级别或在 Service 层增加读锁。

### 26. doctor/index.vue 中 current_patients 被当作等待人数

**位置**：web/src/views/doctor/index.vue:32

**问题**：totalWaiting 用 doc.current_patients 累加，但该字段表示已接诊/负载而非正在等待人数。

**建议**：后端提供专门的统计接口，或前端只展示负载比例。
