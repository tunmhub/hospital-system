#pragma once

#include "repository/IRepository.h"
#include "model/Appointment.h"
#include <vector>

namespace hospital {

/// 挂号仓储接口
class IAppointmentRepository : public IRepository<Appointment> {
public:
    ~IAppointmentRepository() override = default;

    /// 根据医生 ID 查询等待中的挂号列表
    virtual std::vector<Appointment> findByDoctor(int64_t doctor_id) = 0;

    /// 根据患者 ID 查询挂号记录
    virtual std::vector<Appointment> findByPatient(int64_t patient_id) = 0;

    /// 获取指定医生当前等待队列长度
    virtual size_t countWaitingByDoctor(int64_t doctor_id) = 0;

    /// 获取指定医生当天的下一个排队号（按医生按日重置）
    virtual int getNextQueueNumber(int64_t doctor_id) = 0;
};

} // namespace hospital
