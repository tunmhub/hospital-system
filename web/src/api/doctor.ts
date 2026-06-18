import request from '@/utils/request'
import type { Doctor, Appointment } from '@/types'

export const doctorApi = {
  // 获取所有医生
  getAll() {
    return request.get<Doctor[]>('/doctors')
  },

  // 根据 ID 获取医生
  getById(id: number) {
    return request.get<Doctor>(`/doctors/${id}`)
  },

  // 获取医生的排队队列
  getQueue(doctorId: number) {
    return request.get<Appointment[]>(`/doctors/${doctorId}/queue`)
  },

  // 呼叫下一位患者
  callNext(doctorId: number) {
    return request.post<Appointment>(`/doctors/${doctorId}/call_next`)
  }
}
