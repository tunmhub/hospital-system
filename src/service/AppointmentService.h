#pragma once

#include "service/IAppointmentService.h"
#include "repository/IPatientRepository.h"
#include "repository/IDoctorRepository.h"
#include "repository/IAppointmentRepository.h"
#include "algorithm/PriorityQueueStrategy.h"

#include <memory>
#include <mutex>
#include <unordered_map>

namespace hospital {

/// 挂号业务服务实现
///
/// 职责：
/// 1. 校验患者和医生是否存在
/// 2. 检查医生接诊上限
/// 3. 原子操作：医生 current_patients + 1，生成挂号记录
/// 4. 自动分流（贪心最小堆）
/// 5. 优先队列叫号
/// 6. 等待时间预估
class AppointmentService : public IAppointmentService {
public:
    AppointmentService(
        std::shared_ptr<IPatientRepository> patientRepo,
        std::shared_ptr<IDoctorRepository> doctorRepo,
        std::shared_ptr<IAppointmentRepository> appointmentRepo
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

private:
    /// 内部挂号逻辑（不加锁，由调用方负责锁）
    Result<Appointment> makeAppointmentInternal(
        int64_t patient_id, int64_t doctor_id, Priority priority);

    std::shared_ptr<IPatientRepository> patientRepo_;
    std::shared_ptr<IDoctorRepository> doctorRepo_;
    std::shared_ptr<IAppointmentRepository> appointmentRepo_;
    std::mutex serviceMutex_;
};

} // namespace hospital
