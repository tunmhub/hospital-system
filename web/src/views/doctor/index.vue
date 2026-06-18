<template>
  <div class="container py-4">
    <!-- 返回首页按钮 -->
    <div class="mb-3">
      <router-link to="/" class="btn btn-light">
        <i class="bi bi-arrow-left"></i> 返回首页
      </router-link>
    </div>

    <!-- 统计信息 -->
    <div class="row mb-4">
      <div class="col-md-4 mb-3">
        <div class="card border-0 shadow-sm h-100 py-3">
          <div class="card-body text-center d-flex flex-column align-items-center justify-content-center">
            <div class="stats-number" style="font-size: 2.5rem; color: #2563eb; font-weight: bold;">
              {{ doctorStore.doctors.length }}
            </div>
            <div class="text-muted fw-semibold">当班医生总数</div>
          </div>
        </div>
      </div>
      <div class="col-md-4 mb-3">
        <div class="card border-0 shadow-sm h-100 py-3">
          <div class="card-body text-center d-flex flex-column align-items-center justify-content-center">
            <div class="stats-number" style="font-size: 2.5rem; color: #10b981; font-weight: bold;">
              {{ doctorStore.totalWaiting }}
            </div>
            <div class="text-muted fw-semibold">当前等候患者</div>
          </div>
        </div>
      </div>
      <div class="col-md-4 mb-3">
        <div class="card border-0 shadow-sm h-100 py-3 bg-light">
          <div class="card-body d-flex flex-column justify-content-center px-4">
            <div class="d-flex align-items-center mb-2">
              <i class="bi bi-clock-history me-2 fs-4 text-warning"></i>
              <span class="fs-5 fw-bold text-dark">系统自动刷新</span>
            </div>
            <p class="text-muted small mb-0">
              为了保证您获取的排队和负载数据是最新的，系统将每隔 15 秒发起一次隐脉冲拉取动作。
            </p>
          </div>
        </div>
      </div>
    </div>

    <!-- 医生工作台 -->
    <div class="card border-0 shadow-sm mt-3">
      <div class="card-header bg-success text-white py-3">
        <h5 class="mb-0 fw-bold"><i class="bi bi-person-workspace"></i> 医生叫号工作台</h5>
      </div>
      <div class="card-body bg-light p-4">
        <div v-if="doctorStore.doctors.length === 0" class="empty-state py-5">
          <i class="bi bi-person-x text-secondary" style="font-size: 4rem;"></i>
          <p class="mt-3 text-secondary fs-5">暂无当班医生或网络错误</p>
        </div>

        <div class="row gx-4 gy-4">
          <div class="col-md-6 col-lg-4" v-for="doc in doctorStore.doctors" :key="doc.id">
            <div class="bg-white p-3 border rounded shadow-sm h-100 card-hover-effect">
              <!-- 医生信息 -->
              <div class="d-flex align-items-center justify-content-between mb-3">
                <div>
                  <div class="fw-bold fs-5 mb-1">
                    {{ doc.name }}
                    <span class="badge bg-secondary ms-1 fs-6 fw-normal">{{ doc.title || '医师' }}</span>
                  </div>
                  <div class="text-muted small">
                    {{ doc.department }}
                    <span class="ms-2">|</span>
                    等待:
                    <span class="text-primary fw-bold">{{ doctorStore.getWaitingCount(doc.id) }}</span>
                    <span class="ms-2">|</span>
                    负载:
                    <span
                      :class="
                        doc.current_patients >= doc.max_patients
                          ? 'text-danger fw-bold'
                          : 'text-primary'
                      "
                    >
                      {{ doc.current_patients }}/{{ doc.max_patients }}
                    </span>
                  </div>
                </div>
              </div>

              <!-- 队列预览 -->
              <div v-if="getDoctorQueue(doc.id).length > 0" class="mb-3">
                <div class="small text-muted mb-1">当前队列：</div>
                <div class="d-flex flex-wrap gap-1">
                  <span
                    v-for="item in getDoctorQueue(doc.id).slice(0, 3)"
                    :key="item.id"
                    class="badge"
                    :class="getQueueBadgeClass(item)"
                  >
                    #{{ item.queue_number }} {{ item.patient_name || '患者' }}
                  </span>
                  <span v-if="getDoctorQueue(doc.id).length > 3" class="badge bg-secondary">
                    +{{ getDoctorQueue(doc.id).length - 3 }}
                  </span>
                </div>
              </div>

              <!-- 操作按钮 -->
              <div class="d-flex gap-2">
                <button
                  class="btn btn-success flex-grow-1"
                  @click="handleCallNext(doc)"
                  :disabled="doctorStore.getWaitingCount(doc.id) === 0"
                >
                  <i class="bi bi-megaphone"></i> 叫号
                </button>
                <button
                  v-if="hasInProgress(doc.id)"
                  class="btn btn-warning flex-grow-1"
                  @click="handleComplete(doc)"
                >
                  <i class="bi bi-check-circle"></i> 完成就诊
                </button>
              </div>
            </div>
          </div>
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
.card-hover-effect {
  transition: all 0.3s ease;
}
.card-hover-effect:hover {
  transform: translateY(-3px);
  box-shadow: 0 10px 25px rgba(0, 0, 0, 0.1) !important;
}
</style>
