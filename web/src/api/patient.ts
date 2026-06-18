import request from '@/utils/request'
import type { Patient } from '@/types'

export const patientApi = {
  // 获取所有患者
  getAll() {
    return request.get<Patient[]>('/patients')
  },

  // 搜索患者
  search(keyword: string) {
    return request.get<Patient[]>(`/patients?search=${encodeURIComponent(keyword)}`)
  },

  // 根据 ID 获取患者
  getById(id: number) {
    return request.get<Patient>(`/patients/${id}`)
  },

  // 创建患者
  create(data: Partial<Patient>) {
    return request.post<Patient>('/patients', data)
  }
}
