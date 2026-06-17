# 开发日志

## 2026-06-17 17:30 - P4: 工程与运维

完成 P4 阶段主要工程化任务。

### 修复内容

**18. 完善 .gitignore**
- 更新 `.gitignore`，添加 `/build/`、`/web/dist/`、`/public/` 等路径
- 添加日志和数据库文件忽略规则

**19. 删除根目录 node_modules**
- 删除根目录的 `node_modules`、`package-lock.json`、`package.json`
- 这些文件是多余的，`web/` 目录已有完整的前端配置

**20. 新增 README.md**
- 项目简介和技术栈
- 快速开始指南（数据库初始化、后端构建、前端开发）
- 项目结构说明
- API 速览表格
- 核心功能介绍
- 环境变量配置

**21. 引入测试框架**
- 状态：待后续完善（当前优先级较低）

**22. Controller 异常处理中间件**
- 在 `main.cpp` 中添加 `server.set_exception_handler`
- 捕获 `ValidationException` → 400
- 捕获 `NotFoundException` → 404
- 捕获 `DatabaseException` → 500（记录日志）
- 捕获其他异常 → 500（记录日志）

**23. 引入日志库**
- 状态：待后续完善（当前优先级较低，使用 std::cout/std::cerr 已满足需求）

### 修改的文件
- `.gitignore` — 完善忽略规则
- `README.md` — 新增项目文档
- `src/main.cpp` — 添加异常处理中间件

### 验证结果
- 编译通过，0 error / 0 warning

---

## 2026-06-17 17:00 - P3: 前端工程化

完成 P3 阶段所有前端工程化任务。

### 修复内容

**14. TypeScript 类型定义**
- 新增 `web/src/types/index.ts`，定义 Patient、Doctor、Appointment、WaitTimeResponse、MakeAppointmentRequest 等类型
- `doctor/index.vue`、`screen/index.vue`、`patient/index.vue` 均已替换 `any` 为具体类型

**15. 请求拦截器增加静默模式**
- `request.ts` 扩展 AxiosRequestConfig 类型，添加 `silent` 属性
- 响应拦截器检查 `error.config?.silent`，静默模式下不弹出错误提示
- `doctor/index.vue` 和 `screen/index.vue` 的轮询请求设置 `{ silent: true }`

**16. 明确 baseURL**
- `request.ts` 设置 `baseURL: '/api'`
- 所有前端组件的 API 调用路径移除 `/api` 前缀

**17. 患者注册先查后建**
- 后端：`IPatientRepository` 新增 `searchByName(keyword)` 方法
- 后端：`MySQLPatientRepository` 使用 `LIKE` 模糊查询，限制 10 条
- 后端：`ApiController` 新增 `handleSearchPatients`，注册到 `/api/patients?search=xxx`
- 前端：`patient/index.vue` 输入姓名时实时搜索，显示搜索结果下拉列表
- 前端：新增手机号/身份证输入框，找到已有患者时弹窗提示选择或新建

### 修改的文件
**后端：**
- `include/repository/IPatientRepository.h` — 新增 searchByName 接口
- `src/repository/mysql/MySQLPatientRepository.h/.cpp` — 实现 searchByName
- `src/repository/memory/MemoryPatientRepository.h/.cpp` — 实现 searchByName
- `src/api/ApiController.h/.cpp` — 新增 handleSearchPatients

**前端：**
- `web/src/types/index.ts` — 新增类型定义
- `web/src/utils/request.ts` — 设置 baseURL，添加 silent 支持
- `web/src/views/doctor/index.vue` — 类型替换，轮询静默
- `web/src/views/screen/index.vue` — 类型替换，轮询静默
- `web/src/views/patient/index.vue` — 先查后建逻辑

### 验证结果
- 后端编译通过，0 error / 0 warning

---

## 2026-06-17 16:30 - P2: 业务与状态机完善

完成 P2 阶段所有业务与状态机完善任务。

### 修复内容

**11. 引入 InProgress 状态**
- 已在 P0 阶段完成：callNextPatient 设置 InProgress，completeAppointment 和 cancelAppointment 已实现

