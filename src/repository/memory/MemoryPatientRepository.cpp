#include "repository/memory/MemoryPatientRepository.h"

namespace hospital {

std::optional<Patient> MemoryPatientRepository::findById(int64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(id);
    if (it != data_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<Patient> MemoryPatientRepository::findAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Patient> result;
    result.reserve(data_.size());
    for (const auto& [id, patient] : data_) {
        result.push_back(patient);
    }
    return result;
}

bool MemoryPatientRepository::save(Patient& entity) {
    std::lock_guard<std::mutex> lock(mutex_);
    entity.id = next_id_++;
    data_[entity.id] = entity;
    return true;
}

bool MemoryPatientRepository::update(const Patient& entity) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(entity.id);
    if (it == data_.end()) {
        return false;
    }
    it->second = entity;
    return true;
}

bool MemoryPatientRepository::remove(int64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.erase(id) > 0;
}

std::optional<Patient> MemoryPatientRepository::findByPhone(std::string_view phone) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, patient] : data_) {
        if (patient.phone == phone) {
            return patient;
        }
    }
    return std::nullopt;
}

std::optional<Patient> MemoryPatientRepository::findByIdCard(std::string_view id_card) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, patient] : data_) {
        if (patient.id_card == id_card) {
            return patient;
        }
    }
    return std::nullopt;
}

std::vector<Patient> MemoryPatientRepository::searchByName(std::string_view keyword) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Patient> result;
    std::string kw(keyword);
    for (const auto& [id, patient] : data_) {
        if (patient.name.find(kw) != std::string::npos) {
            result.push_back(patient);
            if (result.size() >= 10) break;
        }
    }
    return result;
}

} // namespace hospital
