#pragma once

#include "repository/IAppointmentRepository.h"
#include <mutex>
#include <unordered_map>
#include <vector>
#include <atomic>

namespace hospital {

/// 挂号仓储的内存实现（用于开发和测试）
class MemoryAppointmentRepository : public IAppointmentRepository {
public:
    std::optional<Appointment> findById(int64_t id) override;
    std::vector<Appointment> findAll() override;
    bool save(Appointment& entity) override;
    bool update(const Appointment& entity) override;
    bool remove(int64_t id) override;

    std::vector<Appointment> findByDoctor(int64_t doctor_id) override;
    std::vector<Appointment> findByPatient(int64_t patient_id) override;
    size_t countWaitingByDoctor(int64_t doctor_id) override;
    int getNextQueueNumber(int64_t doctor_id) override;

private:
    std::unordered_map<int64_t, Appointment> data_;
    int64_t next_id_ = 1;
    std::unordered_map<int64_t, int> doctor_queue_counters_;  ///< 按医生分组的排队计数器
    mutable std::mutex mutex_;
};

} // namespace hospital