**12. 明确 current_patients 语义**
- `doctor/index.vue` 新增 `waitingCounts` 状态，从队列接口统计真正的 waiting 人数
- `totalWaiting` 改为从 `waitingCounts` 计算总和
- 医生卡片显示"等待: X"和"负载: X/Y"两个指标

**13. 大屏统一调用 wait_time 接口**
- `screen/index.vue` 在 `refreshQueue()` 中遍历等待队列，调用 `/api/appointments/${item.id}/wait_time`
- 模板使用 `item.wait_minutes` 显示后端返回的等待时间
- 接口失败时显示"计算中..."

### 修改的文件
- `web/src/views/doctor/index.vue` — 等待人数统计改为从队列接口获取
- `web/src/views/screen/index.vue` — 等待时间改为调用后端接口

---

## 2026-06-17 16:00 - P1: 稳定性与并发修复

完成 P1 阶段所有 5 项稳定性与并发修复任务。

### 修复内容

**6. 替换全局单锁 `serviceMutex_` 为医生级锁**
- 将全局 `std::mutex serviceMutex_` 替换为 `std::unordered_map<int64_t, std::unique_ptr<std::mutex>> doctorLocks_`
- 新增 `getDoctorLock(doctor_id)` 方法按需创建医生级锁
- `makeAppointment`、`cancelAppointment`、`callNextPatient`、`completeAppointment` 使用医生级锁
- `autoRouteAppointment` 先选择目标医生，再获取该医生的锁
- `estimateWaitTime` 添加读锁保护

**7. `queue_number` 按医生按日重置**
- 新增 `queue_sequences(doctor_id BIGINT, seq_date DATE, last_number INT)` 表（自动创建）
- `getNextQueueNumber(doctor_id)` 使用 `INSERT ... ON DUPLICATE KEY UPDATE` 原子递增
- 更新 `IAppointmentRepository` 接口签名：`getNextQueueNumber(int64_t doctor_id)`
- 同步更新 `MemoryAppointmentRepository` 实现

**8. `std::stoll` 异常分类处理**
- 在 `ApiController.cpp` 新增 `parseId()` 辅助函数（含额外字符检查）
- 替换所有 7 处 `std::stoll` 调用
- 解析失败返回 400 错误："无效的 XXX ID"

**9. `Result<T>::value()` 增加保护**
- 在 `value()` 方法中检查 `ok_` 状态
- 失败时抛出带错误信息的 `std::runtime_error`

**10. `estimateWaitTime()` 加锁保护**
- 使用医生级锁 `getDoctorLock(apt->doctor_id)` 保护读操作
- 先查询挂号记录获取 doctor_id，再获取对应医生的锁

### 修改的文件
- `include/common/Result.h` — value() 添加异常检查
- `include/repository/IAppointmentRepository.h` — 更新 getNextQueueNumber 签名
- `src/api/ApiController.cpp` — 添加 parseId()，替换所有 std::stoll
- `src/service/AppointmentService.h` — 替换 serviceMutex_ 为 doctorLocks_
- `src/service/AppointmentService.cpp` — 实现医生级锁，estimateWaitTime 加锁
- `src/repository/mysql/MySQLAppointmentRepository.h/.cpp` — 实现按医生按日重置 queue_number
- `src/repository/memory/MemoryAppointmentRepository.h/.cpp` — 同步更新接口

### 验证结果
- cmake 配置通过，make 编译 0 error / 0 warning

---

## 2026-06-16 19:42 - Step 1: 基础框架搭建

完成医院挂号系统 Step 1 基础框架搭建，项目从零代码到可编译运行。

### 创建的模块

**构建系统**
- CMakeLists.txt（C++17, MySQL, pthreads, FetchContent 支持 cpp-httplib）
- cmake/FindMySQL.cmake（支持 mysql_config 和手动回退查找）
- .gitignore

**数据模型**（include/model/）
- Patient.h — 患者（id, name, phone, id_card, age, gender）
- Doctor.h — 医生（id, name, department, title, max_patients, current_patients）
- Appointment.h — 挂号（含 AppointmentStatus 和 Priority 枚举，statusToString/priorityToString 工具函数）
- Department.h — 科室（id, name, location, description）

