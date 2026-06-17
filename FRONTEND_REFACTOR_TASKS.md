# 🏥 医院挂号系统 - 前端重构任务交接单

## 📌 当前进度 (截至 6月17日)
- [x] **创建工程**：已在项目根目录 `/home/tunm/hospital-system/web/` 下初始化了 Vue 3 + Vite + TypeScript 工程。
- [x] **依赖安装**：已安装 `vue-router`, `axios`, `pinia`, `bootstrap`, `bootstrap-icons`, `sweetalert2`, `sass` 以及 `@types/node`。
- [x] **Vite 配置**：已在 `vite.config.ts` 中配置：
  - 本地代理（`/api` 转发至 `http://localhost:8080` 后端服务）
  - 打包输出目录 (`outDir: '../public'`)
  - `@/` 路径别名（指向 `src`）

## 🚀 待完成任务清单 (To-Do List)

### Phase 1: 基础工程化配置 (Base Setup)
- [x] **完善 TS 配置**：修改 `web/tsconfig.app.json` 或 `web/tsconfig.json` ，配置 `compilerOptions.baseUrl` 和 `paths`，使得 `@/*` 解析合法。
- [x] **全局样式引入**：在 `web/src/main.ts` 中引入 Bootstrap 5 及 Bootstrap-Icons 的原生 CSS。
- [x] **抽取公共 SCSS**：将旧版 `index.html` 中的全局渐变背景、`.card` 动画、卡片阴影等抽取到 `web/src/assets/main.scss` 中。

### Phase 2: 核心框架搭建 (Core Flow)
- [x] **封装 Axios 网络请求 (`web/src/utils/request.ts`)**：
  - 设置统一的 `baseURL` (开发态下依赖 Vite 代理，生产态填 `/`)。
  - **响应拦截器**: 统一捕捉后端的非 2xx/200 报错，调用 `SweetAlert2` 弹出统一全局错误提示框。
- [x] **搭建中心路由 (`web/src/router/index.ts`)**：
  - 配置主路由体系。由于各个端在物理意义上应是分离的，暂时可作单页路由拆分：
    - `/` (导航页) -> 指引去前端的三个子模块。
    - `/patient` (患者挂号终端)
    - `/doctor` (医生叫号工作台)
    - `/screen` (大厅排队显示屏)

### Phase 3: 业务视图拆分与开发 (Views)
- [x] **重建『患者挂号终端』 (`web/src/views/patient/index.vue`)**：
  - 将旧的患者信息登记、挂号表单抽离过来。
  - 需要使用 `ref` 或 `reactive` 绑定表单双向数据。
  - 实现挂号成功后的弹窗（SweetAlert2）。
- [x] **重建『医生叫号工作台』 (`web/src/views/doctor/index.vue`)**：
  - 显示所有当班医生卡片及接诊数。
  - 发送 `/api/doctors/{id}/call_next` API 请求，实现叫号，并大字弹窗显示信息。
- [x] **重建『大厅排队显示屏』 (`web/src/views/screen/index.vue`)**：
  - 黑夜/科幻面板风格（纯展示）。
  - 按优先级（Emergency -> Urgent -> Normal -> 号码）自动排序队列。
  - 加入 `setInterval` 按固定频率拉取最新的等待队列。

### Phase 4: 后端联调与部署 (Build & Deploy)
- [x] **清理旧代码**：将原来的 `hospital-system/public/index.html` 备份或删除，以免和打包后的工程冲突。
- [x] **联调测试**：后端 C++ 控制台跑在 `8080`，前端 在 `web` 目录运行 `npm run dev` 跑在 `5173`。
- [x] **编译发布**：执行 `npm run build`，验收 Vite 是否正确地把编译产物存入了根目录的 `public` 文件夹，C++ 托管服务能否直接打开静态分发页面。

---
📝 **给 AI 的提示词 (Prompt)**：
"前端 Vue3 工程已经在 `web/` 目录下建好了，各种依赖也初始化了，请直接帮我从 **Phase 1** 和 **Phase 2** 的任务开始执行。"