#include "repository/mysql/MySQLAppointmentRepository.h"
#include "common/Exception.h"

#include <sstream>
#include <cstring>

namespace hospital {

MySQLAppointmentRepository::MySQLAppointmentRepository(std::shared_ptr<MySQLConnectionPool> pool)
    : pool_(std::move(pool)) {}

std::optional<Appointment> MySQLAppointmentRepository::findById(int64_t id) {
    auto conn = pool_->getConnection();

    const char* sql = "SELECT id, patient_id, doctor_id, status, priority, queue_number, "
                      "registration_fee, insurance_fee, self_fee, settled, "
                      "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at "
                      "FROM appointments WHERE id = ?";

    auto stmt = mysql_stmt_init(conn.get());
    if (!stmt) {
        throw DatabaseException("初始化语句失败");
    }

    if (mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("准备语句失败: ") + mysql_error(conn.get()));
    }

    MYSQL_BIND bind[1] = {};
    long long param_id = id;
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer = &param_id;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("绑定参数失败: ") + mysql_error(conn.get()));
    }

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("执行查询失败: ") + mysql_error(conn.get()));
    }

    // 绑定结果
    MYSQL_BIND result_bind[11] = {};
    long long r_id, r_patient_id, r_doctor_id;
    char r_status[20], r_priority[20];
    int r_queue_number, r_settled;
    double r_registration_fee, r_insurance_fee, r_self_fee;
    char r_created_at[30];
    unsigned long r_status_len, r_priority_len, r_created_at_len;

    result_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    result_bind[0].buffer = &r_id;
    result_bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    result_bind[1].buffer = &r_patient_id;
    result_bind[2].buffer_type = MYSQL_TYPE_LONGLONG;
    result_bind[2].buffer = &r_doctor_id;
    result_bind[3].buffer_type = MYSQL_TYPE_STRING;
    result_bind[3].buffer = r_status;
    result_bind[3].buffer_length = sizeof(r_status);
    result_bind[3].length = &r_status_len;
    result_bind[4].buffer_type = MYSQL_TYPE_STRING;
    result_bind[4].buffer = r_priority;
    result_bind[4].buffer_length = sizeof(r_priority);
    result_bind[4].length = &r_priority_len;
    result_bind[5].buffer_type = MYSQL_TYPE_LONG;
    result_bind[5].buffer = &r_queue_number;
    result_bind[6].buffer_type = MYSQL_TYPE_DOUBLE;
    result_bind[6].buffer = &r_registration_fee;
    result_bind[7].buffer_type = MYSQL_TYPE_DOUBLE;
    result_bind[7].buffer = &r_insurance_fee;
    result_bind[8].buffer_type = MYSQL_TYPE_DOUBLE;
    result_bind[8].buffer = &r_self_fee;
    result_bind[9].buffer_type = MYSQL_TYPE_LONG;
    result_bind[9].buffer = &r_settled;
    result_bind[10].buffer_type = MYSQL_TYPE_STRING;
    result_bind[10].buffer = r_created_at;
    result_bind[10].buffer_length = sizeof(r_created_at);
    result_bind[10].length = &r_created_at_len;

    if (mysql_stmt_bind_result(stmt, result_bind) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("绑定结果失败: ") + mysql_error(conn.get()));
    }

    if (mysql_stmt_store_result(stmt) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("存储结果失败: ") + mysql_error(conn.get()));
    }

    std::optional<Appointment> apt;
    if (mysql_stmt_fetch(stmt) == 0) {
        Appointment a;
        a.id = r_id;
        a.patient_id = r_patient_id;
        a.doctor_id = r_doctor_id;
        a.status = dbToStatus(r_status);
        a.priority = dbToPriority(r_priority);
        a.queue_number = r_queue_number;
        a.registration_fee = r_registration_fee;
        a.insurance_fee = r_insurance_fee;
        a.self_fee = r_self_fee;
        a.settled = (r_settled != 0);
        a.created_at = std::string(r_created_at, r_created_at_len);
        apt = a;
    }

    mysql_stmt_close(stmt);
    return apt;
}

