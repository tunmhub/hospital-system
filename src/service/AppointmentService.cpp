#include "service/AppointmentService.h"
#include "common/Exception.h"

#include <sstream>
#include <vector>

namespace hospital {

AppointmentService::AppointmentService(
    std::shared_ptr<IPatientRepository> patientRepo,
    std::shared_ptr<IDoctorRepository> doctorRepo,
    std::shared_ptr<IAppointmentRepository> appointmentRepo)
    : patientRepo_(std::move(patientRepo))
    , doctorRepo_(std::move(doctorRepo))
    , appointmentRepo_(std::move(appointmentRepo)) {}

// ============================================================
// 医生级锁管理
// ============================================================

std::mutex& AppointmentService::getDoctorLock(int64_t doctor_id) {
    std::lock_guard<std::mutex> guard(lockMapMutex_);
    auto it = doctorLocks_.find(doctor_id);
    if (it == doctorLocks_.end()) {
        auto lock = std::make_unique<std::mutex>();
        auto* ptr = lock.get();
        doctorLocks_[doctor_id] = std::move(lock);
        return *ptr;
    }
    return *it->second;
}

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
// 公开接口（加锁后委托给内部方法）
// ============================================================

Result<Appointment> AppointmentService::makeAppointment(
    int64_t patient_id, int64_t doctor_id, Priority priority) {
    std::lock_guard<std::mutex> lock(getDoctorLock(doctor_id));
    return makeAppointmentInternal(patient_id, doctor_id, priority);
}

Result<void> AppointmentService::cancelAppointment(int64_t appointment_id) {
    auto apt = appointmentRepo_->findById(appointment_id);
    if (!apt) {
        return Result<void>::failure("挂号记录不存在");
    }

    std::lock_guard<std::mutex> lock(getDoctorLock(apt->doctor_id));

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

// ============================================================
// 自动分流挂号（贪心最小堆算法）
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

    // 2. 获取该科室所有医生
    auto doctors = doctorRepo_->findByDepartment(department);
    if (doctors.empty()) {
        std::ostringstream msg;
        msg << "科室 \"" << department << "\" 没有可用医生";
        return Result<Appointment>::failure(msg.str());
    }

    // 3. 最小堆贪心：按 current_patients 从小到大排序
    struct DoctorLoad {
        Doctor doctor;
        int load;
    };
    auto cmp = [](const DoctorLoad& a, const DoctorLoad& b) {
        return a.load > b.load;  // 最小堆
    };
    std::priority_queue<DoctorLoad, std::vector<DoctorLoad>, decltype(cmp)> minHeap(cmp);

    for (const auto& doc : doctors) {
        minHeap.push({doc, doc.current_patients});
    }

    // 4. 贪心选择：从堆顶取出负载最低的医生，获取该医生的锁后执行挂号
    while (!minHeap.empty()) {
        auto top = minHeap.top();
        minHeap.pop();

        if (top.load < top.doctor.max_patients) {
            // 获取目标医生的锁，然后执行挂号
            std::lock_guard<std::mutex> lock(getDoctorLock(top.doctor.id));
            return makeAppointmentInternal(patient_id, top.doctor.id, priority);
        }
    }

    return Result<Appointment>::failure(
        std::string("科室 \"") + std::string(department) + "\" 所有医生已满");
}

// ============================================================
// 呼叫下一位患者（数据库驱动）
// ============================================================

Result<Appointment> AppointmentService::callNextPatient(int64_t doctor_id) {
    std::lock_guard<std::mutex> lock(getDoctorLock(doctor_id));

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

    // 使用优先队列选择最高优先级的患者
    PriorityQueueStrategy queue;
    queue.enqueueBatch(waiting);
    auto next = queue.dequeue();
    if (!next) {
        return Result<Appointment>::failure("当前没有等待中的患者");
    }

    // 更新数据库状态为就诊中（InProgress）
    next->status = AppointmentStatus::InProgress;
    appointmentRepo_->update(*next);

    return Result<Appointment>::success(*next);
}

// ============================================================
// 完成就诊
// ============================================================

Result<void> AppointmentService::completeAppointment(int64_t appointment_id) {
    auto apt = appointmentRepo_->findById(appointment_id);
    if (!apt) {
        return Result<void>::failure("挂号记录不存在");
    }

    std::lock_guard<std::mutex> lock(getDoctorLock(apt->doctor_id));

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
// 预估等待时间
// ============================================================

Result<int> AppointmentService::estimateWaitTime(int64_t appointment_id) {
    auto apt = appointmentRepo_->findById(appointment_id);
    if (!apt) {
        return Result<int>::failure("挂号记录不存在");
    }

    std::lock_guard<std::mutex> lock(getDoctorLock(apt->doctor_id));

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

    // 使用优先队列计算排在该患者前面的人数
    PriorityQueueStrategy tempQueue;
    tempQueue.enqueueBatch(waiting);

    int aheadCount = 0;
    constexpr int MINUTES_PER_PATIENT = 10;

    while (!tempQueue.empty()) {
        auto current = tempQueue.dequeue();
        if (!current) break;

        if (current->id == appointment_id) {
            return Result<int>::success(aheadCount * MINUTES_PER_PATIENT);
        }
        aheadCount++;
    }

    return Result<int>::success(aheadCount * MINUTES_PER_PATIENT);
}

} // namespace hospital
