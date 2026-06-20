<template>
  <div class="doctor-workstation min-h-screen bg-gray-50">
    <!-- 顶部导航栏 -->
    <div class="sticky top-0 z-50 bg-white/80 backdrop-blur-xl border-b border-gray-100 shadow-sm">
      <div class="max-w-7xl mx-auto px-6 py-3 flex items-center justify-between">
        <div class="flex items-center gap-3">
          <router-link
            to="/"
            class="inline-flex items-center gap-2 px-3 py-1.5 rounded-lg text-gray-500 hover:text-gray-800 hover:bg-gray-100 transition-all duration-200 text-sm"
          >
            <el-icon><ArrowLeft /></el-icon>
            <span>返回</span>
          </router-link>
          <div class="w-px h-5 bg-gray-200"></div>
          <h1 class="text-base font-semibold text-gray-800 flex items-center gap-2">
            <span class="inline-flex items-center justify-center w-7 h-7 rounded-lg bg-emerald-500 text-white text-xs">
              <i class="bi bi-person-workspace"></i>
            </span>
            医生叫号工作台
          </h1>
        </div>
        <div class="flex items-center gap-2">
          <span class="inline-flex items-center gap-1.5 px-2.5 py-1 rounded-full bg-emerald-50 text-emerald-600 text-xs font-medium">
            <span class="w-1.5 h-1.5 rounded-full bg-emerald-500 animate-pulse"></span>
            实时同步中
          </span>
        </div>
      </div>
    </div>

    <div class="max-w-7xl mx-auto px-6 py-6 space-y-6">
      <!-- 统计卡片 -->
      <div class="grid grid-cols-1 md:grid-cols-3 gap-4">
        <!-- 当班医生 -->
        <el-card shadow="never" class="stat-card !border-0 !rounded-2xl overflow-hidden">
          <div class="flex items-center gap-4">
            <div class="flex-shrink-0 w-12 h-12 rounded-2xl bg-blue-50 flex items-center justify-center">
              <i class="bi bi-person-badge text-xl text-blue-500"></i>
            </div>
            <div class="flex-1 min-w-0">
              <div class="text-xs text-gray-400 font-medium uppercase tracking-wider mb-0.5">当班医生</div>
              <div class="text-3xl font-bold text-gray-900 tracking-tight leading-none">
                {{ doctorStore.doctors.length }}
              </div>
            </div>
          </div>
        </el-card>

        <!-- 等候患者 -->
        <el-card shadow="never" class="stat-card !border-0 !rounded-2xl overflow-hidden">
          <div class="flex items-center gap-4">
            <div class="flex-shrink-0 w-12 h-12 rounded-2xl bg-amber-50 flex items-center justify-center">
              <i class="bi bi-people text-xl text-amber-500"></i>
            </div>
            <div class="flex-1 min-w-0">
              <div class="text-xs text-gray-400 font-medium uppercase tracking-wider mb-0.5">等候患者</div>
              <div class="text-3xl font-bold text-gray-900 tracking-tight leading-none">
                {{ doctorStore.totalWaiting }}
              </div>
            </div>
          </div>
        </el-card>

        <!-- 系统状态 -->
        <el-card shadow="never" class="stat-card !border-0 !rounded-2xl overflow-hidden">
          <div class="flex items-center gap-4">
            <div class="flex-shrink-0 w-12 h-12 rounded-2xl bg-violet-50 flex items-center justify-center">
              <i class="bi bi-arrow-repeat text-xl text-violet-500"></i>
            </div>
            <div class="flex-1 min-w-0">
              <div class="text-xs text-gray-400 font-medium uppercase tracking-wider mb-0.5">自动刷新</div>
              <div class="text-sm font-medium text-gray-600 leading-snug">
                每 15 秒同步一次数据
              </div>
            </div>
          </div>
        </el-card>
      </div>

      <!-- 医生工作区 -->
      <div>
        <div class="flex items-center justify-between mb-4">
          <h2 class="text-sm font-semibold text-gray-500 uppercase tracking-wider">诊室列表</h2>
          <span class="text-xs text-gray-400">共 {{ doctorStore.doctors.length }} 位医生当班</span>
        </div>

        <!-- 空状态 -->
        <el-empty
          v-if="doctorStore.doctors.length === 0"
          description="暂无当班医生或网络错误"
          class="py-16"
        />

        <!-- 医生卡片网格 -->
        <div class="grid grid-cols-1 md:grid-cols-2 xl:grid-cols-3 gap-4">
          <el-card
            v-for="doc in doctorStore.doctors"
            :key="doc.id"
            shadow="hover"
            class="doctor-card !border-0 !rounded-2xl overflow-hidden group transition-all duration-300"
          >
            <!-- 卡片头部：医生信息 -->
            <div class="flex items-start justify-between mb-4">
              <div class="flex items-center gap-3">
                <!-- 头像 -->
                <div class="w-10 h-10 rounded-xl bg-gradient-to-br from-blue-500 to-indigo-600 flex items-center justify-center text-white font-bold text-sm shadow-lg shadow-blue-500/20">
                  {{ doc.name.charAt(0) }}
                </div>
                <div>
                  <div class="flex items-center gap-2">
                    <span class="text-base font-bold text-gray-900">{{ doc.name }}</span>
                    <el-tag size="small" type="info" effect="plain" class="!rounded-md !text-xs">
                      {{ doc.title || '医师' }}
                    </el-tag>
                  </div>
                  <div class="text-xs text-gray-400 mt-0.5">{{ doc.department }}</div>
                </div>
              </div>
              <!-- 状态指示灯 -->
              <div
                class="flex-shrink-0 w-2.5 h-2.5 rounded-full mt-1.5"
                :class="doc.current_patients >= doc.max_patients ? 'bg-red-400 shadow-red-400/40 shadow-md' : 'bg-emerald-400 shadow-emerald-400/40 shadow-md'"
              ></div>
            </div>

            <!-- 负载进度条 -->
            <div class="mb-4">
              <div class="flex items-center justify-between mb-1.5">
                <span class="text-xs text-gray-400 font-medium">当前负载</span>
                <span class="text-xs font-bold" :class="doc.current_patients >= doc.max_patients ? 'text-red-500' : 'text-gray-700'">
                  {{ doc.current_patients }}<span class="text-gray-300 font-normal"> / </span>{{ doc.max_patients }}
                </span>
              </div>
              <el-progress
                :percentage="Math.round((doc.current_patients / doc.max_patients) * 100)"
                :stroke-width="6"
                :show-text="false"
                :color="doc.current_patients >= doc.max_patients ? '#ef4444' : doc.current_patients > doc.max_patients * 0.7 ? '#f59e0b' : '#10b981'"
                class="!rounded-full overflow-hidden"
              />
            </div>

            <!-- 队列预览 -->
            <div v-if="getDoctorQueue(doc.id).length > 0" class="mb-4">
              <div class="flex items-center justify-between mb-2">
                <span class="text-xs text-gray-400 font-medium">当前队列</span>
                <el-tag
                  size="small"
                  :type="doctorStore.getWaitingCount(doc.id) > 5 ? 'danger' : doctorStore.getWaitingCount(doc.id) > 2 ? 'warning' : 'success'"
                  effect="dark"
                  class="!rounded-full !text-xs !px-2"
                >
                  {{ doctorStore.getWaitingCount(doc.id) }} 人等候
                </el-tag>
              </div>
              <div class="flex flex-wrap gap-1.5">
                <span
                  v-for="item in getDoctorQueue(doc.id).slice(0, 4)"
                  :key="item.id"
                  class="inline-flex items-center gap-1 px-2 py-0.5 rounded-md text-xs font-medium"
                  :class="[getQueueBadgeClass(item), {
                    'bg-blue-50 text-blue-600': item.status === 'in_progress',
                    'bg-red-50 text-red-600': item.priority === 'emergency',
                    'bg-amber-50 text-amber-600': item.priority === 'urgent',
                    'bg-gray-50 text-gray-500': item.status !== 'in_progress' && item.priority === 'normal'
                  }]"
                >
                  <span v-if="item.status === 'in_progress'" class="w-1.5 h-1.5 rounded-full bg-blue-500 animate-pulse"></span>
                  #{{ item.queue_number }}
                </span>
                <span
                  v-if="getDoctorQueue(doc.id).length > 4"
                  class="inline-flex items-center px-2 py-0.5 rounded-md bg-gray-50 text-gray-400 text-xs"
                >
                  +{{ getDoctorQueue(doc.id).length - 4 }}
                </span>
              </div>
            </div>

            <div v-else class="mb-4 py-3 text-center">
              <span class="text-xs text-gray-300">暂无排队患者</span>
            </div>

            <!-- 操作按钮 -->
            <div class="flex gap-2 pt-3 border-t border-gray-50">
              <el-button
                type="primary"
                class="flex-1 !rounded-xl !h-10 !font-semibold !text-sm"
                :disabled="doctorStore.getWaitingCount(doc.id) === 0"
                @click="handleCallNext(doc)"
              >
                <el-icon class="mr-1"><Microphone /></el-icon>
                叫号
              </el-button>
              <el-button
                v-if="hasInProgress(doc.id)"
                type="warning"
                class="flex-1 !rounded-xl !h-10 !font-semibold !text-sm"
                @click="handleComplete(doc)"
              >
                <el-icon class="mr-1"><CircleCheck /></el-icon>
                完成就诊
              </el-button>
              <el-button
                v-else
                disabled
                class="flex-1 !rounded-xl !h-10 !text-sm"
              >
                <el-icon class="mr-1"><Clock /></el-icon>
                等待就诊
              </el-button>
            </div>
          </el-card>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { onMounted, onUnmounted } from 'vue'
