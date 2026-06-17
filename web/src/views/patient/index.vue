<template>
  <div class="container py-4">
    <!-- 返回首页按钮 -->
    <div class="mb-3">
      <router-link to="/" class="btn btn-light">
        <i class="bi bi-arrow-left"></i> 返回首页
      </router-link>
    </div>

    <div class="card h-100 shadow-sm border-0">
      <div class="card-header bg-primary text-white">
        <h5 class="mb-0"><i class="bi bi-person-plus"></i> 患者终端 - 挂号</h5>
      </div>
      <div class="card-body">
        <!-- Patient Info -->
        <h6 class="fw-bold mb-3"><i class="bi bi-person"></i> 患者信息</h6>
        <div class="row mb-3">
          <div class="col-8">
            <input type="text" class="form-control" v-model="patientName" placeholder="患者姓名" required @input="searchPatients">
          </div>
          <div class="col-4">
            <button type="button" class="btn btn-outline-primary w-100" @click="findOrCreatePatient">
              <i class="bi bi-search"></i> 查询/注册
            </button>
          </div>
        </div>

        <!-- 搜索结果下拉 -->
        <div v-if="searchResults.length > 0" class="mb-3">
          <div class="list-group">
            <button v-for="p in searchResults" :key="p.id" class="list-group-item list-group-item-action"
                    @click="selectExistingPatient(p)">
              <i class="bi bi-person-check text-success me-2"></i>
              <strong>{{ p.name }}</strong>
              <span class="ms-2 text-muted">{{ p.phone }}</span>
              <span class="badge bg-secondary ms-2">ID: {{ p.id }}</span>
            </button>
          </div>
        </div>

        <!-- 新患者信息 -->
        <div class="row mb-3">
          <div class="col-6">
            <input type="tel" class="form-control" v-model="phone" placeholder="手机号（新患者必填）">
          </div>
          <div class="col-6">
            <input type="text" class="form-control" v-model="idCard" placeholder="身份证号（可选）">
          </div>
        </div>

        <div v-show="currentPatient.id" class="alert alert-info mb-3">
          <i class="bi bi-person-check"></i> 患者: <strong>{{ currentPatient.name }}</strong>
          <span class="ms-2 badge bg-primary">ID: {{ currentPatient.id }}</span>
        </div>
        <hr>
        
        <!-- Appointment Form -->
        <form @submit.prevent="submitAppointment">
          <div class="mb-3">
            <label class="form-label fw-semibold">就诊科室</label>
            <select class="form-select" v-model="form.department" required>
              <option value="">请选择科室</option>
              <option v-for="dept in departments" :key="dept" :value="dept">{{ dept }}</option>
            </select>
          </div>
          <div class="mb-3">
            <label class="form-label fw-semibold">指定医生（可选不指定，将触发自动分流）</label>
            <select class="form-select" v-model="form.doctorId">
              <option value="">自动分配 (负载最小优先)</option>
              <option v-for="doc in doctors" :key="doc.id" :value="doc.id">
                {{ doc.name }} ({{ doc.department }})
              </option>
            </select>
          </div>
          <div class="mb-3">
            <div class="form-check form-switch">
              <input class="form-check-input" type="checkbox" id="isUrgent" v-model="form.isUrgent">
              <label class="form-check-label fw-semibold" for="isUrgent">
                <i class="bi bi-exclamation-triangle text-warning"></i> 加急就诊
              </label>
            </div>
          </div>
          <div class="mb-3">
            <div class="form-check form-switch">
              <input class="form-check-input" type="checkbox" id="isEmergency" v-model="form.isEmergency">
              <label class="form-check-label fw-semibold" for="isEmergency">
                <i class="bi bi-lightning-charge text-danger"></i> 急诊
              </label>
            </div>
          </div>
          <button type="submit" class="btn btn-primary w-100" :disabled="!currentPatient.id">
            <i class="bi bi-check-circle"></i> 确认挂号
          </button>
        </form>
        
        <hr>
        <h6 class="fw-bold mb-3"><i class="bi bi-people"></i> [开发调试用]快速选择已有患者</h6>
        <div class="d-flex flex-wrap gap-2">
          <button v-for="p in patientList" :key="p.id" class="btn btn-outline-secondary btn-sm" @click="selectPatient(p)">
            {{ p.name }} ({{ p.id }})
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import request from '@/utils/request'
import Swal from 'sweetalert2'
import type { Patient, Doctor, Appointment, MakeAppointmentRequest } from '@/types'

