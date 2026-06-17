#pragma once

#include "common/Result.h"
#include "model/Appointment.h"
#include <string>
#include <vector>
#include <cstdint>

namespace hospital {

/// 挂号业务服务接口
class IAppointmentService {
public:
    virtual ~IAppointmentService() = default;

    /// 创建挂号
    /// @param patient_id  患者 ID
    /// @param doctor_id   医生 ID
    /// @param priority    优先级
    /// @return 成功返回挂号信息，失败返回错误信息
    virtual Result<Appointment> makeAppointment(
        int64_t patient_id,
        int64_t doctor_id,
        Priority priority = Priority::Normal
    ) = 0;

    /// 取消挂号
    virtual Result<void> cancelAppointment(int64_t appointment_id) = 0;

    /// 查询患者的所有挂号记录
    virtual Result<std::vector<Appointment>> getAppointmentsByPatient(int64_t patient_id) = 0;

    /// 查询医生的等待队列
    virtual Result<std::vector<Appointment>> getQueueByDoctor(int64_t doctor_id) = 0;

    /// 自动分流挂号：根据科室自动分配当前负载最低的医生
    virtual Result<Appointment> autoRouteAppointment(
        int64_t patient_id,
        std::string_view department,
        Priority priority = Priority::Normal
    ) = 0;

    /// 呼叫下一位患者：从优先队列中弹出最高优先级患者，标记为就诊中
    virtual Result<Appointment> callNextPatient(int64_t doctor_id) = 0;

    /// 完成就诊：将就诊中的挂号标记为已完成
    virtual Result<void> completeAppointment(int64_t appointment_id) = 0;

    /// 预估等待时间（分钟）：计算该患者前面还有多少人排队 × 10 分钟
    virtual Result<int> estimateWaitTime(int64_t appointment_id) = 0;
};

} // namespace hospital