std::vector<Appointment> MySQLAppointmentRepository::findAll() {
    auto conn = pool_->getConnection();

    const char* sql = "SELECT id, patient_id, doctor_id, status, priority, queue_number, "
                      "registration_fee, insurance_fee, self_fee, settled, "
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

    const char* sql = "INSERT INTO appointments (patient_id, doctor_id, status, priority, queue_number, "
                      "registration_fee, insurance_fee, self_fee, settled) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

    auto stmt = mysql_stmt_init(conn.get());
    if (!stmt) {
        throw DatabaseException("初始化语句失败");
    }

    if (mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("准备语句失败: ") + mysql_error(conn.get()));
    }

    const char* status_str = statusToDb(entity.status);
    const char* priority_str = priorityToDb(entity.priority);

    MYSQL_BIND bind[9] = {};
    long long patient_id = entity.patient_id;
    long long doctor_id = entity.doctor_id;
    int queue_number = entity.queue_number;
    double registration_fee = entity.registration_fee;
    double insurance_fee = entity.insurance_fee;
    double self_fee = entity.self_fee;
    int settled = entity.settled ? 1 : 0;

    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer = &patient_id;
    bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[1].buffer = &doctor_id;
    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = (void*)status_str;
    bind[2].buffer_length = strlen(status_str);
    bind[3].buffer_type = MYSQL_TYPE_STRING;
    bind[3].buffer = (void*)priority_str;
    bind[3].buffer_length = strlen(priority_str);
    bind[4].buffer_type = MYSQL_TYPE_LONG;
    bind[4].buffer = &queue_number;
    bind[5].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[5].buffer = &registration_fee;
    bind[6].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[6].buffer = &insurance_fee;
    bind[7].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[7].buffer = &self_fee;
    bind[8].buffer_type = MYSQL_TYPE_LONG;
    bind[8].buffer = &settled;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("绑定参数失败: ") + mysql_error(conn.get()));
    }

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("插入挂号失败: ") + mysql_error(conn.get()));
    }

    entity.id = static_cast<int64_t>(mysql_stmt_insert_id(stmt));
    mysql_stmt_close(stmt);
    return true;
}

bool MySQLAppointmentRepository::update(const Appointment& entity) {
    auto conn = pool_->getConnection();

    const char* sql = "UPDATE appointments SET "
                      "patient_id = ?, doctor_id = ?, status = ?, priority = ?, queue_number = ?, "
                      "registration_fee = ?, insurance_fee = ?, self_fee = ?, settled = ? "
                      "WHERE id = ?";

    auto stmt = mysql_stmt_init(conn.get());
    if (!stmt) {
        throw DatabaseException("初始化语句失败");
    }

    if (mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("准备语句失败: ") + mysql_error(conn.get()));
    }

    const char* status_str = statusToDb(entity.status);
    const char* priority_str = priorityToDb(entity.priority);

    MYSQL_BIND bind[10] = {};
    long long patient_id = entity.patient_id;
    long long doctor_id = entity.doctor_id;
    int queue_number = entity.queue_number;
    double registration_fee = entity.registration_fee;
    double insurance_fee = entity.insurance_fee;
    double self_fee = entity.self_fee;
    int settled = entity.settled ? 1 : 0;
    long long id = entity.id;

    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer = &patient_id;
    bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[1].buffer = &doctor_id;
    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = (void*)status_str;
    bind[2].buffer_length = strlen(status_str);
    bind[3].buffer_type = MYSQL_TYPE_STRING;
    bind[3].buffer = (void*)priority_str;
    bind[3].buffer_length = strlen(priority_str);
    bind[4].buffer_type = MYSQL_TYPE_LONG;
    bind[4].buffer = &queue_number;
    bind[5].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[5].buffer = &registration_fee;
    bind[6].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[6].buffer = &insurance_fee;
    bind[7].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[7].buffer = &self_fee;
    bind[8].buffer_type = MYSQL_TYPE_LONG;
    bind[8].buffer = &settled;
    bind[9].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[9].buffer = &id;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("绑定参数失败: ") + mysql_error(conn.get()));
    }

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("更新挂号失败: ") + mysql_error(conn.get()));
    }

    bool affected = mysql_stmt_affected_rows(stmt) > 0;
    mysql_stmt_close(stmt);
    return affected;
}

