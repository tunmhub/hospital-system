#pragma once

#include "algorithm/IQueueStrategy.h"
#include "model/Appointment.h"

#include <queue>
#include <vector>
#include <mutex>
#include <optional>

namespace hospital {

/// 急诊优先队列策略
///
/// 比较规则（越小越优先）：
///   1. 优先级：Emergency(2) > Urgent(1) > Normal(0)
///      ——注意：priority_queue 默认是最大堆，所以我们反转比较，
///        使得 priority 值更大的排在前面（Emergency 优先出队）
///   2. 同优先级：queue_number 小的优先（先到先得）
///
/// 并发安全：所有操作使用 std::mutex 保护
class PriorityQueueStrategy : public IQueueStrategy {
public:
    void enqueue(const Appointment& apt) override;
    std::optional<Appointment> dequeue() override;
    std::optional<Appointment> peek() const override;
    size_t size() const override;
    bool empty() const override;
    void clear() override;

    /// 批量入队（从数据库加载时使用，减少锁的开销）
    void enqueueBatch(const std::vector<Appointment>& appointments);

private:
    /// 比较器：priority 值大的优先（Emergency=2 > Urgent=1 > Normal=0）
    /// 同优先级时 queue_number 小的优先
    struct AppointmentComparator {
        bool operator()(const Appointment& a, const Appointment& b) const {
            // priority_queue 默认 top() 返回"最大"元素
            // 我们希望 Emergency(2) 排在最前面，所以用 <
            // 即：priority 大的 > priority 小的
            if (a.priority != b.priority) {
                return static_cast<int>(a.priority) < static_cast<int>(b.priority);
            }
            // 同优先级：queue_number 小的优先（先到先得）
            return a.queue_number > b.queue_number;
        }
    };

    std::priority_queue<Appointment, std::vector<Appointment>, AppointmentComparator> queue_;
    mutable std::mutex mutex_;
};

} // namespace hospital
