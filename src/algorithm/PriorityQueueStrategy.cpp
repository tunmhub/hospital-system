#include "algorithm/PriorityQueueStrategy.h"

namespace hospital {

void PriorityQueueStrategy::enqueue(const Appointment& apt) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(apt);
}

std::optional<Appointment> PriorityQueueStrategy::dequeue() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return std::nullopt;
    }
    Appointment top = queue_.top();
    queue_.pop();
    return top;
}

std::optional<Appointment> PriorityQueueStrategy::peek() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return std::nullopt;
    }
    return queue_.top();
}

size_t PriorityQueueStrategy::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

bool PriorityQueueStrategy::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

void PriorityQueueStrategy::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!queue_.empty()) {
        queue_.pop();
    }
}

void PriorityQueueStrategy::enqueueBatch(const std::vector<Appointment>& appointments) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& apt : appointments) {
        queue_.push(apt);
    }
}

} // namespace hospital