bool MySQLAppointmentRepository::remove(int64_t id) {
    auto conn = pool_->getConnection();

    const char* sql = "DELETE FROM appointments WHERE id = ?";

    auto stmt = mysql_stmt_init(conn.get());
    if (!stmt) {
        throw DatabaseException("初始化语句失败");
    }

    if (mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("准备语句失败: ") + mysql_error(conn.get()));
    }

    MYSQL_BIND bind[1] = {};
    long long param_id = id;
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer = &param_id;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("绑定参数失败: ") + mysql_error(conn.get()));
    }

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("删除挂号失败: ") + mysql_error(conn.get()));
    }

    bool affected = mysql_stmt_affected_rows(stmt) > 0;
    mysql_stmt_close(stmt);
    return affected;
}

std::vector<Appointment> MySQLAppointmentRepository::findByDoctor(int64_t doctor_id) {
    auto conn = pool_->getConnection();

    const char* sql = "SELECT id, patient_id, doctor_id, status, priority, queue_number, "
                      "registration_fee, insurance_fee, self_fee, settled, "
                      "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at "
                      "FROM appointments WHERE doctor_id = ?";

    auto stmt = mysql_stmt_init(conn.get());
    if (!stmt) {
        throw DatabaseException("初始化语句失败");
    }

    if (mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("准备语句失败: ") + mysql_error(conn.get()));
    }

    MYSQL_BIND bind[1] = {};
    long long param_id = doctor_id;
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer = &param_id;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("绑定参数失败: ") + mysql_error(conn.get()));
    }

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("执行查询失败: ") + mysql_error(conn.get()));
    }

    // 绑定结果
    MYSQL_BIND result_bind[11] = {};
    long long r_id, r_patient_id, r_doctor_id;
    char r_status[20], r_priority[20];
    int r_queue_number, r_settled;
    double r_registration_fee, r_insurance_fee, r_self_fee;
    char r_created_at[30];
    unsigned long r_status_len, r_priority_len, r_created_at_len;

    result_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    result_bind[0].buffer = &r_id;
    result_bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    result_bind[1].buffer = &r_patient_id;
    result_bind[2].buffer_type = MYSQL_TYPE_LONGLONG;
    result_bind[2].buffer = &r_doctor_id;
    result_bind[3].buffer_type = MYSQL_TYPE_STRING;
    result_bind[3].buffer = r_status;
    result_bind[3].buffer_length = sizeof(r_status);
    result_bind[3].length = &r_status_len;
    result_bind[4].buffer_type = MYSQL_TYPE_STRING;
    result_bind[4].buffer = r_priority;
    result_bind[4].buffer_length = sizeof(r_priority);
    result_bind[4].length = &r_priority_len;
    result_bind[5].buffer_type = MYSQL_TYPE_LONG;
    result_bind[5].buffer = &r_queue_number;
    result_bind[6].buffer_type = MYSQL_TYPE_DOUBLE;
    result_bind[6].buffer = &r_registration_fee;
    result_bind[7].buffer_type = MYSQL_TYPE_DOUBLE;
    result_bind[7].buffer = &r_insurance_fee;
    result_bind[8].buffer_type = MYSQL_TYPE_DOUBLE;
    result_bind[8].buffer = &r_self_fee;
    result_bind[9].buffer_type = MYSQL_TYPE_LONG;
    result_bind[9].buffer = &r_settled;
    result_bind[10].buffer_type = MYSQL_TYPE_STRING;
    result_bind[10].buffer = r_created_at;
    result_bind[10].buffer_length = sizeof(r_created_at);
    result_bind[10].length = &r_created_at_len;

    if (mysql_stmt_bind_result(stmt, result_bind) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("绑定结果失败: ") + mysql_error(conn.get()));
    }

    if (mysql_stmt_store_result(stmt) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("存储结果失败: ") + mysql_error(conn.get()));
    }

    std::vector<Appointment> appointments;
    while (mysql_stmt_fetch(stmt) == 0) {
        Appointment a;
        a.id = r_id;
        a.patient_id = r_patient_id;
        a.doctor_id = r_doctor_id;
        a.status = dbToStatus(r_status);
        a.priority = dbToPriority(r_priority);
        a.queue_number = r_queue_number;
        a.registration_fee = r_registration_fee;
        a.insurance_fee = r_insurance_fee;
        a.self_fee = r_self_fee;
        a.settled = (r_settled != 0);
        a.created_at = std::string(r_created_at, r_created_at_len);
        appointments.push_back(a);
    }

    mysql_stmt_close(stmt);
    return appointments;
}