import Swal from 'sweetalert2'
import { useDoctorStore } from '@/stores/doctor'
import { useAppointmentStore } from '@/stores/appointment'
import type { Doctor, Appointment } from '@/types'

const doctorStore = useDoctorStore()
const appointmentStore = useAppointmentStore()

let timer: ReturnType<typeof setInterval> | null = null

// 获取医生队列
const getDoctorQueue = (doctorId: number): Appointment[] => {
  return doctorStore.doctorQueues[doctorId] || []
}

// 检查是否有就诊中的患者
const hasInProgress = (doctorId: number): boolean => {
  const queue = doctorStore.doctorQueues[doctorId] || []
  return queue.some((item) => item.status === 'in_progress')
}

// 获取队列徽章样式
const getQueueBadgeClass = (item: Appointment) => {
  if (item.status === 'in_progress') return 'bg-primary'
  if (item.priority === 'emergency') return 'bg-danger'
  if (item.priority === 'urgent') return 'bg-warning text-dark'
  return 'bg-secondary'
}

// 叫号
const handleCallNext = async (doc: Doctor) => {
  try {
    const result = await doctorStore.callNext(doc.id)

    Swal.fire({
      title: '<strong>大厅呼叫成功</strong>',
      html: `<div style="font-size: 1.5rem; margin: 1rem 0;">
               请下一位患者进入诊室就诊
               <div style="color: #10b981; font-weight: bold; font-size: 2.2rem; margin-top: 1rem;">
                 ${(result as any).patient_name || '患者'}
               </div>
             </div>`,
      icon: 'info',
      timer: 8000,
      timerProgressBar: true,
      confirmButtonText: '收到',
      confirmButtonColor: '#10b981'
    })
  } catch (e) {
    // 错误已由 request.ts 处理
  }
}

