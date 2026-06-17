#pragma once

#include "repository/IRepository.h"
#include "model/Doctor.h"
#include <vector>
#include <string>

namespace hospital {

/// 医生仓储接口
class IDoctorRepository : public IRepository<Doctor> {
public:
    ~IDoctorRepository() override = default;

    /// 根据科室查找医生列表
    virtual std::vector<Doctor> findByDepartment(std::string_view department) = 0;

    /// 查找当前负载最低的医生（用于排班分流）
    virtual std::optional<Doctor> findLeastLoaded(std::string_view department) = 0;
};

} // namespace hospital
