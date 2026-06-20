#include "service/AppointmentService.h"
#include "common/Exception.h"

#include <sstream>
#include <vector>

namespace hospital {

AppointmentService::AppointmentService(
    std::shared_ptr<IPatientRepository> patientRepo,
    std::shared_ptr<IDoctorRepository> doctorRepo,
    std::shared_ptr<IAppointmentRepository> appointmentRepo,
    std::shared_ptr<IAutoRoutingService> routingService,
    std::shared_ptr<DoctorLockManager> lockManager)
    : patientRepo_(std::move(patientRepo))
    , doctorRepo_(std::move(doctorRepo))
    , appointmentRepo_(std::move(appointmentRepo))
    , routingService_(std::move(routingService))
    , lockManager_(std::move(lockManager)) {}

// ============================================================
// 内部挂号逻辑（不加锁，由调用方负责锁）
// ============================================================

Result<Appointment> AppointmentService::makeAppointmentInternal(
    int64_t patient_id, int64_t doctor_id, Priority priority) {

    // 1. 检查患者是否存在
    auto patient = patientRepo_->findById(patient_id);
    if (!patient) {
        std::ostringstream msg;
        msg << "患者不存在 (ID=" << patient_id << ")";
        return Result<Appointment>::failure(msg.str());
    }

    // 2. 检查医生是否存在
    auto doctor = doctorRepo_->findById(doctor_id);
    if (!doctor) {
        std::ostringstream msg;
        msg << "医生不存在 (ID=" << doctor_id << ")";
        return Result<Appointment>::failure(msg.str());
    }

    // 3. 检查医生当前接诊人数是否达到上限
    if (doctor->current_patients >= doctor->max_patients) {
        std::ostringstream msg;
        msg << "医生 " << doctor->name << " 当前接诊人数已达上限 ("
            << doctor->current_patients << "/" << doctor->max_patients << ")";
        return Result<Appointment>::failure(msg.str());
    }

    // 4. 生成挂号记录
    Appointment apt;
    apt.patient_id   = patient_id;
    apt.doctor_id    = doctor_id;
    apt.status       = AppointmentStatus::Waiting;
    apt.priority     = priority;
    apt.queue_number = appointmentRepo_->getNextQueueNumber(doctor_id);

    if (!appointmentRepo_->save(apt)) {
        return Result<Appointment>::failure("挂号记录保存失败");
    }

    // 5. 更新医生当前接诊人数
    doctor->current_patients++;
    if (!doctorRepo_->update(*doctor)) {
        appointmentRepo_->remove(apt.id);
        return Result<Appointment>::failure("更新医生接诊人数失败");
    }

    return Result<Appointment>::success(apt);
}

// ============================================================
// 核心挂号 CRUD（加锁后委托给内部方法）
// ============================================================

Result<Appointment> AppointmentService::makeAppointment(
    int64_t patient_id, int64_t doctor_id, Priority priority) {
    std::lock_guard<std::mutex> lock(lockManager_->getLock(doctor_id));
    return makeAppointmentInternal(patient_id, doctor_id, priority);
}

Result<void> AppointmentService::cancelAppointment(int64_t appointment_id) {
    auto apt = appointmentRepo_->findById(appointment_id);
    if (!apt) {
        return Result<void>::failure("挂号记录不存在");
    }

    std::lock_guard<std::mutex> lock(lockManager_->getLock(apt->doctor_id));

    if (apt->status == AppointmentStatus::Cancelled) {
        return Result<void>::failure("该挂号已取消");
    }

    if (apt->status == AppointmentStatus::Completed) {
        return Result<void>::failure("该挂号已完成，无法取消");
    }

    apt->status = AppointmentStatus::Cancelled;
    appointmentRepo_->update(*apt);

    auto doctor = doctorRepo_->findById(apt->doctor_id);
    if (doctor && doctor->current_patients > 0) {
        doctor->current_patients--;
        doctorRepo_->update(*doctor);
    }

    return Result<void>::success();
}

Result<std::vector<Appointment>> AppointmentService::getAppointmentsByPatient(int64_t patient_id) {
    auto patient = patientRepo_->findById(patient_id);
    if (!patient) {
        std::ostringstream msg;
        msg << "患者不存在 (ID=" << patient_id << ")";
        return Result<std::vector<Appointment>>::failure(msg.str());
    }

    auto appointments = appointmentRepo_->findByPatient(patient_id);
    return Result<std::vector<Appointment>>::success(std::move(appointments));
}

Result<std::vector<Appointment>> AppointmentService::getQueueByDoctor(int64_t doctor_id) {
    auto doctor = doctorRepo_->findById(doctor_id);
    if (!doctor) {
        std::ostringstream msg;
        msg << "医生不存在 (ID=" << doctor_id << ")";
        return Result<std::vector<Appointment>>::failure(msg.str());
    }

    auto appointments = appointmentRepo_->findByDoctor(doctor_id);
    return Result<std::vector<Appointment>>::success(std::move(appointments));
}

Result<void> AppointmentService::completeAppointment(int64_t appointment_id) {
    auto apt = appointmentRepo_->findById(appointment_id);
    if (!apt) {
        return Result<void>::failure("挂号记录不存在");
    }

    std::lock_guard<std::mutex> lock(lockManager_->getLock(apt->doctor_id));

    if (apt->status != AppointmentStatus::InProgress) {
        return Result<void>::failure("该挂号不在就诊状态");
    }

    apt->status = AppointmentStatus::Completed;
    appointmentRepo_->update(*apt);

    // 医生接诊人数减 1
    auto doctor = doctorRepo_->findById(apt->doctor_id);
    if (doctor && doctor->current_patients > 0) {
        doctor->current_patients--;
        doctorRepo_->update(*doctor);
    }

    return Result<void>::success();
}

// ============================================================
// 自动分流挂号（委托给 IAutoRoutingService 选医生，自己负责锁定+挂号）
// ============================================================

Result<Appointment> AppointmentService::autoRouteAppointment(
    int64_t patient_id, std::string_view department, Priority priority) {

    // 1. 检查患者是否存在
    auto patient = patientRepo_->findById(patient_id);
    if (!patient) {
        std::ostringstream msg;
        msg << "患者不存在 (ID=" << patient_id << ")";
        return Result<Appointment>::failure(msg.str());
    }

    // 2. 委托给分流服务选择医生
    auto doctorId = routingService_->selectDoctor(department);
    if (!doctorId) {
        return Result<Appointment>::failure(
            std::string("科室 \"") + std::string(department) + "\" 没有可用医生或已满");
    }

    // 3. 锁定目标医生并执行挂号
    std::lock_guard<std::mutex> lock(lockManager_->getLock(*doctorId));
    return makeAppointmentInternal(patient_id, *doctorId, priority);
}

// ============================================================
// 委托给 IQueueManagementService（通过懒加载避免循环依赖）
// ============================================================

Result<Appointment> AppointmentService::callNextPatient(int64_t doctor_id) {
    if (!queueManager_) {
        return Result<Appointment>::failure("排队管理服务未配置");
    }
    return queueManager_->callNextPatient(doctor_id);
}

Result<int> AppointmentService::estimateWaitTime(int64_t appointment_id) {
    if (!queueManager_) {
        return Result<int>::failure("排队管理服务未配置");
    }
    return queueManager_->estimateWaitTime(appointment_id);
}

} // namespace hospital
