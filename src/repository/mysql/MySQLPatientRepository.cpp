#include "repository/mysql/MySQLPatientRepository.h"
#include "common/Exception.h"

#include <iostream>
#include <sstream>
#include <cstring>
#include <iomanip>

namespace hospital {

MySQLPatientRepository::MySQLPatientRepository(std::shared_ptr<MySQLConnectionPool> pool)
    : pool_(std::move(pool)) {}

std::optional<Patient> MySQLPatientRepository::findById(int64_t id) {
    auto conn = pool_->getConnection();

    std::ostringstream sql;
    sql << "SELECT id, name, medical_record_no, phone, id_card, age, gender, insurance_type, "
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

    const char* sql = "SELECT id, name, medical_record_no, phone, id_card, age, gender, insurance_type, "
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

    // 验证必填字段
    if (entity.name.empty()) {
        throw ValidationException("患者姓名不能为空");
    }
    if (entity.phone.empty()) {
        throw ValidationException("手机号不能为空");
    }
    if (entity.id_card.empty()) {
        throw ValidationException("身份证号不能为空");
    }
    if (entity.age <= 0) {
        throw ValidationException("年龄必须大于0");
    }
    if (entity.gender.empty()) {
        throw ValidationException("性别不能为空");
    }

    // 先插入记录，病历号暂时为空
    std::string escaped_name  = escape(conn, entity.name);
    std::string escaped_phone = escape(conn, entity.phone);
    std::string escaped_idcard = escape(conn, entity.id_card);
    std::string escaped_gender = escape(conn, entity.gender);
    const char* insurance_str = insuranceTypeToString(entity.insurance_type);

    std::ostringstream sql;
    sql << "INSERT INTO patients (name, medical_record_no, phone, id_card, age, gender, insurance_type) VALUES ('"
        << escaped_name << "', '', '"
        << escaped_phone << "', '"
        << escaped_idcard << "', "
        << entity.age << ", '"
        << escaped_gender << "', '"
        << insurance_str << "')";

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("插入患者失败: ") + mysql_error(conn.get()));
    }

    // 获取自增 ID
    entity.id = static_cast<int64_t>(mysql_insert_id(conn.get()));

    // 用 ID 生成病历号并更新
    std::ostringstream mr_no;
    mr_no << "MR" << std::setw(8) << std::setfill('0') << entity.id;
    entity.medical_record_no = mr_no.str();

    std::string escaped_mr_no = escape(conn, entity.medical_record_no);
    std::ostringstream update_sql;
    update_sql << "UPDATE patients SET medical_record_no = '" << escaped_mr_no << "' WHERE id = " << entity.id;

    if (mysql_query(conn.get(), update_sql.str().c_str()) != 0) {
        // 更新失败不影响主流程，只记录日志
        std::cerr << "[WARN] 更新病历号失败: " << mysql_error(conn.get()) << std::endl;
    }

    return true;
}

bool MySQLPatientRepository::update(const Patient& entity) {
    auto conn = pool_->getConnection();

    std::string escaped_name  = escape(conn, entity.name);
    std::string escaped_mr_no = escape(conn, entity.medical_record_no);
    std::string escaped_phone = escape(conn, entity.phone);
    std::string escaped_idcard = escape(conn, entity.id_card);
    std::string escaped_gender = escape(conn, entity.gender);
    const char* insurance_str = insuranceTypeToString(entity.insurance_type);

    std::ostringstream sql;
    sql << "UPDATE patients SET "
        << "name = '"     << escaped_name   << "', "
        << "medical_record_no = '" << escaped_mr_no << "', "
        << "phone = '"    << escaped_phone  << "', "
        << "id_card = '"  << escaped_idcard << "', "
        << "age = "       << entity.age     << ", "
        << "gender = '"   << escaped_gender << "', "
        << "insurance_type = '" << insurance_str << "' "
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
    sql << "SELECT id, name, medical_record_no, phone, id_card, age, gender, insurance_type, "
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
    sql << "SELECT id, name, medical_record_no, phone, id_card, age, gender, insurance_type, "
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

std::vector<Patient> MySQLPatientRepository::searchByName(std::string_view keyword) {
    auto conn = pool_->getConnection();

    std::string escaped = escape(conn, keyword);
    std::ostringstream sql;
    sql << "SELECT id, name, medical_record_no, phone, id_card, age, gender, insurance_type, "
        << "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at "
        << "FROM patients WHERE name LIKE '%" << escaped << "%' LIMIT 10";

    if (mysql_query(conn.get(), sql.str().c_str()) != 0) {
        throw DatabaseException(std::string("搜索患者失败: ") + mysql_error(conn.get()));
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

Patient MySQLPatientRepository::parseRow(MYSQL_ROW row) {
    Patient p;
    p.id                 = row[0] ? std::stoll(row[0]) : 0;
    p.name               = row[1] ? row[1] : "";
    p.medical_record_no  = row[2] ? row[2] : "";
    p.phone              = row[3] ? row[3] : "";
    p.id_card            = row[4] ? row[4] : "";
    p.age                = row[5] ? std::stoi(row[5]) : 0;
    p.gender             = row[6] ? row[6] : "";
    p.insurance_type     = row[7] ? stringToInsuranceType(row[7]) : InsuranceType::Self;
    p.created_at         = row[8] ? row[8] : "";
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
