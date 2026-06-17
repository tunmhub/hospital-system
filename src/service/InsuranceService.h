#pragma once

#include "service/IInsuranceService.h"
#include "repository/IPatientRepository.h"
#include "repository/IAppointmentRepository.h"

#include <memory>

namespace hospital {

/// 医保结算服务实现
///
/// 职责：
/// 1. 根据患者医保类型计算费用
/// 2. 挂号费：普通 10 元，加急 20 元，急诊 50 元
/// 3. 医保报销：职工 80%，居民/新农合 60%，自费 0%
class InsuranceService : public IInsuranceService {
public:
    InsuranceService(
        std::shared_ptr<IPatientRepository> patientRepo,
        std::shared_ptr<IAppointmentRepository> appointmentRepo
    );

    Result<Appointment> calculateFee(
        const Patient& patient,
        Priority priority
    ) override;

    Result<Appointment> settleAppointment(int64_t appointment_id) override;

private:
    /// 根据优先级获取基础挂号费
    static double getBaseFee(Priority priority);

    std::shared_ptr<IPatientRepository> patientRepo_;
    std::shared_ptr<IAppointmentRepository> appointmentRepo_;
};

} // namespace hospital
