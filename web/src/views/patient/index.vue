<template>
  <div class="container py-4">
    <!-- 返回首页按钮 -->
    <div class="mb-3">
      <router-link to="/" class="btn btn-light">
        <i class="bi bi-arrow-left"></i> 返回首页
      </router-link>
    </div>

    <!-- 患者信息卡片 -->
    <div class="card shadow-sm border-0 mb-4">
      <div class="card-header bg-primary text-white">
        <h5 class="mb-0"><i class="bi bi-person-plus"></i> 患者终端 - 挂号</h5>
      </div>
      <div class="card-body">
        <!-- 患者查询/注册 -->
        <h6 class="fw-bold mb-3"><i class="bi bi-person"></i> 患者信息</h6>
        <div class="row mb-3">
          <div class="col-8">
            <input
              type="text"
              class="form-control"
              v-model="patientName"
              placeholder="患者姓名"
              required
              @input="onSearchInput"
            />
          </div>
          <div class="col-4">
            <button
              type="button"
              class="btn btn-outline-primary w-100"
              @click="findOrCreatePatient"
            >
              <i class="bi bi-search"></i> 查询/注册
            </button>
          </div>
        </div>

        <!-- 搜索结果下拉 -->
        <div v-if="patientStore.searchResults.length > 0" class="mb-3">
          <div class="list-group">
            <button
              v-for="p in patientStore.searchResults"
              :key="p.id"
              class="list-group-item list-group-item-action"
              @click="selectExistingPatient(p)"
            >
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
            <label class="form-label fw-semibold">手机号 <span class="text-danger">*</span></label>
            <input
              type="tel"
              class="form-control"
              v-model="phone"
              placeholder="请输入手机号"
              required
            />
          </div>
          <div class="col-6">
            <label class="form-label fw-semibold">身份证号 <span class="text-danger">*</span></label>
            <input
              type="text"
              class="form-control"
              v-model="idCard"
              placeholder="请输入身份证号"
              required
            />
          </div>
        </div>
        <div class="row mb-3">
          <div class="col-4">
            <label class="form-label fw-semibold">年龄 <span class="text-danger">*</span></label>
            <input
              type="number"
              class="form-control"
              v-model="age"
              placeholder="请输入年龄"
              min="1"
              required
            />
          </div>
          <div class="col-4">
            <label class="form-label fw-semibold">性别 <span class="text-danger">*</span></label>
            <select class="form-select" v-model="gender" required>
              <option value="">请选择性别</option>
              <option value="男">男</option>
              <option value="女">女</option>
            </select>
          </div>
          <div class="col-4">
            <label class="form-label fw-semibold">医保类型 <span class="text-danger">*</span></label>
            <select class="form-select" v-model="insuranceType" required>
              <option value="">请选择医保类型</option>
              <option value="urban_worker">城镇职工医保 (80%)</option>
              <option value="urban_resident">城镇居民医保 (60%)</option>
              <option value="rural_resident">新农合医保 (60%)</option>
              <option value="self">自费 (0%)</option>
            </select>
          </div>
        </div>

        <!-- 当前选中患者 -->
        <div v-if="patientStore.currentPatient" class="alert alert-info mb-3">
          <i class="bi bi-person-check"></i> 患者:
          <strong>{{ patientStore.currentPatient.name }}</strong>
          <span class="ms-2 badge bg-primary">ID: {{ patientStore.currentPatient.id }}</span>
          <span class="ms-2 badge bg-secondary">{{ patientStore.currentPatient.gender }}</span>
          <span class="ms-2 badge bg-info">{{ getInsuranceTypeText(patientStore.currentPatient.insurance_type || 'self') }}</span>
        </div>

        <hr />

        <!-- 挂号表单 -->
        <form @submit.prevent="submitAppointment">
          <div class="mb-3">
            <label class="form-label fw-semibold">就诊科室</label>
            <select class="form-select" v-model="form.department" required>
              <option value="">请选择科室</option>
              <option v-for="dept in departments" :key="dept" :value="dept">
                {{ dept }}
              </option>
            </select>
          </div>
          <div class="mb-3">
            <label class="form-label fw-semibold">指定医生（可选不指定，将触发自动分流）</label>
            <select class="form-select" v-model="form.doctorId">
              <option value="">自动分配 (负载最小优先)</option>
              <option v-for="doc in filteredDoctors" :key="doc.id" :value="doc.id">
                {{ doc.name }} ({{ doc.title || '医师' }})
              </option>
            </select>
          </div>
          <div class="mb-3">
            <div class="form-check form-switch">
              <input
                class="form-check-input"
                type="checkbox"
                id="isUrgent"
                v-model="form.isUrgent"
              />
              <label class="form-check-label fw-semibold" for="isUrgent">
                <i class="bi bi-exclamation-triangle text-warning"></i> 加急就诊
              </label>
            </div>
          </div>
          <div class="mb-3">
            <div class="form-check form-switch">
              <input
                class="form-check-input"
                type="checkbox"
                id="isEmergency"
                v-model="form.isEmergency"
              />
              <label class="form-check-label fw-semibold" for="isEmergency">
                <i class="bi bi-lightning-charge text-danger"></i> 急诊
              </label>
            </div>
          </div>
          <button
            type="submit"
            class="btn btn-primary w-100"
            :disabled="!patientStore.currentPatient"
          >
            <i class="bi bi-check-circle"></i> 确认挂号
          </button>
        </form>
      </div>
    </div>

    <!-- 挂号成功卡片 -->
    <div v-if="appointmentStore.currentAppointment" class="card shadow-sm border-0 mb-4">
      <div class="card-header bg-success text-white">
        <h5 class="mb-0"><i class="bi bi-check-circle"></i> 挂号成功</h5>
      </div>
      <div class="card-body">
        <div class="row">
          <div class="col-md-6">
            <p><strong>挂号单号：</strong>{{ appointmentStore.currentAppointment.id }}</p>
            <p>
              <strong>排队号码：</strong>
              <span class="fs-4 text-primary fw-bold">
                #{{ appointmentStore.currentAppointment.queue_number }}
              </span>
            </p>
            <p>
              <strong>优先级：</strong>
              <span :class="getPriorityClass(appointmentStore.currentAppointment.priority)">
                {{ getPriorityText(appointmentStore.currentAppointment.priority) }}
              </span>
            </p>
          </div>
          <div class="col-md-6">
            <p>
              <strong>分配医生：</strong>{{ appointmentStore.currentAppointment.doctor_name || '-' }}
            </p>
            <p>
              <strong>就诊科室：</strong>{{ appointmentStore.currentAppointment.department || '-' }}
            </p>
            <p>
              <strong>挂号时间：</strong>{{ appointmentStore.currentAppointment.created_at || '-' }}
            </p>
          </div>
        </div>
        <hr />
        <div class="d-flex gap-2">
          <button class="btn btn-danger" @click="handleCancel">
            <i class="bi bi-x-circle"></i> 退号
          </button>
          <button class="btn btn-warning" @click="handleSettle">
            <i class="bi bi-cash-coin"></i> 医保结算
          </button>
        </div>
      </div>
    </div>

    <!-- 历史就诊记录 -->
    <div v-if="patientStore.currentPatient" class="card shadow-sm border-0">
      <div class="card-header bg-info text-white d-flex justify-content-between align-items-center">
        <h5 class="mb-0"><i class="bi bi-clock-history"></i> 历史就诊记录</h5>
        <button class="btn btn-sm btn-light" @click="refreshHistory">
          <i class="bi bi-arrow-clockwise"></i> 刷新
        </button>
      </div>
      <div class="card-body">
        <div v-if="appointmentStore.patientAppointments.length === 0" class="text-center py-4">
          <i class="bi bi-inbox text-muted" style="font-size: 2rem;"></i>
          <p class="text-muted mt-2">暂无就诊记录</p>
        </div>
        <div v-else class="table-responsive">
          <table class="table table-hover">
            <thead>
              <tr>
                <th>挂号单号</th>
                <th>排队号码</th>
                <th>医生</th>
                <th>科室</th>
                <th>优先级</th>
                <th>状态</th>
                <th>挂号时间</th>
                <th>操作</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="apt in appointmentStore.patientAppointments" :key="apt.id">
                <td>{{ apt.id }}</td>
                <td>#{{ apt.queue_number }}</td>
                <td>{{ apt.doctor_name || '-' }}</td>
                <td>{{ apt.department || '-' }}</td>
                <td>
                  <span :class="getPriorityClass(apt.priority)">
                    {{ getPriorityText(apt.priority) }}
                  </span>
                </td>
                <td>
                  <span :class="getStatusClass(apt.status)">
                    {{ getStatusText(apt.status) }}
                  </span>
                </td>
                <td>{{ apt.created_at || '-' }}</td>
                <td>
                  <div class="btn-group btn-group-sm">
                    <button
                      v-if="apt.status === 'waiting'"
                      class="btn btn-outline-danger"
                      @click="handleCancelHistory(apt)"
                    >
                      退号
                    </button>
                    <button
                      v-if="apt.status === 'completed' && !apt.settled"
                      class="btn btn-outline-warning"
                      @click="handleSettleHistory(apt)"
                    >
                      结算
                    </button>
                    <span v-if="apt.settled" class="badge bg-success">已结算</span>
                  </div>
                </td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, onMounted } from 'vue'
