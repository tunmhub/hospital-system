# 医院挂号系统修复方案

## 修复原则

1. **数据库为唯一真相源**：核心叫号、排队、负载全部以 MySQL 为准，弱化/移除内存缓存。
2. **状态机完整**：挂号状态流转：Waiting -> InProgress -> Completed / Cancelled。
3. **前后端字段对齐**：API 返回前端展示所需的 `patient_name`、`doctor_name`、`department` 等字段。
4. **并发下沉到数据库**：用事务 + 行锁替代全局单锁。
5. **工程化补齐**：类型、测试、日志、文档。

---

## 第一阶段：P0 核心功能修复（必须） ✅ 已完成

> **完成时间**：2026-06-17  
> **验证状态**：编译通过，API 测试成功

### 1. 叫号逻辑改为数据库驱动 ✅

**问题**：内存队列 `doctorQueues_` 与数据库状态不一致。

**方案**：
- 删除 `AppointmentService::doctorQueues_` 和 `getOrCreateQueue()` 中的缓存逻辑。
- `callNextPatient()` 每次直接从数据库查询该医生所有 `Waiting` 状态的挂号记录，构建临时优先队列，弹出队首后更新状态。
- 同时修复 `InProgress` 状态流转。

```cpp
Result<Appointment> AppointmentService::callNextPatient(int64_t doctor_id) {
    std::lock_guard<std::mutex> lock(serviceMutex_);
    
    auto waiting = appointmentRepo_->findByDoctor(doctor_id);
    std::vector<Appointment> active;
    for (const auto& apt : waiting) {
        if (apt.status == AppointmentStatus::Waiting) active.push_back(apt);
    }
    if (active.empty()) return Result<Appointment>::failure("无等待患者");
    
    PriorityQueueStrategy queue;
    queue.enqueueBatch(active);
    auto next = queue.dequeue();
    if (!next) return Result<Appointment>::failure("无等待患者");
    
    next->status = AppointmentStatus::InProgress;
    auto res = appointmentRepo_->update(*next);
    if (!res) return Result<Appointment>::failure("更新叫号状态失败");
    
    return Result<Appointment>::success(*next);
}
```

并新增接口 `completeAppointment(appointment_id)` 将 `InProgress` -> `Completed`，并减少 `doctor.current_patients`。

**实现**：
- 删除 `doctorQueues_` 内存队列和 `getOrCreateQueue()` 方法
- `callNextPatient()` 改为每次从数据库查询
- 新增 `completeAppointment()` 方法
- 更新 `IAppointmentService` 接口

---

### 2. `cancelAppointment` 增加 Completed 校验 ✅

```cpp
if (apt->status == AppointmentStatus::Cancelled) {
    return Result<void>::failure("该挂号已取消");
}
if (apt->status == AppointmentStatus::Completed) {
    return Result<void>::failure("该挂号已完成，无法取消");
}
```

**实现**：已在 `AppointmentService::cancelAppointment()` 中添加 Completed 状态检查。

---

### 3. API 返回前端展示字段 ✅

**涉及接口**：
- `GET /api/doctors/:id/queue`
- `POST /api/doctors/:id/call_next`
- `POST /api/appointments` 和 `/auto`
- `GET /api/patients/:id/appointments`

**方案**：在 `ApiController` 中查询患者/医生信息，组装返回：

```json
{
  "id": 1,
  "patient_id": 5,
  "patient_name": "张三",
  "doctor_id": 2,
  "doctor_name": "李医生",
  "department": "内科",
  "status": "waiting",
  "priority": "normal",
  "queue_number": 12,
  "created_at": "..."
}
```

后端用 `patientRepo->findById` + `doctorRepo->findById` 组装。为避免 N+1，可在 Repository 增加 `findWithNames()` 方法用 JOIN 一次查询。

**实现**：
- 在 `ApiController` 的 `handleGetDoctorQueue`、`handleMakeAppointment`、`handleAutoRouteAppointment`、`handleCallNextPatient` 中查询患者/医生信息并组装返回
- 新增 `POST /api/appointments/:id/complete` 接口

---

### 4. Service 层接入数据库事务

> **状态**：⏸️ 待后续优化（当前使用全局锁保证原子性）

**方案**：
- 在 `MySQLConnectionPool` 增加事务连接支持，或新增 `beginTransaction()` / `commit()` / `rollback()` 方法。
- 在 `AppointmentService` 中把"检查上限 -> 保存挂号 -> 更新医生接诊数"三个操作包在一个事务中。

