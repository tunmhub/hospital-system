#pragma once

#include "repository/IRepository.h"
#include "model/Patient.h"
#include <optional>
#include <string>
#include <vector>

namespace hospital {

/// 患者仓储接口
class IPatientRepository : public IRepository<Patient> {
public:
    ~IPatientRepository() override = default;

    /// 根据手机号查找患者
    virtual std::optional<Patient> findByPhone(std::string_view phone) = 0;

    /// 根据身份证号查找患者
    virtual std::optional<Patient> findByIdCard(std::string_view id_card) = 0;

    /// 根据姓名模糊搜索患者
    virtual std::vector<Patient> searchByName(std::string_view keyword) = 0;
};

} // namespace hospital