import Swal from 'sweetalert2'
import { usePatientStore } from '@/stores/patient'
import { useDoctorStore } from '@/stores/doctor'
import { useAppointmentStore } from '@/stores/appointment'
import type { Patient, Appointment, Priority } from '@/types'

const patientStore = usePatientStore()
const doctorStore = useDoctorStore()
const appointmentStore = useAppointmentStore()

const departments = ['内科', '外科', '妇产科', '儿科', '急诊科', '眼科', '口腔科', '皮肤科']

const patientName = ref('')
const phone = ref('')
const idCard = ref('')
const age = ref<number | null>(null)
const gender = ref('')
const insuranceType = ref('')

const form = reactive({
  department: '',
  doctorId: '',
  isUrgent: false,
  isEmergency: false
})

// 根据科室过滤医生
const filteredDoctors = computed(() => {
  if (!form.department) return doctorStore.doctors
  return doctorStore.doctors.filter(doc => doc.department === form.department)
})

// 搜索输入防抖
let searchTimer: ReturnType<typeof setTimeout> | null = null
const onSearchInput = () => {
  if (searchTimer) clearTimeout(searchTimer)
  searchTimer = setTimeout(() => {
    patientStore.searchPatients(patientName.value)
  }, 300)
}

// 选择已有患者
const selectExistingPatient = (patient: Patient) => {
  patientStore.selectPatient(patient)
  patientStore.clearSearchResults()
  patientName.value = patient.name
  phone.value = patient.phone || ''
  idCard.value = patient.id_card || ''
  age.value = patient.age || null
  gender.value = patient.gender || ''
  insuranceType.value = patient.insurance_type || ''
  refreshHistory()
}