更简洁的方案：Repository 的 `save` / `update` 使用 `SELECT ... FOR UPDATE` 锁住医生行，保证原子性。

---

### 5. 修复 SQL 注入风险 ✅

**方案**：把 `MySQLAppointmentRepository` 中的字符串拼接改为 MySQL 预处理语句。

```cpp
// 示例：findByDoctor
const char* sql = "SELECT * FROM appointments WHERE doctor_id = ?";
auto stmt = mysql_stmt_init(conn);
mysql_stmt_prepare(stmt, sql, strlen(sql));
MYSQL_BIND bind[1] = {};
long long doc_id = doctor_id;
bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
bind[0].buffer = &doc_id;
mysql_stmt_bind_param(stmt, bind);
mysql_stmt_execute(stmt);
```

**实现**：已将以下方法全部改为预处理语句：
- `findById` / `save` / `update` / `remove`
- `findByDoctor` / `findByPatient`
- `countWaitingByDoctor`

---

## 第二阶段：P1 稳定性与并发修复 ✅ 已完成

> **完成时间**：2026-06-17
> **验证状态**：编译通过

### 6. 替换全局单锁 `serviceMutex_` ✅

**方案**：
- 短期：按医生维度加锁 `std::unordered_map<int64_t, std::mutex> doctorLocks_`。
- 长期：并发控制完全下沉到数据库，使用 `SELECT ... FOR UPDATE` 或事务隔离级别。

```cpp
std::mutex& getDoctorLock(int64_t doctor_id) {
    std::lock_guard<std::mutex> lock(lockMapMutex_);
    return doctorLocks_[doctor_id];
}
```

**实现**：
- 将 `serviceMutex_` 替换为 `std::unordered_map<int64_t, std::unique_ptr<std::mutex>> doctorLocks_`
- 新增 `getDoctorLock(doctor_id)` 方法按需创建医生级锁
- `makeAppointment`、`cancelAppointment`、`callNextPatient`、`completeAppointment` 使用医生级锁
- `autoRouteAppointment` 先选择目标医生，再获取该医生的锁
- `estimateWaitTime` 添加读锁保护

---

### 7. `queue_number` 按医生按日重置 ✅

**方案**：
- 新增表 `queue_sequences(doctor_id, date, last_number)`。
- `getNextQueueNumber(doctor_id)` 使用事务 + 行锁获取并递增。

```sql
INSERT INTO queue_sequences (doctor_id, date, last_number)
VALUES (?, CURDATE(), 1)
ON DUPLICATE KEY UPDATE last_number = last_number + 1;
SELECT last_number FROM queue_sequences WHERE doctor_id = ? AND date = CURDATE();
```

**实现**：
- 新增 `queue_sequences(doctor_id BIGINT, seq_date DATE, last_number INT)` 表（自动创建）
- `getNextQueueNumber(doctor_id)` 使用 `INSERT ... ON DUPLICATE KEY UPDATE` 原子递增
- 更新 `IAppointmentRepository` 接口签名：`getNextQueueNumber(int64_t doctor_id)`
- 同步更新 `MemoryAppointmentRepository` 实现

---

### 8. `std::stoll` 异常分类处理 ✅

在 `ApiController` 中增加一个辅助函数：

```cpp
std::optional<int64_t> parseId(const std::string& s) {
    try {
        return std::stoll(s);
    } catch (const std::invalid_argument&) {
        return std::nullopt;
    } catch (const std::out_of_range&) {
        return std::nullopt;
    }
}
```

所有路由先调用 `parseId`，失败返回 400。

**实现**：
- 在 `ApiController.cpp` 新增 `parseId()` 辅助函数（含额外字符检查）
- 替换所有 7 处 `std::stoll` 调用
- 解析失败返回 400 错误："无效的 XXX ID"

---

### 9. `Result<T>::value()` 增加保护 ✅

```cpp
const T& value() const {
    if (!ok_) throw std::runtime_error("访问失败结果的值: " + error_message_);
    return value_.value();
}
```

**实现**：
- 在 `value()` 方法中检查 `ok_` 状态
- 失败时抛出带错误信息的 `std::runtime_error`

---

### 10. `estimateWaitTime()` 加锁或事务读 ✅

```cpp
std::lock_guard<std::mutex> lock(serviceMutex_);
// 原有逻辑
```

**实现**：
- 使用医生级锁 `getDoctorLock(apt->doctor_id)` 保护读操作
- 先查询挂号记录获取 doctor_id，再获取对应医生的锁

