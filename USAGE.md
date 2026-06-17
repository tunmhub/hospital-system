学号：XXXXXXXXXX
姓名：XXX

# 医院挂号系统 - 使用说明

## 一、系统简介

本系统是一个基于 C++17 和 Vue 3 的医院挂号排队系统，采用前后端分离架构，支持患者挂号、医生叫号、医保结算、大屏显示等功能。

### 技术栈

- **后端**：C++17 + cpp-httplib + MySQL 8.0
- **前端**：Vue 3 + TypeScript + Bootstrap 5
- **数据库**：MySQL 8.0+ / MariaDB 10.3+

### 系统特点

1. **智能排队**：支持普通、加急、急诊三级优先级
2. **自动分流**：根据医生负载自动分配患者
3. **医保结算**：支持城镇职工、城镇居民、新农合、自费四种类型
4. **实时显示**：大屏滚动显示排队信息

## 二、环境要求

- **操作系统**：Linux (Ubuntu 20.04+) / macOS / Windows (WSL2)
- **编译器**：GCC 9+ / Clang 10+ / MSVC 2019+
- **CMake**：3.10+
- **MySQL**：8.0+ 或 MariaDB 10.3+
- **Node.js**：16+ (前端开发)

## 三、安装步骤

### 1. 数据库初始化

```bash
# 登录 MySQL
mysql -u root -p

# 执行建表脚本
source sql/schema.sql
```

### 2. 后端编译

```bash
# 创建构建目录
mkdir build && cd build

# 配置 CMake
cmake ..

# 编译
make -j$(nproc)
```

### 3. 启动后端服务

```bash
# 设置环境变量
export MYSQL_HOST=localhost
export MYSQL_PORT=3306
export MYSQL_USER=root
export MYSQL_PASSWORD=your_password
export MYSQL_DATABASE=hospital_db

# 启动服务
./hospital_system
```

服务默认监听 `http://0.0.0.0:8080`

### 4. 前端开发（可选）

```bash
cd web

# 安装依赖
npm install

# 开发模式
npm run dev

# 构建生产版本
npm run build
```

## 四、使用说明

### 4.1 患者挂号

1. 访问 `http://localhost:8080/patient`
2. 输入患者姓名，点击"查询/注册"
3. 如患者不存在，填写手机号和身份证号进行注册
4. 选择就诊科室和医生（可选自动分配）
5. 选择优先级（普通/加急/急诊）
6. 点击"确认挂号"

### 4.2 医生叫号

1. 访问 `http://localhost:8080/doctor`
2. 查看各医生的等待人数和负载情况
3. 点击"叫号"按钮呼叫下一位患者
4. 系统会弹窗显示患者姓名

### 4.3 大屏显示

1. 访问 `http://localhost:8080/screen`
2. 选择要显示的医生
3. 系统自动滚动显示排队信息
4. 包含排队号码、患者姓名、预计等待时间

### 4.4 医保结算

通过 API 进行结算：

```bash
# 结算挂号
curl -X POST http://localhost:8080/api/appointments/1/settle
```

返回示例：
```json
{
  "id": 1,
  "patient_id": 1,
  "patient_name": "张三",
  "insurance_type": "urban_worker",
  "registration_fee": 10.0,
  "insurance_fee": 8.0,
  "self_fee": 2.0,
  "settled": true
}
```

### 4.5 医保类型说明

| 类型 | 代码 | 报销比例 |
|------|------|----------|
| 城镇职工医保 | urban_worker | 80% |
| 城镇居民医保 | urban_resident | 60% |
| 新农合医保 | rural_resident | 60% |
| 自费 | self | 0% |

### 4.6 挂号费说明

| 优先级 | 费用 |
|--------|------|
| 普通 | 10 元 |
| 加急 | 20 元 |
| 急诊 | 50 元 |

## 五、API 文档

### 患者接口

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | /api/patients | 创建患者 |
| GET | /api/patients | 患者列表 |
| GET | /api/patients?search=xxx | 搜索患者 |
| GET | /api/patients/:id | 查询患者 |
| GET | /api/patients/:id/appointments | 患者挂号记录 |

### 医生接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | /api/doctors | 医生列表 |
| GET | /api/doctors/:id/queue | 医生排队队列 |
| POST | /api/doctors/:id/call_next | 呼叫下一位 |

### 挂号接口

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | /api/appointments | 发起挂号 |
| POST | /api/appointments/auto | 自动分流挂号 |
| POST | /api/appointments/:id/cancel | 取消挂号 |
| POST | /api/appointments/:id/complete | 完成就诊 |
| POST | /api/appointments/:id/settle | 医保结算 |
| GET | /api/appointments/:id/wait_time | 预估等待时间 |

### 健康检查

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | /api/health | 服务状态检查 |

## 六、常见问题

### 1. 数据库连接失败

检查环境变量是否正确设置：
```bash
echo $MYSQL_HOST
echo $MYSQL_PORT
echo $MYSQL_USER
echo $MYSQL_PASSWORD
echo $MYSQL_DATABASE
```

### 2. 端口被占用

修改启动端口：
```bash
export SERVER_PORT=9090
./hospital_system
```

### 3. 编译错误

确保安装了正确版本的编译器和 CMake：
```bash
g++ --version  # 需要 9.0+
cmake --version  # 需要 3.10+
```

### 4. 前端无法访问

确保已构建前端并放置在 public 目录：
```bash
cd web
npm run build
cp -r dist ../public
```

## 七、项目结构

```
hospital-system/
├── include/                    # 头文件
│   ├── algorithm/             # 算法实现（优先队列、二分查找、快速排序）
│   ├── api/                   # API 控制器
│   ├── common/                # 公共工具（Result, Exception）
│   ├── db/                    # 数据库连接池
│   ├── model/                 # 数据模型
│   ├── repository/            # 仓储接口
│   └── service/               # 服务接口
├── src/                        # 源文件
├── web/                        # 前端项目
├── sql/                        # 数据库脚本
├── CMakeLists.txt              # 构建配置
├── README.md                   # 项目说明
└── USAGE.md                    # 使用说明（本文件）
```

## 八、算法说明

本系统使用了以下经典算法：

1. **优先队列（堆排序）**：用于患者排队，支持优先级调度
2. **贪心算法**：用于自动分流，选择负载最低的医生
3. **二分查找**：用于快速查找已排序数据
4. **快速排序**：用于数据排序，采用三数取中法优化

## 九、联系方式

如有问题，请联系指导老师或查阅项目文档。
