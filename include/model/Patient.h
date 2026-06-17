#pragma once

#include <string>
#include <ctime>

namespace hospital {

/// 患者数据模型
struct Patient {
    int64_t id = 0;              ///< 患者 ID（数据库自增主键）
    std::string name;            ///< 姓名
    std::string phone;           ///< 手机号
    std::string id_card;         ///< 身份证号
    int age = 0;                 ///< 年龄
    std::string gender;          ///< 性别（"男" / "女"）
    std::string created_at;      ///< 创建时间（ISO 8601 格式）
};

} // namespace hospital
