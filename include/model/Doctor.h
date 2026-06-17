#pragma once

#include <string>

namespace hospital {

/// 医生数据模型
struct Doctor {
    int64_t id = 0;              ///< 医生 ID（数据库自增主键）
    std::string name;            ///< 姓名
    std::string department;      ///< 所属科室
    std::string title;           ///< 职称（主任医师/副主任医师/主治医师/住院医师）
    std::string work_start;      ///< 排班开始时段（如 "08:00"）
    std::string work_end;        ///< 排班结束时段（如 "17:00"）
    int max_patients = 0;        ///< 每日最大接诊量
    int current_patients = 0;    ///< 当前已接诊人数
};

} // namespace hospital