---

## 第三阶段：P2 业务与状态机完善

### 11. 引入 `InProgress` 状态

- `callNextPatient()` 状态改为 `InProgress`。
- 新增 `POST /api/appointments/:id/complete` 完成就诊。
- 新增 `POST /api/appointments/:id/cancel` 取消。
- `doctor.current_patients` 只在真正完成或取消时变化。

---

### 12. 明确 `current_patients` 语义

**方案 A**：改名为 `current_load`（当前未完成挂号数）。
**方案 B**：拆成 `waiting_count` 和 `completed_count`。

同步修改前端 `doctor/index.vue` 的 `totalWaiting` 为真正的等待人数统计（后端提供接口或前端从队列长度计算）。

---

### 13. 大屏统一调用 `wait_time` 接口

修改 `screen/index.vue`，移除本地计算，改为调用：

```ts
const { data } = await api.get(`/appointments/${item.id}/wait_time`)
item.wait_minutes = data.wait_minutes
```

---

## 第四阶段：P3 前端工程化

### 14. TypeScript 类型定义

新增 `web/src/types/index.ts`：

```ts
export interface Patient { id: number; name: string; phone: string; }
export interface Doctor { id: number; name: string; department: string; }
export interface Appointment { id: number; patient_id: number; patient_name?: string; }
```

所有 `any` 替换为具体类型。

---

### 15. 请求拦截器增加静默模式

```ts
api.interceptors.response.use(
  res => res,
  err => {
    if (!err.config?.silent) {
      Swal.fire({ icon: 'error', title: '请求失败', text: err.message })
    }
    return Promise.reject(err)
  }
)
```

轮询请求设置 `silent: true`。

---

### 16. 明确 `baseURL`

```ts
const api = axios.create({
  baseURL: '/api',
  timeout: 10000,
})
```

---

### 17. 患者注册先查后建

修改 `patient/index.vue`：
1. 输入姓名后先调用 `GET /api/patients?search=xxx` 查询。
2. 存在则选择，不存在再新建。
3. 手机号/身份证改为输入框，不再随机生成。

后端新增 `GET /api/patients?search=xxx` 接口。

---

## 第五阶段：P4 工程与运维

### 18. 完善 `.gitignore`

```gitignore
# Build
/build/
/cmake-build-*/
*.exe
hospital_system

# Frontend
/web/node_modules/
/web/dist/
/public/assets/
/public/index.html

# IDE
.idea/
.vscode/
*.swp

# OS
.DS_Store
Thumbs.db
```

### 19. 删除根目录 `node_modules` 和 `package-lock.json`

确认根目录下是否有前端依赖文件，迁移到 `web/` 后删除。

### 20. 新增 `README.md`

包含：项目简介、技术栈、数据库初始化、构建运行、API 速览、目录说明。

### 21. 引入测试框架

- 后端：Google Test
  - `AppointmentService` 单元测试（使用 `MemoryRepository`）
  - `PriorityQueueStrategy` 排序测试
  - Repository 集成测试
- 前端：Vitest + Vue Test Utils
- 接口：Postman / pytest

### 22. Controller 异常处理中间件

cpp-httplib 支持 `set_exception_handler`，统一处理：

```cpp
server.set_exception_handler([](const auto& req, auto& res, std::exception_ptr ep) {
    try {
        if (ep) std::rethrow_exception(ep);
    } catch (const ValidationException& e) {
        setErrorResponse(res, 400, e.what());
    } catch (const NotFoundException& e) {
        setErrorResponse(res, 404, e.what());
    } catch (const DatabaseException& e) {
        setErrorResponse(res, 500, e.what());
    } catch (...) {
        setErrorResponse(res, 500, "服务器内部错误");
    }
});
```

### 23. 引入日志库

使用 `spdlog` 替代 `std::cout/std::cerr`，支持分级和文件输出。

---

## 推荐执行顺序

| 阶段 | 内容 | 预计影响 |
|------|------|----------|
| **P0** | 数据库驱动叫号、cancel 校验、API 字段、事务、SQL 注入 | 核心功能修复，必须优先 |
| **P1** | 锁粒度、queue_number、异常处理、Result 保护 | 稳定性和并发 |
| **P2** | InProgress 状态、current_patients 语义、wait_time 接口 | 业务完整性 |
| **P3** | 前端类型、弹窗、baseURL、患者注册 | 前端质量 |
| **P4** | 测试、README、日志、gitignore | 工程化 |
