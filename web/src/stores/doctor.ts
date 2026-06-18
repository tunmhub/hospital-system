import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { doctorApi } from '@/api/doctor'
import type { Doctor, Appointment } from '@/types'

export const useDoctorStore = defineStore('doctor', () => {
  // 状态
  const doctors = ref<Doctor[]>([])
  const doctorQueues = ref<Record<number, Appointment[]>>({})
  const loading = ref(false)

  // 计算属性：总等待人数
  const totalWaiting = computed(() => {
    return Object.values(doctorQueues.value).reduce((sum, queue) => {
      return sum + queue.filter(item => item.status === 'waiting').length
    }, 0)
  })

  // 加载所有医生
  async function loadDoctors() {
    loading.value = true
    try {
      doctors.value = await doctorApi.getAll()
    } finally {
      loading.value = false
    }
  }

  // 加载医生队列
  async function loadDoctorQueue(doctorId: number) {
    try {
      const queue = await doctorApi.getQueue(doctorId)
      doctorQueues.value[doctorId] = queue
    } catch {
      doctorQueues.value[doctorId] = []
    }
  }

  // 加载所有医生的队列
  async function loadAllQueues() {
    for (const doc of doctors.value) {
      await loadDoctorQueue(doc.id)
    }
  }

  // 获取医生等待人数
  function getWaitingCount(doctorId: number): number {
    const queue = doctorQueues.value[doctorId] || []
    return queue.filter(item => item.status === 'waiting').length
  }

  // 呼叫下一位患者
  async function callNext(doctorId: number) {
    const result = await doctorApi.callNext(doctorId)
    // 刷新队列
    await loadDoctorQueue(doctorId)
    await loadDoctors()
    return result
  }

  return {
    doctors,
    doctorQueues,
    loading,
    totalWaiting,
    loadDoctors,
    loadDoctorQueue,
    loadAllQueues,
    getWaitingCount,
    callNext
  }
})
