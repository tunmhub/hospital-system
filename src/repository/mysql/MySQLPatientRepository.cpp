#include "repository/mysql/MySQLPatientRepository.h"
#include "common/Exception.h"

#include <iostream>
#include <sstream>

namespace hospital {

MySQLPatientRepository::MySQLPatientRepository(std::shared_ptr<MySQLConnectionPool> pool)
    : pool_(std::move(pool)) {}

std::optional<Patient> MySQLPatientRepository::findById(int64_t id) {
    auto conn = pool_->getConnection();

    std::ostringstream sql;
    sql << "SELECT id, name, phone, id_card, age, gender, "
        << "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at "
        << "FROM patients WHERE id = " << id;

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("查询患者失败: ") + mysql_error(conn.get()));
    }

    MYSQL_RES* result = mysql_store_result(conn.get());
    if (!result) {
        throw DatabaseException(std::string("获取结果集失败: ") + mysql_error(conn.get()));
    }

    std::optional<Patient> patient;
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        patient = parseRow(row);
    }

    mysql_free_result(result);
    return patient;
}

std::vector<Patient> MySQLPatientRepository::findAll() {
    auto conn = pool_->getConnection();

    const char* sql = "SELECT id, name, phone, id_card, age, gender, "
                      "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at "
                      "FROM patients";

    if (mysql_query(conn.get(), sql) != 0) {
        throw DatabaseException(std::string("查询所有患者失败: ") + mysql_error(conn.get()));
    }

    MYSQL_RES* result = mysql_store_result(conn.get());
    if (!result) {
        throw DatabaseException(std::string("获取结果集失败: ") + mysql_error(conn.get()));
    }

    std::vector<Patient> patients;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        patients.push_back(parseRow(row));
    }

    mysql_free_result(result);
    return patients;
}

bool MySQLPatientRepository::save(Patient& entity) {
    auto conn = pool_->getConnection();

    std::string escaped_name  = escape(conn, entity.name);
    std::string escaped_phone = escape(conn, entity.phone);
    std::string escaped_idcard = escape(conn, entity.id_card);
    std::string escaped_gender = escape(conn, entity.gender);

    std::ostringstream sql;
    sql << "INSERT INTO patients (name, phone, id_card, age, gender) VALUES ('"
        << escaped_name << "', '"
        << escaped_phone << "', '"
        << escaped_idcard << "', "
        << entity.age << ", '"
        << escaped_gender << "')";

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("插入患者失败: ") + mysql_error(conn.get()));
    }

    // 获取自增 ID
    entity.id = static_cast<int64_t>(mysql_insert_id(conn.get()));
    return true;
}

bool MySQLPatientRepository::update(const Patient& entity) {
    auto conn = pool_->getConnection();

    std::string escaped_name  = escape(conn, entity.name);
    std::string escaped_phone = escape(conn, entity.phone);
    std::string escaped_idcard = escape(conn, entity.id_card);
    std::string escaped_gender = escape(conn, entity.gender);

    std::ostringstream sql;
    sql << "UPDATE patients SET "
        << "name = '"     << escaped_name   << "', "
        << "phone = '"    << escaped_phone  << "', "
        << "id_card = '"  << escaped_idcard << "', "
        << "age = "       << entity.age     << ", "
        << "gender = '"   << escaped_gender << "' "
        << "WHERE id = "  << entity.id;

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("更新患者失败: ") + mysql_error(conn.get()));
    }

    return mysql_affected_rows(conn.get()) > 0;
}

bool MySQLPatientRepository::remove(int64_t id) {
    auto conn = pool_->getConnection();

    std::ostringstream sql;
    sql << "DELETE FROM patients WHERE id = " << id;

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("删除患者失败: ") + mysql_error(conn.get()));
    }

    return mysql_affected_rows(conn.get()) > 0;
}

std::optional<Patient> MySQLPatientRepository::findByPhone(std::string_view phone) {
    auto conn = pool_->getConnection();

    std::string escaped = escape(conn, phone);
    std::ostringstream sql;
    sql << "SELECT id, name, phone, id_card, age, gender, "
        << "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at "
        << "FROM patients WHERE phone = '" << escaped << "'";

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("按手机号查询患者失败: ") + mysql_error(conn.get()));
    }

    MYSQL_RES* result = mysql_store_result(conn.get());
    if (!result) {
        throw DatabaseException(std::string("获取结果集失败: ") + mysql_error(conn.get()));
    }

    std::optional<Patient> patient;
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        patient = parseRow(row);
    }

    mysql_free_result(result);
    return patient;
}

std::optional<Patient> MySQLPatientRepository::findByIdCard(std::string_view id_card) {
    auto conn = pool_->getConnection();

    std::string escaped = escape(conn, id_card);
    std::ostringstream sql;
    sql << "SELECT id, name, phone, id_card, age, gender, "
        << "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at "
        << "FROM patients WHERE id_card = '" << escaped << "'";

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("按身份证查询患者失败: ") + mysql_error(conn.get()));
    }

    MYSQL_RES* result = mysql_store_result(conn.get());
    if (!result) {
        throw DatabaseException(std::string("获取结果集失败: ") + mysql_error(conn.get()));
    }

    std::optional<Patient> patient;
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        patient = parseRow(row);
    }

    mysql_free_result(result);
    return patient;
}

Patient MySQLPatientRepository::parseRow(MYSQL_ROW row) {
    Patient p;
    p.id         = row[0] ? std::stoll(row[0]) : 0;
    p.name       = row[1] ? row[1] : "";
    p.phone      = row[2] ? row[2] : "";
    p.id_card    = row[3] ? row[3] : "";
    p.age        = row[4] ? std::stoi(row[4]) : 0;
    p.gender     = row[5] ? row[5] : "";
    p.created_at = row[6] ? row[6] : "";
    return p;
}

std::string MySQLPatientRepository::escape(std::shared_ptr<MYSQL> conn, std::string_view input) {
    std::string buffer(input.length() * 2 + 1, '\0');
    unsigned long len = mysql_real_escape_string(conn.get(),
                                                  buffer.data(),
                                                  input.data(),
                                                  input.length());
    buffer.resize(len);
    return buffer;
}

} // namespace hospital
