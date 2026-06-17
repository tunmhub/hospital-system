#pragma once

#include <optional>
#include <vector>
#include <cstdint>

namespace hospital {

/// 通用仓储接口（模板基类）
/// 定义标准 CRUD 操作，具体实现可切换 MySQL / Memory / SQLite
///
/// @tparam Entity  实体类型（如 Patient, Doctor）
/// @tparam Id      主键类型（通常为 int64_t）
template<typename Entity, typename Id = int64_t>
class IRepository {
public:
    virtual ~IRepository() = default;

    /// 根据 ID 查找
    virtual std::optional<Entity> findById(Id id) = 0;

    /// 查询所有记录
    virtual std::vector<Entity> findAll() = 0;

    /// 保存新记录（成功后 entity.id 应被赋值）
    virtual bool save(Entity& entity) = 0;

    /// 更新已有记录
    virtual bool update(const Entity& entity) = 0;

    /// 根据 ID 删除
    virtual bool remove(Id id) = 0;
};

} // namespace hospital
