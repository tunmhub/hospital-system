#include "repository/memory/MemoryDoctorRepository.h"
#include <limits>

namespace hospital {

std::optional<Doctor> MemoryDoctorRepository::findById(int64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(id);
    if (it != data_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<Doctor> MemoryDoctorRepository::findAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Doctor> result;
    result.reserve(data_.size());
    for (const auto& [id, doctor] : data_) {
        result.push_back(doctor);
    }
    return result;
}

bool MemoryDoctorRepository::save(Doctor& entity) {
    std::lock_guard<std::mutex> lock(mutex_);
    entity.id = next_id_++;
    data_[entity.id] = entity;
    return true;
}

bool MemoryDoctorRepository::update(const Doctor& entity) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(entity.id);
    if (it == data_.end()) {
        return false;
    }
    it->second = entity;
    return true;
}

bool MemoryDoctorRepository::remove(int64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.erase(id) > 0;
}

std::vector<Doctor> MemoryDoctorRepository::findByDepartment(std::string_view department) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Doctor> result;
    for (const auto& [id, doctor] : data_) {
        if (doctor.department == department) {
            result.push_back(doctor);
        }
    }
    return result;
}

std::optional<Doctor> MemoryDoctorRepository::findLeastLoaded(std::string_view department) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::optional<Doctor> best;
    int min_load = std::numeric_limits<int>::max();

    for (const auto& [id, doctor] : data_) {
        if (doctor.department == department) {
            int load = doctor.current_patients;
            if (load < min_load) {
                min_load = load;
                best = doctor;
            }
        }
    }
    return best;
}

} // namespace hospital