std::vector<Appointment> MySQLAppointmentRepository::findByPatient(int64_t patient_id) {
    auto conn = pool_->getConnection();

    const char* sql = "SELECT id, patient_id, doctor_id, status, priority, queue_number, "
                      "registration_fee, insurance_fee, self_fee, settled, "
                      "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at "
                      "FROM appointments WHERE patient_id = ?";

    auto stmt = mysql_stmt_init(conn.get());
    if (!stmt) {
        throw DatabaseException("初始化语句失败");
    }

    if (mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("准备语句失败: ") + mysql_error(conn.get()));
    }

    MYSQL_BIND bind[1] = {};
    long long param_id = patient_id;
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer = &param_id;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("绑定参数失败: ") + mysql_error(conn.get()));
    }

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("执行查询失败: ") + mysql_error(conn.get()));
    }

    // 绑定结果
    MYSQL_BIND result_bind[11] = {};
    long long r_id, r_patient_id, r_doctor_id;
    char r_status[20], r_priority[20];
    int r_queue_number, r_settled;
    double r_registration_fee, r_insurance_fee, r_self_fee;
    char r_created_at[30];
    unsigned long r_status_len, r_priority_len, r_created_at_len;

    result_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    result_bind[0].buffer = &r_id;
    result_bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    result_bind[1].buffer = &r_patient_id;
    result_bind[2].buffer_type = MYSQL_TYPE_LONGLONG;
    result_bind[2].buffer = &r_doctor_id;
    result_bind[3].buffer_type = MYSQL_TYPE_STRING;
    result_bind[3].buffer = r_status;
    result_bind[3].buffer_length = sizeof(r_status);
    result_bind[3].length = &r_status_len;
    result_bind[4].buffer_type = MYSQL_TYPE_STRING;
    result_bind[4].buffer = r_priority;
    result_bind[4].buffer_length = sizeof(r_priority);
    result_bind[4].length = &r_priority_len;
    result_bind[5].buffer_type = MYSQL_TYPE_LONG;
    result_bind[5].buffer = &r_queue_number;
    result_bind[6].buffer_type = MYSQL_TYPE_DOUBLE;
    result_bind[6].buffer = &r_registration_fee;
    result_bind[7].buffer_type = MYSQL_TYPE_DOUBLE;
    result_bind[7].buffer = &r_insurance_fee;
    result_bind[8].buffer_type = MYSQL_TYPE_DOUBLE;
    result_bind[8].buffer = &r_self_fee;
    result_bind[9].buffer_type = MYSQL_TYPE_LONG;
    result_bind[9].buffer = &r_settled;
    result_bind[10].buffer_type = MYSQL_TYPE_STRING;
    result_bind[10].buffer = r_created_at;
    result_bind[10].buffer_length = sizeof(r_created_at);
    result_bind[10].length = &r_created_at_len;

    if (mysql_stmt_bind_result(stmt, result_bind) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("绑定结果失败: ") + mysql_error(conn.get()));
    }

    if (mysql_stmt_store_result(stmt) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("存储结果失败: ") + mysql_error(conn.get()));
    }

    std::vector<Appointment> appointments;
    while (mysql_stmt_fetch(stmt) == 0) {
        Appointment a;
        a.id = r_id;
        a.patient_id = r_patient_id;
        a.doctor_id = r_doctor_id;
        a.status = dbToStatus(r_status);
        a.priority = dbToPriority(r_priority);
        a.queue_number = r_queue_number;
        a.registration_fee = r_registration_fee;
        a.insurance_fee = r_insurance_fee;
        a.self_fee = r_self_fee;
        a.settled = (r_settled != 0);
        a.created_at = std::string(r_created_at, r_created_at_len);
        appointments.push_back(a);
    }

    mysql_stmt_close(stmt);
    return appointments;
}

