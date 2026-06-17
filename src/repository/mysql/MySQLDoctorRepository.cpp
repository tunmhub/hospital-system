#include "repository/mysql/MySQLDoctorRepository.h"
#include "common/Exception.h"

#include <sstream>

namespace hospital {

MySQLDoctorRepository::MySQLDoctorRepository(std::shared_ptr<MySQLConnectionPool> pool)
    : pool_(std::move(pool)) {}

std::optional<Doctor> MySQLDoctorRepository::findById(int64_t id) {
    auto conn = pool_->getConnection();

    std::ostringstream sql;
    sql << "SELECT id, name, department, title, work_start, work_end, max_patients, current_patients "
        << "FROM doctors WHERE id = " << id;

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("查询医生失败: ") + mysql_error(conn.get()));
    }

    MYSQL_RES* result = mysql_store_result(conn.get());
    if (!result) {
        throw DatabaseException(std::string("获取结果集失败: ") + mysql_error(conn.get()));
    }

    std::optional<Doctor> doctor;
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        doctor = parseRow(row);
    }

    mysql_free_result(result);
    return doctor;
}

std::vector<Doctor> MySQLDoctorRepository::findAll() {
    auto conn = pool_->getConnection();

    const char* sql = "SELECT id, name, department, title, work_start, work_end, max_patients, current_patients FROM doctors";

    if (mysql_query(conn.get(), sql) != 0) {
        throw DatabaseException(std::string("查询所有医生失败: ") + mysql_error(conn.get()));
    }

    MYSQL_RES* result = mysql_store_result(conn.get());
    if (!result) {
        throw DatabaseException(std::string("获取结果集失败: ") + mysql_error(conn.get()));
    }

    std::vector<Doctor> doctors;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        doctors.push_back(parseRow(row));
    }

    mysql_free_result(result);
    return doctors;
}

bool MySQLDoctorRepository::save(Doctor& entity) {
    auto conn = pool_->getConnection();

    std::string escaped_name = escape(conn, entity.name);
    std::string escaped_dept = escape(conn, entity.department);
    std::string escaped_title = escape(conn, entity.title);
    std::string escaped_start = escape(conn, entity.work_start);
    std::string escaped_end = escape(conn, entity.work_end);

    std::ostringstream sql;
    sql << "INSERT INTO doctors (name, department, title, work_start, work_end, max_patients, current_patients) VALUES ('"
        << escaped_name << "', '"
        << escaped_dept << "', '"
        << escaped_title << "', '"
        << escaped_start << "', '"
        << escaped_end << "', "
        << entity.max_patients << ", "
        << entity.current_patients << ")";

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("插入医生失败: ") + mysql_error(conn.get()));
    }

    entity.id = static_cast<int64_t>(mysql_insert_id(conn.get()));
    return true;
}

bool MySQLDoctorRepository::update(const Doctor& entity) {
    auto conn = pool_->getConnection();

    std::string escaped_name = escape(conn, entity.name);
    std::string escaped_dept = escape(conn, entity.department);
    std::string escaped_title = escape(conn, entity.title);
    std::string escaped_start = escape(conn, entity.work_start);
    std::string escaped_end = escape(conn, entity.work_end);

    std::ostringstream sql;
    sql << "UPDATE doctors SET "
        << "name = '"            << escaped_name   << "', "
        << "department = '"      << escaped_dept   << "', "
        << "title = '"           << escaped_title  << "', "
        << "work_start = '"      << escaped_start  << "', "
        << "work_end = '"        << escaped_end    << "', "
        << "max_patients = "     << entity.max_patients     << ", "
        << "current_patients = " << entity.current_patients << " "
        << "WHERE id = "         << entity.id;

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("更新医生失败: ") + mysql_error(conn.get()));
    }

    return mysql_affected_rows(conn.get()) > 0;
}

bool MySQLDoctorRepository::remove(int64_t id) {
    auto conn = pool_->getConnection();

    std::ostringstream sql;
    sql << "DELETE FROM doctors WHERE id = " << id;

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("删除医生失败: ") + mysql_error(conn.get()));
    }

    return mysql_affected_rows(conn.get()) > 0;
}

std::vector<Doctor> MySQLDoctorRepository::findByDepartment(std::string_view department) {
    auto conn = pool_->getConnection();

    std::string escaped = escape(conn, department);
    std::ostringstream sql;
    sql << "SELECT id, name, department, title, work_start, work_end, max_patients, current_patients "
        << "FROM doctors WHERE department = '" << escaped << "'";

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("按科室查询医生失败: ") + mysql_error(conn.get()));
    }

    MYSQL_RES* result = mysql_store_result(conn.get());
    if (!result) {
        throw DatabaseException(std::string("获取结果集失败: ") + mysql_error(conn.get()));
    }

    std::vector<Doctor> doctors;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        doctors.push_back(parseRow(row));
    }

    mysql_free_result(result);
    return doctors;
}

std::optional<Doctor> MySQLDoctorRepository::findLeastLoaded(std::string_view department) {
    auto conn = pool_->getConnection();

    std::string escaped = escape(conn, department);
    std::ostringstream sql;
    sql << "SELECT id, name, department, title, work_start, work_end, max_patients, current_patients "
        << "FROM doctors WHERE department = '" << escaped << "' "
        << "ORDER BY current_patients ASC LIMIT 1";

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("查询最低负载医生失败: ") + mysql_error(conn.get()));
    }

    MYSQL_RES* result = mysql_store_result(conn.get());
    if (!result) {
        throw DatabaseException(std::string("获取结果集失败: ") + mysql_error(conn.get()));
    }

    std::optional<Doctor> doctor;
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        doctor = parseRow(row);
    }

    mysql_free_result(result);
    return doctor;
}

Doctor MySQLDoctorRepository::parseRow(MYSQL_ROW row) {
    Doctor d;
    d.id               = row[0] ? std::stoll(row[0]) : 0;
    d.name             = row[1] ? row[1] : "";
    d.department       = row[2] ? row[2] : "";
    d.title            = row[3] ? row[3] : "";
    d.work_start       = row[4] ? row[4] : "08:00";
    d.work_end         = row[5] ? row[5] : "17:00";
    d.max_patients     = row[6] ? std::stoi(row[6]) : 0;
    d.current_patients = row[7] ? std::stoi(row[7]) : 0;
    return d;
}

std::string MySQLDoctorRepository::escape(std::shared_ptr<MYSQL> conn, std::string_view input) {
    std::string buffer(input.length() * 2 + 1, '\0');
    unsigned long len = mysql_real_escape_string(conn.get(),
                                                  buffer.data(),
                                                  input.data(),
                                                  input.length());
    buffer.resize(len);
    return buffer;
}

} // namespace hospital
