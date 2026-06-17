#pragma once

#include "repository/IPatientRepository.h"
#include <mutex>
#include <unordered_map>
#include <vector>

namespace hospital {

/// 患者仓储的内存实现（用于开发和测试）
class MemoryPatientRepository : public IPatientRepository {
public:
    std::optional<Patient> findById(int64_t id) override;
    std::vector<Patient> findAll() override;
    bool save(Patient& entity) override;
    bool update(const Patient& entity) override;
    bool remove(int64_t id) override;

    std::optional<Patient> findByPhone(std::string_view phone) override;
    std::optional<Patient> findByIdCard(std::string_view id_card) override;

private:
    std::unordered_map<int64_t, Patient> data_;   ///< id → Patient
    int64_t next_id_ = 1;                          ///< 自增 ID 计数器
    mutable std::mutex mutex_;                      ///< 线程安全锁
};

} // namespace hospital
