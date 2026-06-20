<template>
  <div class="patient-page min-h-screen bg-gray-50">
    <!-- 顶部导航 -->
    <div class="sticky top-0 z-50 bg-white/80 backdrop-blur-xl border-b border-gray-100 shadow-sm">
      <div class="max-w-lg mx-auto px-4 py-3 flex items-center gap-3">
        <router-link
          to="/"
          class="inline-flex items-center justify-center w-8 h-8 rounded-full hover:bg-gray-100 transition-colors text-gray-500"
        >
          <el-icon :size="18"><ArrowLeft /></el-icon>
        </router-link>
        <h1 class="text-base font-semibold text-gray-800 flex-1">挂号服务</h1>
        <span class="inline-flex items-center gap-1 text-xs text-emerald-600 font-medium">
          <span class="w-1.5 h-1.5 rounded-full bg-emerald-500 animate-pulse"></span>
          在线
        </span>
      </div>
    </div>

    <div class="max-w-lg mx-auto px-4 py-5 space-y-4 pb-24">

      <!-- ========== 患者查询/注册 ========== -->
      <el-card shadow="never" class="!border-0 !rounded-2xl">
        <template #header>
          <div class="flex items-center gap-2 text-gray-800 font-semibold text-sm">
            <span class="w-6 h-6 rounded-lg bg-blue-50 flex items-center justify-center">
              <i class="bi bi-person text-blue-500 text-xs"></i>
            </span>
            患者信息
          </div>
        </template>

        <!-- 姓名搜索 -->
        <div class="flex gap-2 mb-3">
          <el-input
            v-model="patientName"
            placeholder="输入患者姓名"
            size="large"
            class="flex-1"
            clearable
            @input="onSearchInput"
            @keyup.enter="findOrCreatePatient"
          >
            <template #prefix>
              <el-icon><Search /></el-icon>
            </template>
          </el-input>
          <el-button
            type="primary"
            size="large"
            class="!rounded-xl !px-5"
            @click="findOrCreatePatient"
          >
            查询
          </el-button>
        </div>

        <!-- 搜索结果 -->
        <div v-if="patientStore.searchResults.length > 0" class="space-y-1.5 mb-3">
          <div
            v-for="p in patientStore.searchResults"
            :key="p.id"
            class="flex items-center gap-3 px-3 py-2.5 rounded-xl bg-blue-50/60 hover:bg-blue-50 cursor-pointer transition-colors"
            @click="selectExistingPatient(p)"
          >
            <div class="w-8 h-8 rounded-full bg-blue-100 flex items-center justify-center flex-shrink-0">
              <i class="bi bi-person-check text-blue-500 text-sm"></i>
            </div>
            <div class="flex-1 min-w-0">
              <div class="text-sm font-semibold text-gray-800 truncate">{{ p.name }}</div>
              <div class="text-xs text-gray-400">{{ p.phone }}</div>
            </div>
            <span class="text-xs text-gray-300">#{{ p.id }}</span>
          </div>
        </div>

        <!-- 患者信息表单（新注册或编辑） -->
        <div class="space-y-3">
          <div class="grid grid-cols-2 gap-3">
            <div>
              <label class="text-xs text-gray-400 font-medium mb-1 block">手机号 <span class="text-red-400">*</span></label>
              <el-input v-model="phone" placeholder="11位手机号" size="large" class="!rounded-xl" />
            </div>
            <div>
              <label class="text-xs text-gray-400 font-medium mb-1 block">身份证号 <span class="text-red-400">*</span></label>
              <el-input v-model="idCard" placeholder="18位身份证号" size="large" class="!rounded-xl" />
            </div>
          </div>
          <div class="grid grid-cols-3 gap-3">
            <div>
              <label class="text-xs text-gray-400 font-medium mb-1 block">年龄 <span class="text-red-400">*</span></label>
              <el-input-number v-model="age" :min="1" :max="150" size="large" class="!w-full !rounded-xl" controls-position="right" />
            </div>
            <div>
              <label class="text-xs text-gray-400 font-medium mb-1 block">性别 <span class="text-red-400">*</span></label>
              <el-select v-model="gender" placeholder="选择" size="large" class="!w-full">
                <el-option label="男" value="男" />
                <el-option label="女" value="女" />
              </el-select>
            </div>
            <div>
              <label class="text-xs text-gray-400 font-medium mb-1 block">医保 <span class="text-red-400">*</span></label>
              <el-select v-model="insuranceType" placeholder="选择" size="large" class="!w-full">
                <el-option label="职工 (80%)" value="urban_worker" />
                <el-option label="居民 (60%)" value="urban_resident" />
                <el-option label="新农合 (60%)" value="rural_resident" />
                <el-option label="自费" value="self" />
              </el-select>
            </div>
          </div>
        </div>
      </el-card>

      <!-- ========== 当前患者指示条 ========== -->
      <div
        v-if="patientStore.currentPatient"
        class="flex items-center gap-3 px-4 py-3 rounded-2xl bg-gradient-to-r from-blue-500 to-indigo-500 text-white shadow-lg shadow-blue-500/20"
      >
        <div class="w-10 h-10 rounded-full bg-white/20 flex items-center justify-center text-lg font-bold">
          {{ patientStore.currentPatient.name.charAt(0) }}
        </div>
        <div class="flex-1 min-w-0">
          <div class="font-semibold text-sm">{{ patientStore.currentPatient.name }}</div>
          <div class="text-xs text-blue-100">
            {{ patientStore.currentPatient.gender }} · {{ getInsuranceTypeText(patientStore.currentPatient.insurance_type || 'self') }}
          </div>
        </div>
        <span class="text-xs bg-white/20 px-2 py-0.5 rounded-full">ID: {{ patientStore.currentPatient.id }}</span>
      </div>

      <!-- ========== 挂号表单 ========== -->
      <el-card shadow="never" class="!border-0 !rounded-2xl">
        <template #header>
          <div class="flex items-center gap-2 text-gray-800 font-semibold text-sm">
            <span class="w-6 h-6 rounded-lg bg-emerald-50 flex items-center justify-center">
              <i class="bi bi-calendar-plus text-emerald-500 text-xs"></i>
            </span>
            预约挂号
          </div>
        </template>

        <form @submit.prevent="submitAppointment" class="space-y-4">
          <!-- 科室 -->
          <div>
            <label class="text-xs text-gray-400 font-medium mb-1.5 block">就诊科室</label>
            <el-select
              v-model="form.department"
              placeholder="请选择就诊科室"
              size="large"
              class="!w-full"
            >
              <el-option
                v-for="dept in departments"
                :key="dept"
                :label="dept"
                :value="dept"
              />
            </el-select>
          </div>

          <!-- 医生 -->
          <div>
            <label class="text-xs text-gray-400 font-medium mb-1.5 block">
              指定医生
              <span class="text-gray-300 font-normal">（不选则自动分配）</span>
            </label>
            <el-select
              v-model="form.doctorId"
              placeholder="自动分配 · 负载最小优先"
              size="large"
              class="!w-full"
              clearable
            >
              <el-option
                v-for="doc in filteredDoctors"
                :key="doc.id"
                :label="`${doc.name}（${doc.title || '医师'}）`"
                :value="doc.id"
              />
            </el-select>
          </div>

          <!-- 优先级 -->
          <div>
            <label class="text-xs text-gray-400 font-medium mb-2 block">就诊类型</label>
            <div class="grid grid-cols-3 gap-2">
              <button
                type="button"
                class="flex flex-col items-center gap-1 py-3 rounded-xl border-2 transition-all duration-200"
                :class="!form.isUrgent && !form.isEmergency
                  ? 'border-blue-500 bg-blue-50 text-blue-600'
                  : 'border-gray-100 bg-white text-gray-400 hover:border-gray-200'"
                @click="form.isUrgent = false; form.isEmergency = false"
              >
                <i class="bi bi-clipboard-check text-lg"></i>
                <span class="text-xs font-semibold">普通</span>
              </button>
              <button
                type="button"
                class="flex flex-col items-center gap-1 py-3 rounded-xl border-2 transition-all duration-200"
                :class="form.isUrgent && !form.isEmergency
                  ? 'border-amber-500 bg-amber-50 text-amber-600'
                  : 'border-gray-100 bg-white text-gray-400 hover:border-gray-200'"
                @click="form.isUrgent = true; form.isEmergency = false"
              >
                <i class="bi bi-exclamation-triangle text-lg"></i>
                <span class="text-xs font-semibold">加急</span>
              </button>
              <button
                type="button"
                class="flex flex-col items-center gap-1 py-3 rounded-xl border-2 transition-all duration-200"
                :class="form.isEmergency
                  ? 'border-red-500 bg-red-50 text-red-600'
                  : 'border-gray-100 bg-white text-gray-400 hover:border-gray-200'"
                @click="form.isUrgent = false; form.isEmergency = true"
              >
                <i class="bi bi-lightning-charge text-lg"></i>
                <span class="text-xs font-semibold">急诊</span>
              </button>
            </div>
          </div>

          <!-- 提交按钮 -->
          <el-button
            type="primary"
            native-type="submit"
            size="large"
            class="w-full !rounded-xl !h-12 !text-base !font-semibold"
            :disabled="!patientStore.currentPatient"
          >
            <el-icon class="mr-1"><CircleCheck /></el-icon>
            确认挂号
          </el-button>
        </form>
      </el-card>

      <!-- ========== 挂号成功卡片 ========== -->
      <el-card
        v-if="appointmentStore.currentAppointment"
        shadow="never"
        class="!border-0 !rounded-2xl overflow-hidden"
      >
        <!-- 顶部装饰条 -->
        <div class="h-1 bg-gradient-to-r from-emerald-400 to-teal-400 -mt-5 -mx-5 mb-4"></div>

        <div class="flex items-center gap-2 mb-4">
          <span class="w-6 h-6 rounded-full bg-emerald-100 flex items-center justify-center">
            <i class="bi bi-check-lg text-emerald-500 text-sm"></i>
          </span>
          <span class="text-sm font-semibold text-gray-800">挂号成功</span>
        </div>

        <!-- 排队号码突出显示 -->
        <div class="text-center py-4 mb-4 rounded-2xl bg-gradient-to-br from-blue-50 to-indigo-50">
          <div class="text-xs text-gray-400 mb-1">您的排队号码</div>
          <div class="text-5xl font-black text-blue-600 tracking-tight leading-none">
            #{{ appointmentStore.currentAppointment.queue_number }}
          </div>
        </div>

        <!-- 详情网格 -->
        <div class="grid grid-cols-2 gap-x-4 gap-y-3 mb-5 text-sm">
          <div>
            <div class="text-xs text-gray-400 mb-0.5">挂号单号</div>
            <div class="font-semibold text-gray-700">{{ appointmentStore.currentAppointment.id }}</div>
          </div>
          <div>
            <div class="text-xs text-gray-400 mb-0.5">优先级</div>
            <div class="font-semibold" :title="getPriorityClass(appointmentStore.currentAppointment.priority)" :class="{
              'text-red-500': appointmentStore.currentAppointment.priority === 'emergency',
              'text-amber-500': appointmentStore.currentAppointment.priority === 'urgent',
              'text-gray-500': appointmentStore.currentAppointment.priority === 'normal'
            }">
              {{ getPriorityText(appointmentStore.currentAppointment.priority) }}
            </div>
          </div>
          <div>
            <div class="text-xs text-gray-400 mb-0.5">分配医生</div>
            <div class="font-semibold text-gray-700">{{ appointmentStore.currentAppointment.doctor_name || '自动分配中' }}</div>
          </div>
          <div>
            <div class="text-xs text-gray-400 mb-0.5">就诊科室</div>
            <div class="font-semibold text-gray-700">{{ appointmentStore.currentAppointment.department || '-' }}</div>
          </div>
        </div>

        <!-- 操作按钮 -->
        <div class="grid grid-cols-2 gap-3">
          <el-button
            size="large"
            class="!rounded-xl !h-11 !font-semibold"
            @click="handleCancel"
          >
            <el-icon class="mr-1"><Close /></el-icon>
            退号
          </el-button>
          <el-button
            type="warning"
            size="large"
            class="!rounded-xl !h-11 !font-semibold"
            @click="handleSettle"
          >
            <el-icon class="mr-1"><Wallet /></el-icon>
            医保结算
          </el-button>
        </div>
      </el-card>

      <!-- ========== 历史就诊记录 ========== -->
      <el-card
        v-if="patientStore.currentPatient"
        shadow="never"
        class="!border-0 !rounded-2xl"
      >
        <template #header>
          <div class="flex items-center justify-between">
            <div class="flex items-center gap-2 text-gray-800 font-semibold text-sm">
              <span class="w-6 h-6 rounded-lg bg-violet-50 flex items-center justify-center">
                <i class="bi bi-clock-history text-violet-500 text-xs"></i>
              </span>
              就诊记录
            </div>
            <el-button text size="small" @click="refreshHistory" class="!text-gray-400">
              <el-icon class="mr-0.5"><Refresh /></el-icon> 刷新
            </el-button>
          </div>
        </template>

        <!-- 空状态 -->
        <div v-if="appointmentStore.patientAppointments.length === 0" class="py-8 text-center">
          <div class="w-12 h-12 rounded-full bg-gray-50 flex items-center justify-center mx-auto mb-3">
            <i class="bi bi-inbox text-gray-300 text-xl"></i>
          </div>
          <div class="text-sm text-gray-300">暂无就诊记录</div>
        </div>

        <!-- 记录列表（移动端友好卡片） -->
        <div v-else class="space-y-2.5">
          <div
            v-for="apt in appointmentStore.patientAppointments"
            :key="apt.id"
            class="p-3 rounded-xl bg-gray-50/80 hover:bg-gray-50 transition-colors"
          >
            <div class="flex items-start justify-between mb-2">
              <div class="flex items-center gap-2">
                <span class="text-xl font-black text-blue-600 leading-none">#{{ apt.queue_number }}</span>
                <span
                  class="px-2 py-0.5 rounded-full text-xs font-medium"
                  :title="getStatusClass(apt.status)"
                  :class="{
                    'bg-amber-50 text-amber-600': apt.status === 'waiting',
                    'bg-blue-50 text-blue-600': apt.status === 'in_progress',
                    'bg-emerald-50 text-emerald-600': apt.status === 'completed',
                    'bg-gray-100 text-gray-400': apt.status === 'cancelled'
                  }"
                >
                  {{ getStatusText(apt.status) }}
                </span>
                <span
                  v-if="apt.settled"
                  class="px-2 py-0.5 rounded-full text-xs font-medium bg-emerald-50 text-emerald-600"
                >
                  已结算
                </span>
              </div>
              <span class="text-xs text-gray-300">{{ apt.created_at || '' }}</span>
            </div>

            <div class="flex items-center gap-3 text-xs text-gray-500 mb-2">
              <span v-if="apt.doctor_name">
                <i class="bi bi-person mr-0.5"></i>{{ apt.doctor_name }}
              </span>
              <span v-if="apt.department">
                <i class="bi bi-building mr-0.5"></i>{{ apt.department }}
              </span>
              <span :class="{
                'text-red-400': apt.priority === 'emergency',
                'text-amber-400': apt.priority === 'urgent',
                'text-gray-400': apt.priority === 'normal'
              }">
                {{ getPriorityText(apt.priority) }}
              </span>
            </div>

            <!-- 操作按钮 -->
            <div class="flex gap-2">
              <el-button
                v-if="apt.status === 'waiting'"
                size="small"
                class="!rounded-lg !text-xs"
                @click="handleCancelHistory(apt)"
              >
                退号
              </el-button>
              <el-button
                v-if="apt.status === 'completed' && !apt.settled"
                type="warning"
                size="small"
                class="!rounded-lg !text-xs"
                @click="handleSettleHistory(apt)"
              >
                结算
              </el-button>
            </div>
          </div>
        </div>
      </el-card>

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

  // 格式校验
  const phoneStr = phone.value.trim()
  if (!/^1\d{10}$/.test(phoneStr)) {
    Swal.fire({
      icon: 'warning',
      title: '手机号格式不正确',
      text: `手机号必须为 11 位数字且以 1 开头，当前输入: "${phoneStr}"（${phoneStr.length} 位）`
    })
    return
  }

  const idStr = idCard.value.trim()
  if (!/^\d{17}[\dXx]$/.test(idStr)) {
    Swal.fire({
      icon: 'warning',
      title: '身份证号格式不正确',
      text: `身份证号必须为 18 位（前 17 位数字 + 末位数字或 X），当前输入: "${idStr}"（${idStr.length} 位）`
    })
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

<style scoped>
/* Element Plus 圆角覆盖 */
.patient-page :deep(.el-input__wrapper),
.patient-page :deep(.el-select .el-input__wrapper),
.patient-page :deep(.el-input-number .el-input__wrapper) {
  border-radius: 0.75rem;
  box-shadow: 0 0 0 1px #e5e7eb inset;
}
.patient-page :deep(.el-input__wrapper:focus-within),
.patient-page :deep(.el-select .el-input.is-focus .el-input__wrapper) {
  box-shadow: 0 0 0 2px #3b82f6 inset;
}

/* 卡片内部间距 */
.patient-page :deep(.el-card__header) {
  padding: 1rem 1.25rem 0.75rem;
  border-bottom: none;
}
.patient-page :deep(.el-card__body) {
  padding: 0.75rem 1.25rem 1.25rem;
}

/* 按钮点击缩放 */
.patient-page .el-button:active {
  transform: scale(0.97);
}

/* 提交按钮渐变 */
.patient-page .el-button--primary:not(:disabled) {
  background: linear-gradient(135deg, #3b82f6 0%, #6366f1 100%);
  border: none;
}

/* 历史记录列表动画 */
.patient-page .space-y-2\.5 > div {
  animation: fadeSlideUp 0.3s ease forwards;
}
@keyframes fadeSlideUp {
  from { opacity: 0; transform: translateY(8px); }
  to   { opacity: 1; transform: translateY(0); }
}
</style>
