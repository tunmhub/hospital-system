#include "service/QueueManagementService.h"

#include <sstream>
#include <vector>

namespace hospital {

QueueManagementService::QueueManagementService(
    std::shared_ptr<IAppointmentRepository> appointmentRepo,
    std::shared_ptr<IDoctorRepository> doctorRepo,
    std::shared_ptr<IQueueStrategy> queueStrategy,
    std::shared_ptr<DoctorLockManager> lockManager)
    : appointmentRepo_(std::move(appointmentRepo))
    , doctorRepo_(std::move(doctorRepo))
    , queueStrategy_(std::move(queueStrategy))
    , lockManager_(std::move(lockManager)) {}

// ============================================================
// 呼叫下一位患者（从 AppointmentService 迁移）
// ============================================================

Result<Appointment> QueueManagementService::callNextPatient(int64_t doctor_id) {
    std::lock_guard<std::mutex> lock(lockManager_->getLock(doctor_id));

    auto doctor = doctorRepo_->findById(doctor_id);
    if (!doctor) {
        std::ostringstream msg;
        msg << "医生不存在 (ID=" << doctor_id << ")";
        return Result<Appointment>::failure(msg.str());
    }

    // 从数据库查询该医生所有等待中的挂号
    auto waitingList = appointmentRepo_->findByDoctor(doctor_id);
    std::vector<Appointment> waiting;
    for (const auto& apt : waitingList) {
        if (apt.status == AppointmentStatus::Waiting) {
            waiting.push_back(apt);
        }
    }

    if (waiting.empty()) {
        return Result<Appointment>::failure("当前没有等待中的患者");
    }

    // 使用注入的队列策略选择最高优先级的患者
    queueStrategy_->clear();
    for (const auto& apt : waiting) {
        queueStrategy_->enqueue(apt);
    }
    auto next = queueStrategy_->dequeue();
    if (!next) {
        return Result<Appointment>::failure("当前没有等待中的患者");
    }

    // 更新数据库状态为就诊中（InProgress）
    next->status = AppointmentStatus::InProgress;
    appointmentRepo_->update(*next);

    return Result<Appointment>::success(*next);
}

// ============================================================
// 预估等待时间（从 AppointmentService 迁移）
// ============================================================

Result<int> QueueManagementService::estimateWaitTime(int64_t appointment_id) {
    auto apt = appointmentRepo_->findById(appointment_id);
    if (!apt) {
        return Result<int>::failure("挂号记录不存在");
    }

    std::lock_guard<std::mutex> lock(lockManager_->getLock(apt->doctor_id));

    if (apt->status != AppointmentStatus::Waiting) {
        return Result<int>::failure("该挂号不在等待状态");
    }

    // 获取该医生所有等待中的挂号
    auto waitingList = appointmentRepo_->findByDoctor(apt->doctor_id);
    std::vector<Appointment> waiting;
    for (const auto& a : waitingList) {
        if (a.status == AppointmentStatus::Waiting) {
            waiting.push_back(a);
        }
    }

    // 使用注入的队列策略计算排在该患者前面的人数
    queueStrategy_->clear();
    for (const auto& a : waiting) {
        queueStrategy_->enqueue(a);
    }

    int aheadCount = 0;
    constexpr int MINUTES_PER_PATIENT = 10;

    while (!queueStrategy_->empty()) {
        auto current = queueStrategy_->dequeue();
        if (!current) break;

        if (current->id == appointment_id) {
            return Result<int>::success(aheadCount * MINUTES_PER_PATIENT);
        }
        aheadCount++;
    }

    return Result<int>::success(aheadCount * MINUTES_PER_PATIENT);
}

} // namespace hospital
