// ============================================================
// 医院挂号系统 - TypeScript 类型定义
// ============================================================

// 医保类型
export type InsuranceType = 'urban_worker' | 'urban_resident' | 'rural_resident' | 'self'

// 患者类型
export interface Patient {
  id: number
  name: string
  medical_record_no?: string
  phone: string
  id_card?: string
  age?: number
  gender?: string
  insurance_type?: InsuranceType
  created_at?: string
}

// 医生类型
export interface Doctor {
  id: number
  name: string
  department: string
  title?: string
  work_start?: string
  work_end?: string
  max_patients: number
  current_patients: number
}

// 挂号状态
export type AppointmentStatus = 'waiting' | 'in_progress' | 'completed' | 'cancelled'

// 优先级
export type Priority = 'normal' | 'urgent' | 'emergency'

// 挂号类型
export interface Appointment {
  id: number
  patient_id: number
  patient_name?: string
  doctor_id: number
  doctor_name?: string
  department?: string
  status: AppointmentStatus
  priority: Priority
  queue_number: number
  registration_fee?: number
  insurance_fee?: number
  self_fee?: number
  settled?: boolean
  created_at?: string
}

// 等待时间响应
export interface WaitTimeResponse {
  appointment_id: number
  wait_minutes: number
}

// 挂号请求
export interface MakeAppointmentRequest {
  patient_id: number
  doctor_id?: number
  department?: string
  priority?: Priority
}

// 结算响应
export interface SettlementResponse {
  id: number
  patient_id: number
  patient_name: string
  insurance_type: InsuranceType
  registration_fee: number
  insurance_fee: number
  self_fee: number
  settled: boolean
}

// 科室统计
export interface DepartmentStat {
  department: string
  waiting_count: number
  estimated_wait_minutes: number
}

// API 错误响应
export interface ApiError {
  error: string
}

// API 成功消息
export interface ApiMessage {
  message: string
}
