import { defineStore } from 'pinia'
import { ref } from 'vue'
import { appointmentApi } from '@/api/appointment'
import type { Appointment, MakeAppointmentRequest } from '@/types'

export const useAppointmentStore = defineStore('appointment', () => {
  // 状态
  const currentAppointment = ref<Appointment | null>(null)
  const patientAppointments = ref<Appointment[]>([])
  const doctorQueue = ref<Appointment[]>([])
  const loading = ref(false)

  // 创建挂号
  async function makeAppointment(data: MakeAppointmentRequest) {
    loading.value = true
    try {
      const result = data.doctor_id
        ? await appointmentApi.create(data)
        : await appointmentApi.autoRoute(data)
      currentAppointment.value = result
      return result
    } finally {
      loading.value = false
    }
  }

  // 退号（取消挂号）
  async function cancelAppointment(id: number) {
    const result = await appointmentApi.cancel(id)
    // 如果取消的是当前挂号，清空
    if (currentAppointment.value?.id === id) {
      currentAppointment.value = null
    }
    return result
  }

  // 完成就诊
  async function completeAppointment(id: number) {
    return await appointmentApi.complete(id)
  }

  // 医保结算
  async function settleAppointment(id: number) {
    return await appointmentApi.settle(id)
  }

  // 获取等待时间
  async function getWaitTime(id: number) {
    return await appointmentApi.getWaitTime(id)
  }

  // 加载患者历史挂号记录
  async function loadPatientHistory(patientId: number) {
    loading.value = true
    try {
      patientAppointments.value = await appointmentApi.getByPatient(patientId)
    } finally {
      loading.value = false
    }
  }

  // 加载医生队列
  async function loadDoctorQueue(doctorId: number) {
    loading.value = true
    try {
      doctorQueue.value = await appointmentApi.getByDoctor(doctorId)
    } finally {
      loading.value = false
    }
  }

  // 清空当前挂号
  function clearCurrentAppointment() {
    currentAppointment.value = null
  }

  // 清空患者历史
  function clearPatientHistory() {
    patientAppointments.value = []
  }

  return {
    currentAppointment,
    patientAppointments,
    doctorQueue,
    loading,
    makeAppointment,
    cancelAppointment,
    completeAppointment,
    settleAppointment,
    getWaitTime,
    loadPatientHistory,
    loadDoctorQueue,
    clearCurrentAppointment,
    clearPatientHistory
  }
})