// 完成就诊
const handleComplete = async (doc: Doctor) => {
  const queue = doctorStore.doctorQueues[doc.id] || []
  const inProgress = queue.find((item) => item.status === 'in_progress')

  if (!inProgress) {
    Swal.fire({
      icon: 'warning',
      title: '没有就诊中的患者',
      confirmButtonColor: '#2563eb'
    })
    return
  }

  try {
    await appointmentStore.completeAppointment(inProgress.id)

    Swal.fire({
      icon: 'success',
      title: '就诊已完成',
      text: `患者 ${inProgress.patient_name || '患者'} 的就诊已结束`,
      timer: 2000,
      showConfirmButton: false
    })

    // 刷新队列
    await doctorStore.loadDoctorQueue(doc.id)
    await doctorStore.loadDoctors()
  } catch (e) {
    // 错误已由 request.ts 处理
  }
}

// 加载数据
const loadData = async () => {
  await doctorStore.loadDoctors()
  await doctorStore.loadAllQueues()
}

onMounted(() => {
  loadData()
  timer = window.setInterval(loadData, 15000)
})

onUnmounted(() => {
  if (timer) clearInterval(timer)
})
</script>

<style scoped>
/* 统计卡片悬停效果 */
.stat-card :deep(.el-card__body) {
  padding: 1.25rem 1.5rem;
}
.stat-card {
  transition: all 0.3s ease;
}
.stat-card:hover {
  transform: translateY(-2px);
  box-shadow: 0 8px 25px rgba(0, 0, 0, 0.08);
}

/* 医生卡片 */
.doctor-card :deep(.el-card__body) {
  padding: 1.25rem;
}
.doctor-card {
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}
.doctor-card:hover {
  transform: translateY(-3px);
  box-shadow: 0 12px 30px rgba(0, 0, 0, 0.1);
}

/* 按钮点击涟漪效果 */
.doctor-card .el-button:active {
  transform: scale(0.97);
}

/* 进度条圆角 */
.doctor-card :deep(.el-progress-bar__outer) {
  border-radius: 999px;
}
.doctor-card :deep(.el-progress-bar__inner) {
  border-radius: 999px;
}

/* 叫号按钮脉冲动画 */
.doctor-card .el-button--primary:not(:disabled) {
  background: linear-gradient(135deg, #10b981 0%, #059669 100%);
  border: none;
  position: relative;
  overflow: hidden;
}
.doctor-card .el-button--primary:not(:disabled)::after {
  content: '';
  position: absolute;
  inset: 0;
  background: linear-gradient(135deg, rgba(255,255,255,0.15) 0%, transparent 50%);
  pointer-events: none;
}

/* 完成就诊按钮 */
.doctor-card .el-button--warning {
  background: linear-gradient(135deg, #f59e0b 0%, #d97706 100%);
  border: none;
}

/* 禁用按钮样式 */
.doctor-card .el-button.is-disabled {
  background: #f3f4f6 !important;
  border-color: #e5e7eb !important;
  color: #9ca3af !important;
}
</style>
