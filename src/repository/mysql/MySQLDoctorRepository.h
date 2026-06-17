#pragma once

#include "repository/IDoctorRepository.h"
#include "db/MySQLConnectionPool.h"

#include <memory>

namespace hospital {

/// 医生仓储的 MySQL 实现
class MySQLDoctorRepository : public IDoctorRepository {
public:
    explicit MySQLDoctorRepository(std::shared_ptr<MySQLConnectionPool> pool);

    std::optional<Doctor> findById(int64_t id) override;
    std::vector<Doctor> findAll() override;
    bool save(Doctor& entity) override;
    bool update(const Doctor& entity) override;
    bool remove(int64_t id) override;

    std::vector<Doctor> findByDepartment(std::string_view department) override;
    std::optional<Doctor> findLeastLoaded(std::string_view department) override;

private:
    Doctor parseRow(MYSQL_ROW row);
    std::string escape(std::shared_ptr<MYSQL> conn, std::string_view input);

    std::shared_ptr<MySQLConnectionPool> pool_;
};

} // namespace hospital
