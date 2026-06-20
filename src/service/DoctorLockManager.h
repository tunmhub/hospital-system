#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace hospital {

/// 医生级细粒度锁管理器（线程安全）
///
/// 多个 Service 共享同一实例，确保对同一医生的操作互斥。
/// 通过 DI 容器以单例注入各 Service。
class DoctorLockManager {
public:
    /// 获取指定医生的锁（按需创建，线程安全）
    std::mutex& getLock(int64_t doctor_id) {
        std::lock_guard<std::mutex> guard(mapMutex_);
        auto it = locks_.find(doctor_id);
        if (it == locks_.end()) {
            auto lock = std::make_unique<std::mutex>();
            auto* ptr = lock.get();
            locks_[doctor_id] = std::move(lock);
            return *ptr;
        }
        return *it->second;
    }

private:
    std::mutex mapMutex_;
    std::unordered_map<int64_t, std::unique_ptr<std::mutex>> locks_;
};

} // namespace hospital