size_t MySQLAppointmentRepository::countWaitingByDoctor(int64_t doctor_id) {
    auto conn = pool_->getConnection();

    const char* sql = "SELECT COUNT(*) FROM appointments "
                      "WHERE doctor_id = ? AND status = 'waiting'";

    auto stmt = mysql_stmt_init(conn.get());
    if (!stmt) {
        throw DatabaseException("初始化语句失败");
    }

    if (mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("准备语句失败: ") + mysql_error(conn.get()));
    }

    MYSQL_BIND bind[1] = {};
    long long param_id = doctor_id;
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer = &param_id;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("绑定参数失败: ") + mysql_error(conn.get()));
    }

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("执行查询失败: ") + mysql_error(conn.get()));
    }

    // 绑定结果
    MYSQL_BIND result_bind[1] = {};
    long long count = 0;
    result_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    result_bind[0].buffer = &count;

    if (mysql_stmt_bind_result(stmt, result_bind) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("绑定结果失败: ") + mysql_error(conn.get()));
    }

    if (mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("获取结果失败: ") + mysql_error(conn.get()));
    }

    mysql_stmt_close(stmt);
    return static_cast<size_t>(count);
}

int MySQLAppointmentRepository::getNextQueueNumber(int64_t doctor_id) {
    auto conn = pool_->getConnection();

    // 使用 INSERT ... ON DUPLICATE KEY UPDATE 实现原子递增
    // 首先确保表存在
    const char* create_sql = "CREATE TABLE IF NOT EXISTS queue_sequences ("
                             "doctor_id BIGINT NOT NULL, "
                             "seq_date DATE NOT NULL, "
                             "last_number INT NOT NULL DEFAULT 0, "
                             "PRIMARY KEY (doctor_id, seq_date))";

    if (mysql_query(conn.get(), create_sql) != 0) {
        throw DatabaseException(std::string("创建序列表失败: ") + mysql_error(conn.get()));
    }

    // 插入或递增当天的序列号
    const char* upsert_sql = "INSERT INTO queue_sequences (doctor_id, seq_date, last_number) "
                             "VALUES (?, CURDATE(), 1) "
                             "ON DUPLICATE KEY UPDATE last_number = last_number + 1";

    auto stmt = mysql_stmt_init(conn.get());
    if (!stmt) {
        throw DatabaseException("初始化语句失败");
    }

    if (mysql_stmt_prepare(stmt, upsert_sql, strlen(upsert_sql)) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("准备语句失败: ") + mysql_error(conn.get()));
    }

    MYSQL_BIND bind[1] = {};
    long long param_doctor_id = doctor_id;
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer = &param_doctor_id;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("绑定参数失败: ") + mysql_error(conn.get()));
    }

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("执行更新失败: ") + mysql_error(conn.get()));
    }

    mysql_stmt_close(stmt);

    // 查询当前值
    const char* select_sql = "SELECT last_number FROM queue_sequences "
                             "WHERE doctor_id = ? AND seq_date = CURDATE()";

    stmt = mysql_stmt_init(conn.get());
    if (!stmt) {
        throw DatabaseException("初始化语句失败");
    }

    if (mysql_stmt_prepare(stmt, select_sql, strlen(select_sql)) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("准备语句失败: ") + mysql_error(conn.get()));
    }

    MYSQL_BIND select_bind[1] = {};
    select_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    select_bind[0].buffer = &param_doctor_id;

    if (mysql_stmt_bind_param(stmt, select_bind) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("绑定参数失败: ") + mysql_error(conn.get()));
    }

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("执行查询失败: ") + mysql_error(conn.get()));
    }

    MYSQL_BIND result_bind[1] = {};
    int last_number = 0;
    result_bind[0].buffer_type = MYSQL_TYPE_LONG;
    result_bind[0].buffer = &last_number;

    if (mysql_stmt_bind_result(stmt, result_bind) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("绑定结果失败: ") + mysql_error(conn.get()));
    }

    if (mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_close(stmt);
        throw DatabaseException(std::string("获取结果失败: ") + mysql_error(conn.get()));
    }

    mysql_stmt_close(stmt);
    return last_number;
}

