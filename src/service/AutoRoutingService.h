#pragma once

#include "service/IAutoRoutingService.h"
#include "repository/IDoctorRepository.h"
#include "repository/IAppointmentRepository.h"

#include <memory>

namespace hospital {

/// 自动分流服务实现
///
/// 从 AppointmentService 中提取，职责：
/// 1. 根据科室查找可用医生
/// 2. 贪心最小堆算法选择负载最低的医生
/// 3. 返回选中的 doctor_id（不执行挂号，不持有锁）
class AutoRoutingService : public IAutoRoutingService {
public:
    AutoRoutingService(
        std::shared_ptr<IDoctorRepository> doctorRepo,
        std::shared_ptr<IAppointmentRepository> appointmentRepo
    );

    std::optional<int64_t> selectDoctor(std::string_view department) override;

private:
    std::shared_ptr<IDoctorRepository> doctorRepo_;
    std::shared_ptr<IAppointmentRepository> appointmentRepo_;
};

} // namespace hospital