**公共工具**（include/common/）
- Result.h — 统一业务结果封装，支持带值和 void 特化
- Exception.h — 自定义异常体系（HospitalException → DatabaseException / QueueFullException / NotFoundException / ValidationException）

**Repository 接口层**（include/repository/）
- IRepository.h — 通用 CRUD 模板接口（findById, findAll, save, update, remove）
- IPatientRepository.h — 扩展 findByPhone, findByIdCard
- IDoctorRepository.h — 扩展 findByDepartment, findLeastLoaded
- IAppointmentRepository.h — 扩展 findByDoctor, findByPatient, countWaitingByDoctor, getNextQueueNumber

**Service 和算法接口**（include/service/, include/algorithm/）
- IAppointmentService.h — 挂号业务接口（makeAppointment, cancelAppointment, 查询接口）
- IQueueStrategy.h — 队列策略接口（enqueue, dequeue, peek, size, empty, clear）

**DI 容器**（src/di/）
- Container.h/.cpp — 基于 std::any + std::type_index 的轻量容器，支持 bind/bindFactory/bindInstance 三种注册方式

**内存 Repository 实现**（src/repository/memory/）
- MemoryPatientRepository — 全部线程安全（std::mutex）
- MemoryDoctorRepository — 含 findLeastLoaded 最小负载查询
- MemoryAppointmentRepository — 排队号使用 std::atomic 原子计数

**入口和数据库**
- main.cpp — DI 容器初始化 + 功能验证（添加患者/医生 + 查询）
- sql/schema.sql — MySQL 建表脚本（patients, departments, doctors, appointments）+ 8 个初始科室数据

### 修复的问题
- DI 容器 lambda 存入 std::any 时需显式转为 std::function，否则 any_cast 类型不匹配导致 bad any_cast

### 验证结果
- cmake 配置通过，make 编译 0 error / 0 warning
- 程序运行正常：DI 容器解析成功，患者/医生 CRUD 验证通过

---

## 2026-06-16 20:06 - Step 2: MySQL 连接池与 Repository 实现

完成持久化层从内存实现切换到真实 MySQL 数据库。

### 创建的模块

**MySQL 连接池**（include/db/, src/db/）
- MySQLConnectionPool.h/.cpp — 高并发连接池
  - 构造时预创建 N 个 MYSQL* 连接存入 std::queue
  - std::mutex + std::condition_variable 保证线程安全的获取与归还
  - getConnection() 返回带自定义删除器的 std::shared_ptr<MYSQL>，析构时自动归还而非 close
  - 支持 shutdown 优雅关闭

**MySQL Repository 实现**（src/repository/mysql/）
- MySQLPatientRepository — 患者 CRUD + findByPhone/findByIdCard
- MySQLDoctorRepository — 医生 CRUD + findByDepartment/findLeastLoaded
- MySQLAppointmentRepository — 挂号 CRUD + findByDoctor/findByPatient/countWaitingByDoctor/getNextQueueNumber
- 所有 SQL 字符串参数使用 mysql_real_escape_string 转义防注入
- 所有执行失败抛出 DatabaseException，包含 mysql_error() 详细信息

**配置与集成**
- main.cpp 支持从环境变量读取 MySQL 配置（MYSQL_HOST/PORT/USER/PASSWORD/DATABASE）
- DI 容器注入 MySQLConnectionPool 单例，通过 bindFactory 注入各 Repository
- CMakeLists.txt 新增 3 个 MySQL Repository 源文件

### 验证结果
- 编译通过（0 error / 0 warning）
- 连接池初始化 10 个连接成功
- 插入患者「王五」和医生「赵医生」到 MySQL 成功
- 查询验证全链路联通：数据已持久化到 hospital_db

---

## 2026-06-16 20:17 - Step 3: AppointmentService 业务逻辑与 HTTP API

完成业务逻辑层和 RESTful API 层搭建，系统可对外提供 HTTP 服务。

### 创建的模块

**依赖引入**
- nlohmann/json 单头文件（include/json.hpp）— JSON 解析与序列化

