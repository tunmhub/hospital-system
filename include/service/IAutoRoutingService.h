#pragma once

#include "common/Result.h"
#include "model/Appointment.h"
#include <cstdint>
#include <optional>
#include <string_view>

namespace hospital {

/// 自动分流服务接口
///
/// 职责：根据科室自动选择当前负载最低的医生
class IAutoRoutingService {
public:
    virtual ~IAutoRoutingService() = default;

    /// 自动选择医生：根据科室选择当前负载最低且未满的医生
    /// @param department  科室名称
    /// @return 选中的医生 ID，无可用医生返回 nullopt
    virtual std::optional<int64_t> selectDoctor(std::string_view department) = 0;
};

} // namespace hospital
