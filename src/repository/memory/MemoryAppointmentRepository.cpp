#include "repository/memory/MemoryAppointmentRepository.h"

namespace hospital {

std::optional<Appointment> MemoryAppointmentRepository::findById(int64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(id);
    if (it != data_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<Appointment> MemoryAppointmentRepository::findAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Appointment> result;
    result.reserve(data_.size());
    for (const auto& [id, apt] : data_) {
        result.push_back(apt);
    }
    return result;
}

bool MemoryAppointmentRepository::save(Appointment& entity) {
    std::lock_guard<std::mutex> lock(mutex_);
    entity.id = next_id_++;
    data_[entity.id] = entity;
    return true;
}

bool MemoryAppointmentRepository::update(const Appointment& entity) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(entity.id);
    if (it == data_.end()) {
        return false;
    }
    it->second = entity;
    return true;
}

bool MemoryAppointmentRepository::remove(int64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.erase(id) > 0;
}

std::vector<Appointment> MemoryAppointmentRepository::findByDoctor(int64_t doctor_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Appointment> result;
    for (const auto& [id, apt] : data_) {
        if (apt.doctor_id == doctor_id) {
            result.push_back(apt);
        }
    }
    return result;
}

std::vector<Appointment> MemoryAppointmentRepository::findByPatient(int64_t patient_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Appointment> result;
    for (const auto& [id, apt] : data_) {
        if (apt.patient_id == patient_id) {
            result.push_back(apt);
        }
    }
    return result;
}

size_t MemoryAppointmentRepository::countWaitingByDoctor(int64_t doctor_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [id, apt] : data_) {
        if (apt.doctor_id == doctor_id && apt.status == AppointmentStatus::Waiting) {
            ++count;
        }
    }
    return count;
}

int MemoryAppointmentRepository::getNextQueueNumber(int64_t doctor_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return ++doctor_queue_counters_[doctor_id];
}

std::vector<DeptStat> MemoryAppointmentRepository::countWaitingByDepartment() {
    std::lock_guard<std::mutex> lock(mutex_);

    // 按科室统计候诊人数
    std::unordered_map<std::string, int> dept_counts;

    for (const auto& [id, apt] : data_) {
        if (apt.status == AppointmentStatus::Waiting) {
            // 通过医生仓储获取科室信息
            if (doctorRepo_) {
                auto doctor = doctorRepo_->findById(apt.doctor_id);
                if (doctor) {
                    dept_counts[doctor->department]++;
                }
            }
        }
    }

    // 转换为 DeptStat 向量
    std::vector<DeptStat> stats;
    for (const auto& [dept, count] : dept_counts) {
        DeptStat stat;
        stat.department = dept;
        stat.waiting_count = count;
        stat.estimated_wait_minutes = count * 10;  // 每人 10 分钟
        stats.push_back(stat);
    }

    return stats;
}

} // namespace hospital
