#pragma once

#include "repository/IAppointmentRepository.h"
#include "db/MySQLConnectionPool.h"

#include <memory>

namespace hospital {

/// 挂号仓储的 MySQL 实现
class MySQLAppointmentRepository : public IAppointmentRepository {
public:
    explicit MySQLAppointmentRepository(std::shared_ptr<MySQLConnectionPool> pool);

    std::optional<Appointment> findById(int64_t id) override;
    std::vector<Appointment> findAll() override;
    bool save(Appointment& entity) override;
    bool update(const Appointment& entity) override;
    bool remove(int64_t id) override;

    std::vector<Appointment> findByDoctor(int64_t doctor_id) override;
    std::vector<Appointment> findByPatient(int64_t patient_id) override;
    size_t countWaitingByDoctor(int64_t doctor_id) override;
    int getNextQueueNumber(int64_t doctor_id) override;

private:
    Appointment parseRow(MYSQL_ROW row);

    /// 将 AppointmentStatus 枚举转为数据库字符串
    static const char* statusToDb(AppointmentStatus s);
    /// 将 Priority 枚举转为数据库字符串
    static const char* priorityToDb(Priority p);
    /// 将数据库字符串转为 AppointmentStatus 枚举
    static AppointmentStatus dbToStatus(const char* s);
    /// 将数据库字符串转为 Priority 枚举
    static Priority dbToPriority(const char* s);

    std::shared_ptr<MySQLConnectionPool> pool_;
};

} // namespace hospital