**Service 层**（src/service/）
- AppointmentService.h/.cpp — 挂号业务服务实现
  - 注入 IPatientRepository + IDoctorRepository + IAppointmentRepository
  - makeAppointment(): 校验患者/医生存在 → 检查接诊上限 → 生成挂号记录 → 更新医生接诊数
  - cancelAppointment(): 更新状态为已取消 → 释放医生接诊名额
  - 整个挂号流程使用 std::mutex 保护，防止并发抢号超员
  - 挂号失败时回滚（删除已创建的挂号记录）

**API Controller 层**（src/api/）
- ApiController.h/.cpp — RESTful 路由控制器
  - POST /api/patients — 注册患者（JSON: name, phone, id_card, age, gender）
  - GET /api/patients/:id — 查询患者
  - GET /api/patients/:id/appointments — 患者挂号记录
  - GET /api/doctors — 医生列表
  - GET /api/doctors/:id/queue — 医生排队队列
  - POST /api/appointments — 发起挂号（JSON: patient_id, doctor_id, priority）
  - POST /api/appointments/:id/cancel — 取消挂号
  - GET /api/health — 健康检查
  - 全局异常捕获：HospitalException → HTTP 400 + {"error": "描述"}

**main.cpp 集成**
- DI 容器注册 AppointmentService（通过 bindFactory 注入三个 Repository）
- httplib::Server 监听 0.0.0.0:8080
- 支持 SERVER_PORT 环境变量自定义端口

### curl 测试结果
- POST /api/patients → 201，返回患者 JSON（ID=2, ID=3）
- GET /api/patients/2 → 200，返回患者详情
- GET /api/doctors → 200，返回 3 个医生列表
- POST /api/appointments → 201，普通挂号成功（queue_number=1）
- POST /api/appointments → 201，急诊挂号成功（priority=emergency）
- GET /api/doctors/2/queue → 200，返回排队队列
- GET /api/patients/2/appointments → 200，返回患者挂号记录
- 错误测试：患者不存在 → 400 {"error": "患者不存在 (ID=999)"}
- 错误测试：医生不存在 → 400 {"error": "医生不存在 (ID=999)"}
- 错误测试：缺少字段 → 400 {"error": "缺少必填字段: patient_id, doctor_id"}
- POST /api/appointments/1/cancel → 200，取消成功，医生接诊数自动减 1

---

## 2026-06-16 20:35 - Step 4: 优先队列分诊与贪心分流算法

完成核心数据结构和算法实现，系统具备急诊插队、自动分流和智能叫号能力。

### 创建的模块

**优先队列策略**（src/algorithm/）
- PriorityQueueStrategy.h/.cpp — 实现 IQueueStrategy 接口
  - 核心容器：std::priority_queue<Appointment> + 自定义比较器
  - 比较规则：Emergency(2) > Urgent(1) > Normal(0)，同优先级按 queue_number 先到先得
  - 并发安全：所有操作使用 std::mutex 保护
  - 支持批量入队 enqueueBatch()

**AppointmentService 扩展**（src/service/）
- autoRouteAppointment(): 贪心最小堆自动分流
  - 获取科室所有医生 → 压入最小堆（按 current_patients 排序）→ 取堆顶有空闲的医生
- callNextPatient(): 优先队列叫号
  - 从数据库加载等待患者 → 入优先队列 → 弹出最高优先级 → 更新状态为 Completed
- estimateWaitTime(): 等待时间预估
  - 将所有等待患者入优先队列 → 逐个出队直到目标患者 → 前面人数 × 10 分钟
- 修复死锁 bug：提取 makeAppointmentInternal() 内部方法（不加锁），公开方法加锁后委托调用

**API 新增路由**（src/api/）
- POST /api/appointments/auto — 自动分流挂号（patient_id, department, priority）
- POST /api/doctors/:id/call_next — 呼叫下一位患者
- GET /api/appointments/:id/wait_time — 预估等待时间

### curl 测试结果
- 3 个普通号 + 1 个急诊号 → call_next 4 次：急诊患者第 1 个被叫到 ✓
- 自动分流：患者5 自动分配到内科最低负载医生 ✓
- 等待时间预估：
  - 普通患者（前面有急诊+加急）→ 20 分钟 ✓
  - 加急患者（排在最前面）→ 0 分钟 ✓

