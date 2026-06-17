#include "service/InsuranceService.h"

#include <sstream>

namespace hospital {

InsuranceService::InsuranceService(
    std::shared_ptr<IPatientRepository> patientRepo,
    std::shared_ptr<IAppointmentRepository> appointmentRepo)
    : patientRepo_(std::move(patientRepo))
    , appointmentRepo_(std::move(appointmentRepo)) {}

double InsuranceService::getBaseFee(Priority priority) {
    switch (priority) {
        case Priority::Normal:    return 10.0;   // 普通挂号费 10 元
        case Priority::Urgent:    return 20.0;   // 加急挂号费 20 元
        case Priority::Emergency: return 50.0;   // 急诊挂号费 50 元
    }
    return 10.0;
}

Result<Appointment> InsuranceService::calculateFee(
    const Patient& patient,
    Priority priority) {

    // 1. 计算基础挂号费
    double baseFee = getBaseFee(priority);

    // 2. 获取医保报销比例
    double rate = getInsuranceReimbursementRate(patient.insurance_type);

    // 3. 计算医保报销金额和自费金额
    double insuranceFee = baseFee * rate;
    double selfFee = baseFee - insuranceFee;

    // 4. 构造结果
    Appointment apt;
    apt.registration_fee = baseFee;
    apt.insurance_fee = insuranceFee;
    apt.self_fee = selfFee;

    return Result<Appointment>::success(apt);
}

Result<Appointment> InsuranceService::settleAppointment(int64_t appointment_id) {
    // 1. 查询挂号记录
    auto aptOpt = appointmentRepo_->findById(appointment_id);
    if (!aptOpt) {
        return Result<Appointment>::failure("挂号记录不存在");
    }

    Appointment apt = *aptOpt;

    // 2. 检查是否已结算
    if (apt.settled) {
        return Result<Appointment>::failure("该挂号已结算");
    }

    // 3. 查询患者信息
    auto patientOpt = patientRepo_->findById(apt.patient_id);
    if (!patientOpt) {
        return Result<Appointment>::failure("患者信息不存在");
    }

    // 4. 计算费用
    double baseFee = getBaseFee(apt.priority);
    double rate = getInsuranceReimbursementRate(patientOpt->insurance_type);
    double insuranceFee = baseFee * rate;
    double selfFee = baseFee - insuranceFee;

    // 5. 更新挂号记录
    apt.registration_fee = baseFee;
    apt.insurance_fee = insuranceFee;
    apt.self_fee = selfFee;
    apt.settled = true;

    if (!appointmentRepo_->update(apt)) {
        return Result<Appointment>::failure("结算失败，更新数据库出错");
    }

    return Result<Appointment>::success(apt);
}

} // namespace hospital
