import request from '@/utils/request'
import type { Appointment, SettlementResponse, MakeAppointmentRequest, WaitTimeResponse, ApiMessage } from '@/types'

export const appointmentApi = {
  // 创建挂号（指定医生）
  create(data: MakeAppointmentRequest) {
    return request.post<Appointment>('/appointments', data)
  },

  // 自动分流挂号（不指定医生）
  autoRoute(data: MakeAppointmentRequest) {
    return request.post<Appointment>('/appointments/auto', data)
  },

  // 取消挂号（退号）
  cancel(id: number) {
    return request.post<ApiMessage>(`/appointments/${id}/cancel`)
  },

  // 完成就诊
  complete(id: number) {
    return request.post<ApiMessage>(`/appointments/${id}/complete`)
  },

  // 医保结算
  settle(id: number) {
    return request.post<SettlementResponse>(`/appointments/${id}/settle`)
  },

  // 获取等待时间
  getWaitTime(id: number) {
    return request.get<WaitTimeResponse>(`/appointments/${id}/wait_time`)
  },

  // 获取患者挂号记录
  getByPatient(patientId: number) {
    return request.get<Appointment[]>(`/patients/${patientId}/appointments`)
  },

  // 获取医生队列
  getByDoctor(doctorId: number) {
    return request.get<Appointment[]>(`/doctors/${doctorId}/queue`)
  }
}
