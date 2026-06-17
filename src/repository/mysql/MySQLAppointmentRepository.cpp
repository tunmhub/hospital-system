#include "repository/mysql/MySQLAppointmentRepository.h"
#include "common/Exception.h"

#include <sstream>

namespace hospital {

MySQLAppointmentRepository::MySQLAppointmentRepository(std::shared_ptr<MySQLConnectionPool> pool)
    : pool_(std::move(pool)) {}

std::optional<Appointment> MySQLAppointmentRepository::findById(int64_t id) {
    auto conn = pool_->getConnection();

    std::ostringstream sql;
    sql << "SELECT id, patient_id, doctor_id, status, priority, queue_number, "
        << "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at "
        << "FROM appointments WHERE id = " << id;

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("查询挂号失败: ") + mysql_error(conn.get()));
    }

    MYSQL_RES* result = mysql_store_result(conn.get());
    if (!result) {
        throw DatabaseException(std::string("获取结果集失败: ") + mysql_error(conn.get()));
    }

    std::optional<Appointment> apt;
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        apt = parseRow(row);
    }

    mysql_free_result(result);
    return apt;
}

std::vector<Appointment> MySQLAppointmentRepository::findAll() {
    auto conn = pool_->getConnection();

    const char* sql = "SELECT id, patient_id, doctor_id, status, priority, queue_number, "
                      "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at "
                      "FROM appointments";

    if (mysql_query(conn.get(), sql) != 0) {
        throw DatabaseException(std::string("查询所有挂号失败: ") + mysql_error(conn.get()));
    }

    MYSQL_RES* result = mysql_store_result(conn.get());
    if (!result) {
        throw DatabaseException(std::string("获取结果集失败: ") + mysql_error(conn.get()));
    }

    std::vector<Appointment> appointments;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        appointments.push_back(parseRow(row));
    }

    mysql_free_result(result);
    return appointments;
}

bool MySQLAppointmentRepository::save(Appointment& entity) {
    auto conn = pool_->getConnection();

    std::ostringstream sql;
    sql << "INSERT INTO appointments (patient_id, doctor_id, status, priority, queue_number) VALUES ("
        << entity.patient_id << ", "
        << entity.doctor_id << ", '"
        << statusToDb(entity.status) << "', '"
        << priorityToDb(entity.priority) << "', "
        << entity.queue_number << ")";

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("插入挂号失败: ") + mysql_error(conn.get()));
    }

    entity.id = static_cast<int64_t>(mysql_insert_id(conn.get()));
    return true;
}

bool MySQLAppointmentRepository::update(const Appointment& entity) {
    auto conn = pool_->getConnection();

    std::ostringstream sql;
    sql << "UPDATE appointments SET "
        << "patient_id = "    << entity.patient_id    << ", "
        << "doctor_id = "     << entity.doctor_id     << ", "
        << "status = '"       << statusToDb(entity.status)   << "', "
        << "priority = '"     << priorityToDb(entity.priority) << "', "
        << "queue_number = "  << entity.queue_number  << " "
        << "WHERE id = "      << entity.id;

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("更新挂号失败: ") + mysql_error(conn.get()));
    }

    return mysql_affected_rows(conn.get()) > 0;
}

bool MySQLAppointmentRepository::remove(int64_t id) {
    auto conn = pool_->getConnection();

    std::ostringstream sql;
    sql << "DELETE FROM appointments WHERE id = " << id;

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("删除挂号失败: ") + mysql_error(conn.get()));
    }

    return mysql_affected_rows(conn.get()) > 0;
}

std::vector<Appointment> MySQLAppointmentRepository::findByDoctor(int64_t doctor_id) {
    auto conn = pool_->getConnection();

    std::ostringstream sql;
    sql << "SELECT id, patient_id, doctor_id, status, priority, queue_number, "
        << "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at "
        << "FROM appointments WHERE doctor_id = " << doctor_id;

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("按医生查询挂号失败: ") + mysql_error(conn.get()));
    }

    MYSQL_RES* result = mysql_store_result(conn.get());
    if (!result) {
        throw DatabaseException(std::string("获取结果集失败: ") + mysql_error(conn.get()));
    }

    std::vector<Appointment> appointments;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        appointments.push_back(parseRow(row));
    }

    mysql_free_result(result);
    return appointments;
}