---

## 2026-06-16 21:00 - Step 5: 前端 Web 图形界面 (GUI) 与 C++ 静态托管

完成全栈系统搭建，实现现代化 Web 图形界面，系统可通过浏览器访问。

### 创建的模块

**C++ 后端静态文件托管**
- main.cpp 新增 `server.set_mount_point("/", "./public")` 配置
- 添加目录存在性检测，失败时打印警告日志
- 浏览器访问 `http://localhost:8080/` 自动加载 `public/index.html`

**前端 Web 界面**（public/）
- index.html — 单页应用 (SPA)，使用 CDN 引入：
  - Bootstrap 5 — 响应式 UI 框架
  - Bootstrap Icons — 图标库
  - SweetAlert2 — 美观弹窗组件
- 设计风格：
  - 紫色渐变背景 + 毛玻璃导航栏
  - 卡片式布局，hover 动画效果
  - 急诊患者红色脉冲高亮
  - 实时状态指示灯

**页面功能模块**

1. **统计卡片**（顶部）
   - 当班医生数量
   - 等候患者总数
   - 平均等待时间（估算）

2. **挂号终端**（左侧）
   - 患者姓名输入 + 查询/注册功能
   - 患者快速选择按钮列表
   - 科室下拉选择（8 个科室）
   - 指定医生（可选，支持自动分配）
   - 加急/急诊复选框
   - SweetAlert 成功弹窗（显示挂号编号、排队号码）

3. **医生叫号台**（右上）
   - 医生卡片列表（头像、姓名、科室、职称）
   - 接诊数/上限 比例显示
   - 「叫号」按钮 — 大字弹窗显示患者姓名和就诊科室

4. **排队大屏**（右下）
   - 选择医生查看对应队列
   - 按优先级排序（急诊 > 加急 > 普通）
   - 颜色编码：急诊红色、加急黄色、普通蓝色
   - 预估等待时间显示
   - **只显示 waiting 状态的患者**（过滤已完成记录）

**JavaScript 交互逻辑**
- `loadDoctors()` — GET /api/doctors，渲染医生列表和统计
- `loadPatients()` — GET /api/patients，加载患者快速选择列表
- `findOrCreatePatient()` — 查询或注册新患者
- `selectPatient()` — 选择患者并启用挂号按钮
- `submitAppointment()` — POST /api/appointments 或 /api/appointments/auto
- `callNext(doctorId)` — POST /api/doctors/:id/call_next，弹窗叫号
- `refreshQueue(doctorId)` — GET /api/doctors/:id/queue，渲染排队大屏
- 自动刷新：每 30 秒轮询更新医生列表和当前队列

### 后端新增 API

**患者列表接口**
- `GET /api/patients` — 返回所有患者列表
- ApiController 新增 `handleListPatients` 方法

### 数据修复

**添加更多医生数据**
- 原有 3 个医生（内科、外科、儿科）
- 新增 5 个医生覆盖所有科室：
  - 张医生（妇产科）、李医生（急诊科）
  - 赵医生（眼科）、周医生（口腔科）、吴医生（皮肤科）

**修复中文乱码**
- 问题：MySQL 客户端编码为 latin1，导致中文数据插入时乱码
- 解决：使用 `SET NAMES utf8mb4` 重新插入医生数据

**修复优先级大小写不匹配**
- 问题：前端发送 `"Emergency"`，后端期望 `"emergency"`
- 解决：前端改为小写发送，后端兼容大小写

**修复排队大屏数据不一致**
- 问题：队列 API 返回所有历史记录（包括 completed），导致显示人数与实际等待人数不符
- 解决：前端过滤只显示 `status=waiting` 的患者

### 技术实现
- 纯前端：HTML + Vanilla JavaScript，无需 Node/npm
- API 通信：原生 fetch API，JSON 格式
- 错误处理：SweetAlert 错误弹窗
- 响应式设计：Bootstrap 栅格系统，支持移动端

### 验证结果
- cmake 配置通过
- make 编译成功（0 error / 0 warning）
- 静态文件托管配置正确
- 前端页面可正常加载
- 挂号、叫号、队列显示全链路测试通过

