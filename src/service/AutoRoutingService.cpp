#include "service/AutoRoutingService.h"

#include <queue>
#include <vector>

namespace hospital {

AutoRoutingService::AutoRoutingService(
    std::shared_ptr<IDoctorRepository> doctorRepo,
    std::shared_ptr<IAppointmentRepository> appointmentRepo)
    : doctorRepo_(std::move(doctorRepo))
    , appointmentRepo_(std::move(appointmentRepo)) {}

// ============================================================
// 自动选择医生（从 AppointmentService 迁移，贪心最小堆算法）
// ============================================================

std::optional<int64_t> AutoRoutingService::selectDoctor(std::string_view department) {
    // 1. 获取该科室所有医生
    auto doctors = doctorRepo_->findByDepartment(department);
    if (doctors.empty()) {
        return std::nullopt;
    }

    // 2. 最小堆贪心：按 current_patients 从小到大排序
    struct DoctorLoad {
        int64_t doctor_id;
        int load;
        int max_patients;
    };
    auto cmp = [](const DoctorLoad& a, const DoctorLoad& b) {
        return a.load > b.load;  // 最小堆
    };
    std::priority_queue<DoctorLoad, std::vector<DoctorLoad>, decltype(cmp)> minHeap(cmp);

    for (const auto& doc : doctors) {
        minHeap.push({doc.id, doc.current_patients, doc.max_patients});
    }

    // 3. 贪心选择：从堆顶取出负载最低且未满的医生
    while (!minHeap.empty()) {
        auto top = minHeap.top();
        minHeap.pop();

        if (top.load < top.max_patients) {
            return top.doctor_id;
        }
    }

    return std::nullopt;
}

} // namespace hospital
