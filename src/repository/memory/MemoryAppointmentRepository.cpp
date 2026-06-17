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

int MemoryAppointmentRepository::getNextQueueNumber() {
    return queue_counter_.fetch_add(1);
}

} // namespace hospital
