#pragma once

#include "repository/IPatientRepository.h"
#include "db/MySQLConnectionPool.h"

#include <memory>

namespace hospital {

/// 患者仓储的 MySQL 实现
class MySQLPatientRepository : public IPatientRepository {
public:
    /// 通过依赖注入获取连接池
    explicit MySQLPatientRepository(std::shared_ptr<MySQLConnectionPool> pool);

    std::optional<Patient> findById(int64_t id) override;
    std::vector<Patient> findAll() override;
    bool save(Patient& entity) override;
    bool update(const Patient& entity) override;
    bool remove(int64_t id) override;

    std::optional<Patient> findByPhone(std::string_view phone) override;
    std::optional<Patient> findByIdCard(std::string_view id_card) override;

private:
    /// 从 MYSQL_RES 的当前行解析出一个 Patient 对象
    Patient parseRow(MYSQL_ROW row);

    /// 对字符串进行 MySQL 转义（防 SQL 注入）
    std::string escape(std::shared_ptr<MYSQL> conn, std::string_view input);

    std::shared_ptr<MySQLConnectionPool> pool_;
};

} // namespace hospital
