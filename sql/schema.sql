-- ============================================================
-- 医院挂号系统 - 数据库建表脚本
-- 数据库: MySQL 5.7+ / MariaDB 10.3+
-- 字符集: utf8mb4（支持 emoji 和中文）
-- ============================================================

-- 创建数据库（如果不存在）
CREATE DATABASE IF NOT EXISTS hospital_db
    DEFAULT CHARACTER SET utf8mb4
    DEFAULT COLLATE utf8mb4_unicode_ci;

USE hospital_db;

-- ============================================================
-- 患者表
-- ============================================================
CREATE TABLE IF NOT EXISTS patients (
    id          BIGINT AUTO_INCREMENT PRIMARY KEY COMMENT '患者ID',
    name        VARCHAR(50)  NOT NULL COMMENT '姓名',
    phone       VARCHAR(20)  NOT NULL COMMENT '手机号',
    id_card     VARCHAR(18)  NOT NULL COMMENT '身份证号',
    age         INT          NOT NULL DEFAULT 0 COMMENT '年龄',
    gender      VARCHAR(4)   NOT NULL DEFAULT '' COMMENT '性别',
    created_at  DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',

    UNIQUE INDEX uk_phone   (phone),
    UNIQUE INDEX uk_id_card (id_card)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='患者表';

-- ============================================================
-- 科室表
-- ============================================================
CREATE TABLE IF NOT EXISTS departments (
    id          BIGINT AUTO_INCREMENT PRIMARY KEY COMMENT '科室ID',
    name        VARCHAR(50)  NOT NULL COMMENT '科室名称',
    location    VARCHAR(100) NOT NULL DEFAULT '' COMMENT '位置（楼层/区域）',
    description TEXT COMMENT '科室描述',

    UNIQUE INDEX uk_name (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='科室表';

-- ============================================================
-- 医生表
-- ============================================================
CREATE TABLE IF NOT EXISTS doctors (
    id               BIGINT AUTO_INCREMENT PRIMARY KEY COMMENT '医生ID',
    name             VARCHAR(50)  NOT NULL COMMENT '姓名',
    department       VARCHAR(50)  NOT NULL COMMENT '所属科室',
    title            VARCHAR(20)  NOT NULL DEFAULT '' COMMENT '职称',
    max_patients     INT          NOT NULL DEFAULT 30 COMMENT '每日最大接诊量',
    current_patients INT          NOT NULL DEFAULT 0 COMMENT '当前已接诊人数',

    INDEX idx_department (department)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='医生表';

-- ============================================================
-- 挂号表
-- ============================================================
CREATE TABLE IF NOT EXISTS appointments (
    id          BIGINT AUTO_INCREMENT PRIMARY KEY COMMENT '挂号ID',
    patient_id  BIGINT       NOT NULL COMMENT '患者ID',
    doctor_id   BIGINT       NOT NULL COMMENT '医生ID',
    status      VARCHAR(20)  NOT NULL DEFAULT 'waiting' COMMENT '状态: waiting/in_progress/completed/cancelled',
    priority    VARCHAR(10)  NOT NULL DEFAULT 'normal' COMMENT '优先级: normal/urgent/emergency',
    queue_number INT         NOT NULL DEFAULT 0 COMMENT '排队号码',
    created_at  DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '挂号时间',

    INDEX idx_patient  (patient_id),
    INDEX idx_doctor   (doctor_id),
    INDEX idx_status   (status),

    CONSTRAINT fk_appt_patient FOREIGN KEY (patient_id) REFERENCES patients(id),
    CONSTRAINT fk_appt_doctor  FOREIGN KEY (doctor_id)  REFERENCES doctors(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='挂号表';

-- ============================================================
-- 初始化科室数据
-- ============================================================
INSERT INTO departments (name, location, description) VALUES
    ('内科',   '门诊楼2层', '内科常见疾病诊治'),
    ('外科',   '门诊楼3层', '外科手术及创伤处理'),
    ('儿科',   '门诊楼1层', '儿童及青少年疾病'),
    ('妇产科', '门诊楼4层', '妇科及产科诊疗'),
    ('急诊科', '急诊楼1层', '24小时急诊服务'),
    ('骨科',   '门诊楼3层', '骨骼及关节疾病'),
    ('眼科',   '门诊楼2层', '眼部疾病诊治'),
    ('耳鼻喉科', '门诊楼2层', '耳鼻喉相关疾病')
ON DUPLICATE KEY UPDATE name = name;
