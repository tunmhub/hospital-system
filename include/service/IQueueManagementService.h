#pragma once

#include "common/Result.h"
#include "model/Appointment.h"
#include <cstdint>

namespace hospital {

/// 排队管理服务接口
///
/// 职责：叫号调度、等待时间预估
class IQueueManagementService {
public:
    virtual ~IQueueManagementService() = default;

    /// 呼叫下一位患者：从优先队列中弹出最高优先级患者，标记为就诊中
    virtual Result<Appointment> callNextPatient(int64_t doctor_id) = 0;

    /// 预估等待时间（分钟）：计算该患者前面还有多少人排队 × 10 分钟
    virtual Result<int> estimateWaitTime(int64_t appointment_id) = 0;
};

} // namespace hospital
