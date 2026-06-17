# 医院挂号系统 - 架构讨论记录

## 项目概述

**项目名称**：医院挂号系统
**技术栈**：C++17 + CMake + cpp-httplib + MySQL + 多线程
**架构定位**：前后端分离的工业级、高并发系统

---

## 关键技术决策

### 1. 数据库选择
- **决策**：直接使用 MySQL
- **原因**：贴近最终形态，避免后期迁移成本

### 2. 架构复杂度
- **决策**：平衡型
- **原则**：不过度设计，但保留灵活性。该用高级特性就用，不刻意追求简单或复杂

### 3. 扩展能力要求
需要支持以下后期扩展：
- ✅ 数据库可切换（MySQL ↔ SQLite ↔ 内存存储）
- ✅ 并发模型可升级（单线程 → 线程池 → 协程）
- ✅ 算法可插拔（普通队列 → 优先队列 → 智能调度）

---

## 架构设计

### 目录结构

```
hospital-system/
├── CMakeLists.txt
├── cmake/FindMySQL.cmake
├── include/
│   ├── common/           # Result.h, Exception.h
│   ├── model/            # Patient.h, Doctor.h, Appointment.h
│   ├── repository/       # 数据访问接口（抽象层）
│   ├── service/          # 业务逻辑接口
│   ├── algorithm/        # 算法接口
│   └── controller/       # API 路由
├── src/
│   ├── main.cpp
│   ├── config/           # AppConfig
│   ├── repository/
│   │   ├── mysql/        # MySQL 实现
│   │   └── memory/       # 内存实现（测试用）
│   ├── service/
│   ├── algorithm/
│   ├── controller/
│   └── di/               # 依赖注入容器
├── web/                  # 前端静态文件
├── sql/schema.sql
└── tests/
```

### 核心设计模式

**1. Repository 接口抽象（支持数据库切换）**
```cpp
template<typename Entity, typename Id>
class IRepository {
public:
    virtual ~IRepository() = default;
    virtual std::optional<Entity> findById(Id id) = 0;
    virtual std::vector<Entity> findAll() = 0;
    virtual bool save(const Entity& entity) = 0;
    virtual bool update(const Entity& entity) = 0;
    virtual bool remove(Id id) = 0;
};
```

**2. 队列策略接口（支持算法插拔）**
```cpp
class IQueueStrategy {
public:
    virtual ~IQueueStrategy() = default;
    virtual void enqueue(const Appointment& apt) = 0;
    virtual std::optional<Appointment> dequeue() = 0;
    virtual size_t size() const = 0;
};
```

**3. 依赖注入容器（解耦模块）**
```cpp
class Container {
    std::unordered_map<std::string, std::any> bindings_;
public:
    template<typename Interface, typename Implementation>
    void bind();

    template<typename Interface>
    std::shared_ptr<Interface> resolve();
};
```

### 依赖关系

```
Controller (API 路由)
    ↓ 调用
Service (业务逻辑) ← 依赖注入
    ↓ 调用
Repository (数据访问) ← 接口抽象，可切换实现
    ↓ 实现
MySQL / Memory / SQLite
```

---

## 开发路线

| 阶段 | 目标 | 核心交付物 |
|------|------|-----------|
| **Step 1** | 基础框架 | CMake + 接口定义 + 数据模型 + DI 容器 |
| **Step 2** | MySQL 基础 | 连接池 + Repository 实现 + 建表 SQL |
| **Step 3** | 业务逻辑 | AppointmentService + HTTP API |
| **Step 4** | 算法扩展 | 优先队列 + Dijkstra 路径规划 |

---

## 核心算法要求

1. **急诊插队管理**：`std::priority_queue` 最大堆，按紧急程度和挂号时间排序
2. **排班自动分流**：最小堆贪心算法，分配给当前负载最低的医生
3. **综合导诊与时间预估**：Dijkstra 算法，计算转诊最短路径

---

## 代码规范

- C++17 标准
- 禁止裸指针，使用 `std::unique_ptr` / `std::shared_ptr`
- 使用 RAII 管理资源
- 使用 `std::optional` 处理可能为空的返回值
- 使用 `std::string_view` 替代 `const std::string&` 进行只读传参
- 共享资源使用 `std::mutex` / `std::shared_mutex` 保护
- 自定义异常继承 `std::exception`

---

## 待讨论问题

1. 前端复杂度：简单表单 or SPA？
2. 用户认证：第一版是否需要登录功能？
3. MySQL 环境：是否已安装？

---

## 参考文档

- `Phase1_Setup.md` - 原始架构设计文档
- `ARCHITECTURE_DISCUSSION.md` - 本文件（架构讨论记录）
