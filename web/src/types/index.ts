// 患者类型
export interface Patient {
  id: number
  name: string
  phone: string
  id_card?: string
  age?: number
  gender?: string
  created_at?: string
}

// 医生类型
export interface Doctor {
  id: number
  name: string
  department: string
  title?: string
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

// API 错误响应
export interface ApiError {
  error: string
}