// 查询或创建患者
const findOrCreatePatient = async () => {
  if (!patientName.value.trim()) {
    Swal.fire({ icon: 'warning', title: '请输入患者姓名' })
    return
  }

  await patientStore.searchPatients(patientName.value)

  if (patientStore.searchResults.length > 0) {
    const html = patientStore.searchResults
      .map(
        (p) =>
          `<button class="btn btn-outline-primary btn-sm m-1" onclick="window.__selectPatient(${p.id})">${p.name} (${p.phone})</button>`
      )
      .join('')

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

  await createNewPatient()
}

// 暴露给全局供 Swal 按钮调用
;(window as any).__selectPatient = (id: number) => {
  const patient = patientStore.searchResults.find((p) => p.id === id)
  if (patient) {
    selectExistingPatient(patient)
    Swal.close()
  }
}

// 创建新患者
const createNewPatient = async () => {
  // 验证必填字段
  if (!patientName.value.trim()) {
    Swal.fire({ icon: 'warning', title: '请输入患者姓名' })
    return
  }
  if (!phone.value.trim()) {
    Swal.fire({ icon: 'warning', title: '请输入手机号' })
    return
  }
  if (!idCard.value.trim()) {
    Swal.fire({ icon: 'warning', title: '请输入身份证号' })
    return
  }
  if (!age.value || age.value <= 0) {
    Swal.fire({ icon: 'warning', title: '请输入有效年龄' })
    return
  }
  if (!gender.value) {
    Swal.fire({ icon: 'warning', title: '请选择性别' })
    return
  }
  if (!insuranceType.value) {
    Swal.fire({ icon: 'warning', title: '请选择医保类型' })
    return
  }

  try {
    const patient = await patientStore.createPatient({
      name: patientName.value,
      phone: phone.value,
      id_card: idCard.value,
      age: age.value,
      gender: gender.value,
      insurance_type: insuranceType.value as any
    })

    selectExistingPatient(patient)
    Swal.fire({
      icon: 'success',
      title: '患者注册成功',
      text: `患者ID: ${patient.id}`,
      timer: 1500,
      showConfirmButton: false
    })
  } catch (e) {
    // 错误已由 request.ts 处理
  }
}

// 提交挂号
const submitAppointment = async () => {
  if (!patientStore.currentPatient) return

  let priority: Priority = 'normal'
  if (form.isEmergency) priority = 'emergency'
  else if (form.isUrgent) priority = 'urgent'

  const data = {
    patient_id: patientStore.currentPatient.id,
    doctor_id: form.doctorId ? parseInt(form.doctorId, 10) : undefined,
    department: form.department,
    priority
  }

  try {
    const result = await appointmentStore.makeAppointment(data)

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

    // 刷新历史记录
    refreshHistory()
    doctorStore.loadDoctors()
  } catch (e) {
    // 错误已由 request.ts 处理
  }
}

// 退号
const handleCancel = async () => {
  if (!appointmentStore.currentAppointment) return

  const result = await Swal.fire({
    title: '确认退号？',
    text: `挂号单号 ${appointmentStore.currentAppointment.id} 将被取消`,
    icon: 'warning',
    showCancelButton: true,
    confirmButtonColor: '#dc3545',
    cancelButtonColor: '#6c757d',
    confirmButtonText: '确认退号',
    cancelButtonText: '取消'
  })

  if (result.isConfirmed) {
    try {
      await appointmentStore.cancelAppointment(appointmentStore.currentAppointment.id)
      Swal.fire({
        icon: 'success',
        title: '退号成功',
        timer: 1500,
        showConfirmButton: false
      })
      refreshHistory()
      doctorStore.loadDoctors()
    } catch (e) {
      // 错误已由 request.ts 处理
    }
  }
}

// 医保结算
const handleSettle = async () => {
  if (!appointmentStore.currentAppointment) return

  try {
    const result = await appointmentStore.settleAppointment(appointmentStore.currentAppointment.id)

    Swal.fire({
      icon: 'info',
      title: '医保结算完成',
      html: `<div class="text-start mt-3">
               <p><strong>患者姓名：</strong>${result.patient_name}</p>
               <p><strong>医保类型：</strong>${getInsuranceTypeText(result.insurance_type)}</p>
               <hr>
               <p><strong>挂号费：</strong>¥${result.registration_fee.toFixed(2)}</p>
               <p><strong>医保报销：</strong>¥${result.insurance_fee.toFixed(2)}</p>
               <p><strong>自费金额：</strong><span class="text-danger fw-bold">¥${result.self_fee.toFixed(2)}</span></p>
             </div>`,
      confirmButtonColor: '#10b981'
    })

    refreshHistory()
  } catch (e) {
    // 错误已由 request.ts 处理
  }
}

// 历史记录中的退号
const handleCancelHistory = async (apt: Appointment) => {
  const result = await Swal.fire({
    title: '确认退号？',
    text: `挂号单号 ${apt.id} 将被取消`,
    icon: 'warning',
    showCancelButton: true,
    confirmButtonColor: '#dc3545',
    cancelButtonColor: '#6c757d',
    confirmButtonText: '确认退号',
    cancelButtonText: '取消'
  })

  if (result.isConfirmed) {
    try {
      await appointmentStore.cancelAppointment(apt.id)
      Swal.fire({
        icon: 'success',
        title: '退号成功',
        timer: 1500,
        showConfirmButton: false
      })
      refreshHistory()
      doctorStore.loadDoctors()
    } catch (e) {
      // 错误已由 request.ts 处理
    }
  }
}

// 历史记录中的结算
const handleSettleHistory = async (apt: Appointment) => {
  try {
    const result = await appointmentStore.settleAppointment(apt.id)

    Swal.fire({
      icon: 'info',
      title: '医保结算完成',
      html: `<div class="text-start mt-3">
               <p><strong>患者姓名：</strong>${result.patient_name}</p>
               <p><strong>医保类型：</strong>${getInsuranceTypeText(result.insurance_type)}</p>
               <hr>
               <p><strong>挂号费：</strong>¥${result.registration_fee.toFixed(2)}</p>
               <p><strong>医保报销：</strong>¥${result.insurance_fee.toFixed(2)}</p>
               <p><strong>自费金额：</strong><span class="text-danger fw-bold">¥${result.self_fee.toFixed(2)}</span></p>
             </div>`,
      confirmButtonColor: '#10b981'
    })

    refreshHistory()
  } catch (e) {
    // 错误已由 request.ts 处理
  }
}

// 刷新历史记录
const refreshHistory = () => {
  if (patientStore.currentPatient) {
    appointmentStore.loadPatientHistory(patientStore.currentPatient.id)
  }
}

// 工具函数
const getPriorityText = (priority: string) => {
  const map: Record<string, string> = {
    emergency: '急诊',
    urgent: '加急',
    normal: '普通'
  }
  return map[priority] || priority
}

const getPriorityClass = (priority: string) => {
  const map: Record<string, string> = {
    emergency: 'text-danger fw-bold',
    urgent: 'text-warning fw-bold',
    normal: 'text-muted'
  }
  return map[priority] || ''
}

const getStatusText = (status: string) => {
  const map: Record<string, string> = {
    waiting: '等待中',
    in_progress: '就诊中',
    completed: '已完成',
    cancelled: '已取消'
  }
  return map[status] || status
}

const getStatusClass = (status: string) => {
  const map: Record<string, string> = {
    waiting: 'badge bg-warning text-dark',
    in_progress: 'badge bg-primary',
    completed: 'badge bg-success',
    cancelled: 'badge bg-secondary'
  }
  return map[status] || 'badge bg-secondary'
}

const getInsuranceTypeText = (type: string) => {
  const map: Record<string, string> = {
    urban_worker: '城镇职工医保 (80%)',
    urban_resident: '城镇居民医保 (60%)',
    rural_resident: '新农合医保 (60%)',
    self: '自费 (0%)'
  }
  return map[type] || type
}

onMounted(() => {
  doctorStore.loadDoctors()
})
</script>
