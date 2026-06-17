#pragma once

#include <string>
#include <ctime>

namespace hospital {

/// 医保类型枚举
enum class InsuranceType {
    UrbanWorker,    ///< 城镇职工医保（报销80%）
    UrbanResident,  ///< 城镇居民医保（报销60%）
    RuralResident,  ///< 新农合医保（报销60%）
    Self            ///< 自费（无报销）
};

/// 将医保类型转为字符串
inline const char* insuranceTypeToString(InsuranceType type) {
    switch (type) {
        case InsuranceType::UrbanWorker:   return "urban_worker";
        case InsuranceType::UrbanResident: return "urban_resident";
        case InsuranceType::RuralResident: return "rural_resident";
        case InsuranceType::Self:          return "self";
    }
    return "self";
}

/// 将字符串转为医保类型
inline InsuranceType stringToInsuranceType(const char* s) {
    std::string str(s);
    if (str == "urban_worker")   return InsuranceType::UrbanWorker;
    if (str == "urban_resident") return InsuranceType::UrbanResident;
    if (str == "rural_resident") return InsuranceType::RuralResident;
    return InsuranceType::Self;
}

/// 获取医保报销比例
inline double getInsuranceReimbursementRate(InsuranceType type) {
    switch (type) {
        case InsuranceType::UrbanWorker:   return 0.80;  // 80%
        case InsuranceType::UrbanResident: return 0.60;  // 60%
        case InsuranceType::RuralResident: return 0.60;  // 60%
        case InsuranceType::Self:          return 0.00;  // 0%
    }
    return 0.00;
}

/// 患者数据模型
struct Patient {
    int64_t id = 0;              ///< 患者 ID（数据库自增主键）
    std::string name;            ///< 姓名
    std::string medical_record_no;  ///< 病历号（如 MR20260001）
    std::string phone;           ///< 手机号
    std::string id_card;         ///< 身份证号
    int age = 0;                 ///< 年龄
    std::string gender;          ///< 性别（"男" / "女"）
    InsuranceType insurance_type = InsuranceType::Self;  ///< 医保类型
    std::string created_at;      ///< 创建时间（ISO 8601 格式）
};

} // namespace hospital