std::vector<Appointment> MySQLAppointmentRepository::findByPatient(int64_t patient_id) {
    auto conn = pool_->getConnection();

    std::ostringstream sql;
    sql << "SELECT id, patient_id, doctor_id, status, priority, queue_number, "
        << "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at "
        << "FROM appointments WHERE patient_id = " << patient_id;

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("按患者查询挂号失败: ") + mysql_error(conn.get()));
    }

    MYSQL_RES* result = mysql_store_result(conn.get());
    if (!result) {
        throw DatabaseException(std::string("获取结果集失败: ") + mysql_error(conn.get()));
    }

    std::vector<Appointment> appointments;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        appointments.push_back(parseRow(row));
    }

    mysql_free_result(result);
    return appointments;
}

size_t MySQLAppointmentRepository::countWaitingByDoctor(int64_t doctor_id) {
    auto conn = pool_->getConnection();

    std::ostringstream sql;
    sql << "SELECT COUNT(*) FROM appointments "
        << "WHERE doctor_id = " << doctor_id << " AND status = 'waiting'";

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("统计等待人数失败: ") + mysql_error(conn.get()));
    }

    MYSQL_RES* result = mysql_store_result(conn.get());
    if (!result) {
        throw DatabaseException(std::string("获取结果集失败: ") + mysql_error(conn.get()));
    }

    size_t count = 0;
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row && row[0]) {
        count = std::stoull(row[0]);
    }

    mysql_free_result(result);
    return count;
}

int MySQLAppointmentRepository::getNextQueueNumber() {
    auto conn = pool_->getConnection();

    const char* sql = "SELECT COALESCE(MAX(queue_number), 0) + 1 FROM appointments";

    if (mysql_query(conn.get(), sql) != 0) {
        throw DatabaseException(std::string("获取排队号失败: ") + mysql_error(conn.get()));
    }

    MYSQL_RES* result = mysql_store_result(conn.get());
    if (!result) {
        throw DatabaseException(std::string("获取结果集失败: ") + mysql_error(conn.get()));
    }

    int next = 1;
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row && row[0]) {
        next = std::stoi(row[0]);
    }

    mysql_free_result(result);
    return next;
}

Appointment MySQLAppointmentRepository::parseRow(MYSQL_ROW row) {
    Appointment a;
    a.id           = row[0] ? std::stoll(row[0]) : 0;
    a.patient_id   = row[1] ? std::stoll(row[1]) : 0;
    a.doctor_id    = row[2] ? std::stoll(row[2]) : 0;
    a.status       = row[3] ? dbToStatus(row[3]) : AppointmentStatus::Waiting;
    a.priority     = row[4] ? dbToPriority(row[4]) : Priority::Normal;
    a.queue_number = row[5] ? std::stoi(row[5]) : 0;
    a.created_at   = row[6] ? row[6] : "";
    return a;
}

const char* MySQLAppointmentRepository::statusToDb(AppointmentStatus s) {
    switch (s) {
        case AppointmentStatus::Waiting:    return "waiting";
        case AppointmentStatus::InProgress: return "in_progress";
        case AppointmentStatus::Completed:  return "completed";
        case AppointmentStatus::Cancelled:  return "cancelled";
    }
    return "waiting";
}

const char* MySQLAppointmentRepository::priorityToDb(Priority p) {
    switch (p) {
        case Priority::Normal:    return "normal";
        case Priority::Urgent:    return "urgent";
        case Priority::Emergency: return "emergency";
    }
    return "normal";
}

AppointmentStatus MySQLAppointmentRepository::dbToStatus(const char* s) {
    if (std::string(s) == "in_progress") return AppointmentStatus::InProgress;
    if (std::string(s) == "completed")   return AppointmentStatus::Completed;
    if (std::string(s) == "cancelled")   return AppointmentStatus::Cancelled;
    return AppointmentStatus::Waiting;
}

Priority MySQLAppointmentRepository::dbToPriority(const char* s) {
    if (std::string(s) == "urgent")    return Priority::Urgent;
    if (std::string(s) == "emergency") return Priority::Emergency;
    return Priority::Normal;
}

} // namespace hospital
