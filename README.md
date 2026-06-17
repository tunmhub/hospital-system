# 医院挂号系统

一个基于 C++ 和 Vue 3 的医院挂号排队系统，支持患者挂号、医生叫号、大屏显示等功能。

## 技术栈

### 后端
- **语言**：C++17
- **Web 框架**：cpp-httplib
- **数据库**：MySQL 8.0+
- **JSON**：nlohmann/json
- **构建**：CMake 3.14+

### 前端
- **框架**：Vue 3 + TypeScript
- **构建**：Vite
- **UI**：Bootstrap 5 + Bootstrap Icons
- **HTTP 客户端**：Axios
- **弹窗**：SweetAlert2

## 快速开始

### 1. 数据库初始化

```sql
-- 创建数据库
CREATE DATABASE hospital_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- 导入表结构
mysql -u root -p hospital_db < sql/schema.sql
```

### 2. 后端构建

```bash
# 创建构建目录
mkdir build && cd build

# 配置 CMake
cmake ..

# 编译
make -j$(nproc)

# 运行（需设置环境变量）
export MYSQL_HOST=localhost
export MYSQL_PORT=3306
export MYSQL_USER=root
export MYSQL_PASSWORD=your_password
export MYSQL_DATABASE=hospital_db

./hospital_system
```

### 3. 前端开发

```bash
cd web

# 安装依赖
npm install

# 开发模式
npm run dev

# 构建生产版本
npm run build
```

## 项目结构

```
hospital-system/
├── include/                    # 头文件
│   ├── api/                   # API 控制器
│   ├── common/                # 公共工具（Result, Exception）
│   ├── db/                    # 数据库连接池
│   ├── model/                 # 数据模型
│   ├── repository/            # 仓储接口
│   ├── service/               # 服务接口
│   └── algorithm/             # 队列算法
├── src/                        # 源文件
│   ├── api/                   # API 实现
│   ├── repository/            # 仓储实现
│   │   ├── memory/           # 内存实现（测试用）
│   │   └── mysql/            # MySQL 实现
│   ├── service/               # 服务实现
│   └── main.cpp               # 入口
├── web/                        # 前端项目
│   ├── src/
│   │   ├── views/            # 页面组件
│   │   ├── utils/            # 工具函数
│   │   ├── types/            # TypeScript 类型
│   │   └── router/           # 路由配置
│   └── package.json
├── sql/                        # 数据库脚本
└── CMakeLists.txt              # 构建配置
```

## API 速览

### 患者接口
| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/patients` | 创建患者 |
| GET | `/api/patients` | 患者列表 |
| GET | `/api/patients?search=xxx` | 搜索患者 |
| GET | `/api/patients/:id` | 查询患者 |
| GET | `/api/patients/:id/appointments` | 患者挂号记录 |

### 医生接口
| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/doctors` | 医生列表 |
| GET | `/api/doctors/:id/queue` | 医生排队队列 |
| POST | `/api/doctors/:id/call_next` | 呼叫下一位 |

### 挂号接口
| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/appointments` | 发起挂号 |
| POST | `/api/appointments/auto` | 自动分流挂号 |
| POST | `/api/appointments/:id/cancel` | 取消挂号 |
| POST | `/api/appointments/:id/complete` | 完成就诊 |
| GET | `/api/appointments/:id/wait_time` | 预估等待时间 |

## 核心功能

1. **智能排队**：支持普通、加急、急诊三级优先级
2. **自动分流**：根据医生负载自动分配患者
3. **实时叫号**：医生工作台一键叫号
4. **大屏显示**：排队大厅实时滚动显示
5. **等待预估**：基于队列位置计算等待时间

## 环境变量

| 变量名 | 说明 | 默认值 |
|--------|------|--------|
| MYSQL_HOST | 数据库主机 | localhost |
| MYSQL_PORT | 数据库端口 | 3306 |
| MYSQL_USER | 数据库用户 | root |
| MYSQL_PASSWORD | 数据库密码 | - |
| MYSQL_DATABASE | 数据库名 | hospital_db |

## 许可证

MIT License
