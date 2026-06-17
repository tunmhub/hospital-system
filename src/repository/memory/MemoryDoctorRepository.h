#pragma once

#include "repository/IDoctorRepository.h"
#include <mutex>
#include <unordered_map>
#include <vector>

namespace hospital {

/// 医生仓储的内存实现（用于开发和测试）
class MemoryDoctorRepository : public IDoctorRepository {
public:
    std::optional<Doctor> findById(int64_t id) override;
    std::vector<Doctor> findAll() override;
    bool save(Doctor& entity) override;
    bool update(const Doctor& entity) override;
    bool remove(int64_t id) override;

    std::vector<Doctor> findByDepartment(std::string_view department) override;
    std::optional<Doctor> findLeastLoaded(std::string_view department) override;

private:
    std::unordered_map<int64_t, Doctor> data_;
    int64_t next_id_ = 1;
    mutable std::mutex mutex_;
};

} // namespace hospital