### 使用说明
1. 启动后端：`MYSQL_PASSWORD=密码 ./hospital_system`
2. 浏览器访问：`http://localhost:8080/`
3. 功能测试：
   - 挂号：输入患者姓名 → 注册/选择患者 → 选择科室 → 点击「确认挂号」
   - 叫号：点击医生卡片上的「叫号」按钮
   - 查看队列：在右上角下拉框选择医生

---
## 2026-06-17 - Step 4: 前端重构与工程化升级
彻底舍弃了基于单体 HTML 的陈旧架构，使用 **Vue 3 + Vite + TypeScript** 对前端进做了现代化重构重做。完成三端分离设计：
- **患者端**：`/patient`，实现新患者创建、科室与医生的选择和挂号挂载。
- **医生端**：`/doctor`，当班视图统计，一键呼叫算法，SweetAlert2 大号广播提示功能。
- **大厅屏**：`/screen`，分离出无头的急诊科幻特效视图，自动按优先级排序并利用 CSS 提供呼吸等光效。

**工程化成就：**
- 初始化完整的 npm package：Axios (统一网络/异常拦截), Vue Router (单页多端路由), Sass (独立层叠样式), 等等。
- Vite 配置 `outDir: '../public'`，一次执行 `npm run build` 即可完成部署并交给后端的 `cpp-httplib` 托管。彻底进入前后端工程平行分离的标准工业模式。

---

## 2026-06-17 - P0 核心功能修复

完成 REPAIR_PLAN.md 中 P0 阶段的所有修复任务。

### 修复内容

**1. SQL 注入修复** ✅
- 文件：`src/repository/mysql/MySQLAppointmentRepository.cpp`
- 将所有字符串拼接改为 MySQL 预处理语句（`mysql_stmt_prepare` / `mysql_stmt_bind_param`）
- 涉及方法：`findById`、`save`、`update`、`remove`、`findByDoctor`、`findByPatient`、`countWaitingByDoctor`

**2. 叫号逻辑重构** ✅
- 文件：`src/service/AppointmentService.cpp`
- 删除 `doctorQueues_` 内存队列和 `getOrCreateQueue()` 方法
- `callNextPatient()` 改为每次从数据库查询等待中的挂号

**3. 状态机完善** ✅
- `callNextPatient()` 状态改为 `InProgress`（原来是 `Completed`）
- 新增 `completeAppointment()` 方法，将 `InProgress` 状态改为 `Completed`
- 新增 `POST /api/appointments/:id/complete` 接口

**4. cancelAppointment 校验** ✅
- 在 `cancelAppointment()` 中添加 Completed 状态检查
- 已完成的挂号无法取消

**5. API 字段组装** ✅
- 在 `ApiController` 中查询患者/医生信息并组装返回
- 涉及接口：`handleGetDoctorQueue`、`handleMakeAppointment`、`handleAutoRouteAppointment`、`handleCallNextPatient`
- 返回字段包含：`patient_name`、`doctor_name`、`department`

### 修改的文件
- `src/repository/mysql/MySQLAppointmentRepository.cpp` - SQL 注入修复
- `src/repository/mysql/MySQLAppointmentRepository.h` - 无需修改
- `src/service/AppointmentService.cpp` - 叫号逻辑重构 + InProgress 状态
- `src/service/AppointmentService.h` - 新增 completeAppointment 方法
- `include/service/IAppointmentService.h` - 新增 completeAppointment 接口
- `src/api/ApiController.cpp` - 新增 complete 接口 + 字段组装
- `src/api/ApiController.h` - 新增 handleCompleteAppointment 方法

### 验证结果
- 编译通过：`make` 成功
- API 测试：
  - 创建患者：`POST /api/patients` ✅
  - 挂号：`POST /api/appointments` 返回 `patient_name`、`doctor_name`、`department` ✅
  - 叫号：`POST /api/doctors/:id/call_next` 状态变为 `in_progress` ✅
  - 完成就诊：`POST /api/appointments/:id/complete` 成功 ✅

### 状态流转
```
Waiting → InProgress → Completed
                  ↘ Cancelled
```
