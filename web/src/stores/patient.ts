import { defineStore } from 'pinia'
import { ref } from 'vue'
import { patientApi } from '@/api/patient'
import type { Patient } from '@/types'

export const usePatientStore = defineStore('patient', () => {
  // 状态
  const patients = ref<Patient[]>([])
  const currentPatient = ref<Patient | null>(null)
  const searchResults = ref<Patient[]>([])
  const loading = ref(false)

  // 加载所有患者
  async function loadPatients() {
    loading.value = true
    try {
      patients.value = await patientApi.getAll()
    } finally {
      loading.value = false
    }
  }

  // 搜索患者
  async function searchPatients(keyword: string) {
    if (!keyword.trim()) {
      searchResults.value = []
      return
    }
    try {
      searchResults.value = await patientApi.search(keyword)
    } catch {
      searchResults.value = []
    }
  }

  // 根据 ID 获取患者
  async function getPatientById(id: number) {
    return await patientApi.getById(id)
  }

  // 创建患者
  async function createPatient(data: Partial<Patient>) {
    const patient = await patientApi.create(data)
    // 刷新列表
    await loadPatients()
    return patient
  }

  // 选择当前患者
  function selectPatient(patient: Patient | null) {
    currentPatient.value = patient
  }

  // 清空搜索结果
  function clearSearchResults() {
    searchResults.value = []
  }

  return {
    patients,
    currentPatient,
    searchResults,
    loading,
    loadPatients,
    searchPatients,
    getPatientById,
    createPatient,
    selectPatient,
    clearSearchResults
  }
})
