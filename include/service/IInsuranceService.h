#pragma once

#include "model/Appointment.h"
#include "model/Patient.h"
#include "common/Result.h"

namespace hospital {

/// 医保结算服务接口
class IInsuranceService {
public:
    virtual ~IInsuranceService() = default;

    /// 计算挂号费用
    /// @param patient 患者信息（包含医保类型）
    /// @param priority 优先级（影响挂号费）
    /// @return 计算结果：挂号费、医保报销、自费金额
    virtual Result<Appointment> calculateFee(
        const Patient& patient,
        Priority priority
    ) = 0;

    /// 结算挂号
    /// @param appointment_id 挂号 ID
    /// @return 结算后的挂号信息
    virtual Result<Appointment> settleAppointment(int64_t appointment_id) = 0;
};

} // namespace hospital
