#pragma once

#include "service/IQueueManagementService.h"
#include "repository/IAppointmentRepository.h"
#include "repository/IDoctorRepository.h"
#include "algorithm/IQueueStrategy.h"
#include "service/DoctorLockManager.h"

#include <memory>

namespace hospital {

/// 排队管理服务实现
///
/// 从 AppointmentService 中提取，职责：
/// 1. 呼叫下一位患者（优先队列调度）
/// 2. 预估等待时间（排队算法）
class QueueManagementService : public IQueueManagementService {
public:
    QueueManagementService(
        std::shared_ptr<IAppointmentRepository> appointmentRepo,
        std::shared_ptr<IDoctorRepository> doctorRepo,
        std::shared_ptr<IQueueStrategy> queueStrategy,
        std::shared_ptr<DoctorLockManager> lockManager
    );

    Result<Appointment> callNextPatient(int64_t doctor_id) override;
    Result<int> estimateWaitTime(int64_t appointment_id) override;

private:
    std::shared_ptr<IAppointmentRepository> appointmentRepo_;
    std::shared_ptr<IDoctorRepository> doctorRepo_;
    std::shared_ptr<IQueueStrategy> queueStrategy_;
    std::shared_ptr<DoctorLockManager> lockManager_;
};

} // namespace hospital