Appointment MySQLAppointmentRepository::parseRow(MYSQL_ROW row) {
    Appointment a;
    a.id                = row[0] ? std::stoll(row[0]) : 0;
    a.patient_id        = row[1] ? std::stoll(row[1]) : 0;
    a.doctor_id         = row[2] ? std::stoll(row[2]) : 0;
    a.status            = row[3] ? dbToStatus(row[3]) : AppointmentStatus::Waiting;
    a.priority          = row[4] ? dbToPriority(row[4]) : Priority::Normal;
    a.queue_number      = row[5] ? std::stoi(row[5]) : 0;
    a.registration_fee  = row[6] ? std::stod(row[6]) : 0.0;
    a.insurance_fee     = row[7] ? std::stod(row[7]) : 0.0;
    a.self_fee          = row[8] ? std::stod(row[8]) : 0.0;
    a.settled           = row[9] ? (std::stoi(row[9]) != 0) : false;
    a.created_at        = row[10] ? row[10] : "";
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

std::vector<DeptStat> MySQLAppointmentRepository::countWaitingByDepartment() {
    auto conn = pool_->getConnection();

    // 按科室统计候诊人数，每个患者按 10 分钟预估等待时间
    const char* sql = "SELECT d.department, COUNT(*) AS waiting_count, "
                      "COUNT(*) * 10 AS estimated_wait_minutes "
                      "FROM appointments a "
                      "JOIN doctors d ON a.doctor_id = d.id "
                      "WHERE a.status = 'waiting' "
                      "GROUP BY d.department "
                      "ORDER BY waiting_count DESC";

    if (mysql_query(conn.get(), sql) != 0) {
        throw DatabaseException(std::string("统计科室候诊人数失败: ") + mysql_error(conn.get()));
    }

    MYSQL_RES* result = mysql_store_result(conn.get());
    if (!result) {
        throw DatabaseException(std::string("获取结果集失败: ") + mysql_error(conn.get()));
    }

    std::vector<DeptStat> stats;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        DeptStat stat;
        stat.department = row[0] ? row[0] : "";
        stat.waiting_count = row[1] ? std::stoi(row[1]) : 0;
        stat.estimated_wait_minutes = row[2] ? std::stoi(row[2]) : 0;
        stats.push_back(stat);
    }

    mysql_free_result(result);
    return stats;
}

} // namespace hospital