const departments = ['内科', '外科', '妇产科', '儿科', '急诊科', '眼科', '口腔科', '皮肤科']
const doctors = ref<Doctor[]>([])
const patientList = ref<Patient[]>([])
const searchResults = ref<Patient[]>([])

const patientName = ref('')
const phone = ref('')
const idCard = ref('')
const currentPatient = reactive<Partial<Patient>>({ id: 0, name: '' })

const form = reactive({
  department: '',
  doctorId: '',
  isUrgent: false,
  isEmergency: false
})

const loadDoctors = async () => {
  try {
    doctors.value = await request.get('/doctors')
  } catch (e) {}
}

const loadPatients = async () => {
  try {
    patientList.value = await request.get('/patients')
  } catch (e) {}
}

onMounted(() => {
  loadDoctors()
  loadPatients()
})

// 搜索患者
const searchPatients = async () => {
  if (!patientName.value.trim()) {
    searchResults.value = []
    return
  }
  try {
    searchResults.value = await request.get(`/patients?search=${encodeURIComponent(patientName.value)}`)
  } catch (e) {
    searchResults.value = []
  }
}

// 选择已有患者
const selectExistingPatient = (patient: Patient) => {
  selectPatient(patient)
  searchResults.value = []
  phone.value = patient.phone || ''
  idCard.value = patient.id_card || ''
}

const findOrCreatePatient = async () => {
  if (!patientName.value.trim()) {
    Swal.fire({ icon: 'warning', title: '请输入患者姓名' })
    return
  }

  // 先搜索
  await searchPatients()

  // 如果找到匹配的患者，提示选择
  if (searchResults.value.length > 0) {
    const html = searchResults.value.map(p =>
      `<button class="btn btn-outline-primary btn-sm m-1" onclick="window.__selectPatient(${p.id})">${p.name} (${p.phone})</button>`
    ).join('')

    Swal.fire({
      title: '找到已有患者',
      html: `<p>请选择已有患者或点击"新建"创建新患者：</p><div class="mt-2">${html}</div>`,
      showCancelButton: true,
      confirmButtonText: '新建患者',
      cancelButtonText: '取消',
      confirmButtonColor: '#10b981'
    }).then(async (result) => {
      if (result.isConfirmed) {
        await createNewPatient()
      }
    })
    return
  }

  // 没找到，直接创建
  await createNewPatient()
}

// 暴露给全局供 Swal 按钮调用
;(window as any).__selectPatient = (id: number) => {
  const patient = searchResults.value.find(p => p.id === id)
  if (patient) {
    selectExistingPatient(patient)
    Swal.close()
  }
}

const createNewPatient = async () => {
  try {
    const patient: Patient = await request.post('/patients', {
      name: patientName.value,
      phone: phone.value || '13800000000',
      id_card: idCard.value || '',
      age: 0,
      gender: ''
    })

    selectPatient(patient)
    Swal.fire({
      icon: 'success',
      title: '患者注册成功',
      text: `患者ID: ${patient.id}`,
      timer: 1500,
      showConfirmButton: false
    })
    loadPatients()
  } catch (e) {}
}

const selectPatient = (p: Partial<Patient>) => {
  currentPatient.id = p.id
  currentPatient.name = p.name
  patientName.value = p.name || ''
}

const submitAppointment = async () => {
  let priority: 'normal' | 'urgent' | 'emergency' = 'normal'
  if (form.isEmergency) priority = 'emergency'
  else if (form.isUrgent) priority = 'urgent'

  const body: MakeAppointmentRequest = {
    patient_id: currentPatient.id!,
    priority
  }

  if (form.doctorId) {
    body.doctor_id = parseInt(form.doctorId, 10)
  } else {
    body.department = form.department
  }

  try {
    const endpoint = form.doctorId ? '/appointments' : '/appointments/auto'
    const result: Appointment = await request.post(endpoint, body)

    // 如果不指定医生的话，后端会自动分配一个
    Swal.fire({
      icon: 'success',
      title: '挂号成功',
      html: `<div class="text-start mt-3">
               <p><strong>挂号单号：</strong>${result.id || 'N/A'}</p>
               <p><strong>排队流水：</strong><span class="fs-4 text-primary fw-bold">#${result.queue_number}</span></p>
               ${result.doctor_name ? `<p><strong>分配医生：</strong>${result.doctor_name}</p>` : ''}
               ${result.department ? `<p><strong>就诊科室：</strong>${result.department}</p>` : ''}
             </div>`,
      confirmButtonColor: '#2563eb'
    })

    // 刷新医生状态
    loadDoctors()
  } catch (e) {}
}
</script>
