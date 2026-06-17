#pragma once

#include <string>

namespace hospital {

/// 挂号状态枚举
enum class AppointmentStatus {
    Waiting,    ///< 等待就诊
    InProgress, ///< 就诊中
    Completed,  ///< 已完成
    Cancelled   ///< 已取消
};

/// 优先级枚举（用于急诊插队）
enum class Priority {
    Normal = 0,    ///< 普通
    Urgent = 1,    ///< 加急
    Emergency = 2  ///< 急诊
};

/// 将状态转为字符串（用于 JSON 序列化和日志）
inline const char* statusToString(AppointmentStatus s) {
    switch (s) {
        case AppointmentStatus::Waiting:    return "waiting";
        case AppointmentStatus::InProgress: return "in_progress";
        case AppointmentStatus::Completed:  return "completed";
        case AppointmentStatus::Cancelled:  return "cancelled";
    }
    return "unknown";
}

/// 将优先级转为字符串
inline const char* priorityToString(Priority p) {
    switch (p) {
        case Priority::Normal:    return "normal";
        case Priority::Urgent:    return "urgent";
        case Priority::Emergency: return "emergency";
    }
    return "unknown";
}

/// 挂号数据模型
struct Appointment {
    int64_t id = 0;                ///< 挂号 ID
    int64_t patient_id = 0;        ///< 患者 ID
    int64_t doctor_id = 0;         ///< 医生 ID
    AppointmentStatus status = AppointmentStatus::Waiting;
    Priority priority = Priority::Normal;
    int queue_number = 0;          ///< 排队号码
    double registration_fee = 0.0; ///< 挂号费
    double insurance_fee = 0.0;    ///< 医保报销金额
    double self_fee = 0.0;         ///< 自费金额
    bool settled = false;          ///< 是否已结算
    std::string created_at;        ///< 挂号时间
};

} // namespace hospital
