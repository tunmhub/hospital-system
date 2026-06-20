#pragma once

#include "service/IAppointmentService.h"
#include "service/IQueueManagementService.h"
#include "repository/IPatientRepository.h"
#include "repository/IDoctorRepository.h"
#include "repository/IAppointmentRepository.h"
#include "service/IAutoRoutingService.h"
#include "service/DoctorLockManager.h"

#include <memory>

namespace hospital {

/// 挂号业务服务实现（精简后）
///
/// 职责（核心挂号 CRUD）：
/// 1. 校验患者和医生是否存在
/// 2. 检查医生接诊上限
/// 3. 原子操作：医生 current_patients + 1，生成挂号记录
///
/// 分流调度委托给 IAutoRoutingService，
/// 叫号/等待时间委托给 IQueueManagementService（由 ApiController 直接调用）。
class AppointmentService : public IAppointmentService {
public:
    AppointmentService(
        std::shared_ptr<IPatientRepository> patientRepo,
        std::shared_ptr<IDoctorRepository> doctorRepo,
        std::shared_ptr<IAppointmentRepository> appointmentRepo,
        std::shared_ptr<IAutoRoutingService> routingService,
        std::shared_ptr<DoctorLockManager> lockManager
    );

    Result<Appointment> makeAppointment(
        int64_t patient_id,
        int64_t doctor_id,
        Priority priority
    ) override;

    Result<void> cancelAppointment(int64_t appointment_id) override;

    Result<std::vector<Appointment>> getAppointmentsByPatient(int64_t patient_id) override;

    Result<std::vector<Appointment>> getQueueByDoctor(int64_t doctor_id) override;

    Result<Appointment> autoRouteAppointment(
        int64_t patient_id,
        std::string_view department,
        Priority priority
    ) override;

    Result<Appointment> callNextPatient(int64_t doctor_id) override;

    Result<void> completeAppointment(int64_t appointment_id);

    Result<int> estimateWaitTime(int64_t appointment_id) override;

    /// 设置排队管理服务（延迟注入，打破循环依赖）
    void setQueueManagementService(std::shared_ptr<IQueueManagementService> queueManager) {
        queueManager_ = std::move(queueManager);
    }

private:
    /// 内部挂号逻辑（不加锁，由调用方负责锁）
    Result<Appointment> makeAppointmentInternal(
        int64_t patient_id, int64_t doctor_id, Priority priority);

    std::shared_ptr<IPatientRepository> patientRepo_;
    std::shared_ptr<IDoctorRepository> doctorRepo_;
    std::shared_ptr<IAppointmentRepository> appointmentRepo_;
    std::shared_ptr<IAutoRoutingService> routingService_;
    std::shared_ptr<DoctorLockManager> lockManager_;
    std::shared_ptr<IQueueManagementService> queueManager_;  ///< 延迟注入
};

} // namespace hospital
