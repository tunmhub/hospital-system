# 项目特定配置

## 文档自动更新规则

在完成以下类型的任务后，**必须自动检查并更新相关文档**：

### 需要更新文档的场景
1. **新增功能** - 更新 devlog.md、USAGE.md、README.md
2. **修复 Bug** - 更新 devlog.md
3. **架构变更** - 更新 devlog.md、README.md、ARCHITECTURE_DISCUSSION.md
4. **API 变更** - 更新 USAGE.md、devlog.md
5. **数据库变更** - 更新 devlog.md、README.md

### 文档更新检查清单
完成代码修改后，依次检查：
1. `devlog.md` - 是否需要添加新的日志条目？
2. `USAGE.md` - 使用说明是否需要更新？
3. `README.md` - 项目说明是否需要更新？
4. `IMPROVEMENTS.md` - 改进记录是否需要更新？

### 更新原则
- 在 devlog.md 开头添加新条目（最新的在最前面）
- 记录：日期、完成内容、修改的文件、验证结果
- 保持格式一致
