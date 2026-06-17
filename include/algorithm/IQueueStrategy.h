#pragma once

#include "model/Appointment.h"
#include <optional>
#include <cstddef>

namespace hospital {

/// 队列策略接口
/// 支持不同排队算法的插拔替换：
///   - 普通 FIFO 队列
///   - 优先级队列（急诊插队）
///   - 智能调度（未来扩展）
class IQueueStrategy {
public:
    virtual ~IQueueStrategy() = default;

    /// 将挂号加入队列
    virtual void enqueue(const Appointment& apt) = 0;

    /// 从队列取出下一个（按策略排序后的队首）
    virtual std::optional<Appointment> dequeue() = 0;

    /// 查看队首但不取出
    virtual std::optional<Appointment> peek() const = 0;

    /// 当前队列大小
    virtual size_t size() const = 0;

    /// 队列是否为空
    virtual bool empty() const = 0;

    /// 清空队列
    virtual void clear() = 0;
};

} // namespace hospital
