<template>
  <div class="screen-container bg-dark text-white min-vh-100 d-flex flex-column pb-5">
    <div class="container h-100 mt-4 flex-grow-1">
      <!-- 返回首页按钮 -->
      <div class="mb-3">
        <router-link to="/" class="btn btn-outline-light">
          <i class="bi bi-arrow-left"></i> 返回首页
        </router-link>
      </div>

      <!-- 头部：标题与选择 -->
      <div class="d-flex justify-content-between align-items-center mb-5 pb-3 border-bottom border-secondary">
        <h1 class="fw-bold m-0" style="letter-spacing: 2px;">
          <i class="bi bi-display text-primary me-2"></i> 门诊排队大厅
        </h1>
        <select
          class="form-select w-auto bg-dark text-white fs-5 border-secondary py-2"
          v-model="selectedDoctor"
          @change="refreshQueue"
        >
          <option value="">-- 请选择要投屏的医生 --</option>
          <option v-for="doc in doctorStore.doctors" :key="doc.id" :value="doc.id">
            {{ doc.name }} ({{ doc.department }})
          </option>
        </select>
      </div>

      <!-- 未选择状态 -->
      <div v-if="!selectedDoctor" class="text-center py-5 mt-5 d-flex flex-column align-items-center opacity-50">
        <i class="bi bi-cast" style="font-size: 8rem;"></i>
        <h3 class="mt-4 fw-light">未接入数据源，请在右上角选择接入排队信号</h3>
      </div>

      <!-- 选择后的挂号大屏列表 -->
      <div v-else class="queue-board">
        <div v-if="queue.length === 0" class="text-center text-success py-5 mt-5 pulse-anim">
          <i class="bi bi-check-circle" style="font-size: 6rem;"></i>
          <h2 class="mt-4 fw-bold">当前诊室无人排队</h2>
        </div>

        <div v-else class="row gx-4 gy-4">
          <div class="col-md-6 col-lg-4" v-for="item in queue" :key="item.id">
            <!-- 卡片按优先级定色 -->
            <div
              class="card h-100 border-0 shadow-lg queue-card"
              :class="getCardClass(item.priority)"
            >
              <div class="card-body p-4 position-relative overflow-hidden">
                <!-- 遮罩层制造反光效果 -->
                <div class="card-glare"></div>

                <div class="d-flex justify-content-between align-items-start mb-3">
                  <span class="fs-1 fw-bold display-4 queue-number">#{{ item.queue_number }}</span>
                  <span class="badge px-3 py-2 fs-6 rounded-pill" :class="getBadgeClass(item.priority)">
                    {{ getPriorityText(item.priority) }}
                  </span>
                </div>

                <h2 class="fw-bold mb-3 name-text">
                  {{ item.patient_name || `患者${item.patient_id}` }}
                </h2>

                <div class="d-flex align-items-center mt-auto">
                  <i class="bi bi-hourglass-split me-2 opacity-75"></i>
                  <span class="opacity-75 fs-5">
                    预计等待:
                    <strong>{{ item.wait_minutes !== null ? item.wait_minutes + ' 分钟' : '计算中...' }}</strong>
                  </span>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'
import { useDoctorStore } from '@/stores/doctor'
import { appointmentApi } from '@/api/appointment'
import type { Appointment, WaitTimeResponse } from '@/types'

const doctorStore = useDoctorStore()

// 扩展 Appointment 添加等待时间
interface QueueItem extends Appointment {
  wait_minutes?: number | null
}

const selectedDoctor = ref('')
const queue = ref<QueueItem[]>([])
let timer: ReturnType<typeof setInterval> | null = null

const refreshQueue = async () => {
  if (!selectedDoctor.value) {
    queue.value = []
    return
  }

  try {
    const allQueue = await appointmentApi.getByDoctor(Number(selectedDoctor.value))

    // 只过滤出目前状态在等待的
    const waiting: QueueItem[] = allQueue.filter((item) => item.status === 'waiting')

    // 强制排序：急诊(0) > 加急(1) > 普通(2)
    const priorityOrder: Record<string, number> = { emergency: 0, urgent: 1, normal: 2 }
    waiting.sort((a, b) => {
      const pA = priorityOrder[String(a.priority).toLowerCase()] ?? 2
      const pB = priorityOrder[String(b.priority).toLowerCase()] ?? 2
      if (pA !== pB) return pA - pB
      return a.queue_number - b.queue_number
    })

    // 调用后端 wait_time 接口获取等待时间
    for (const item of waiting) {
      try {
        const waitData: WaitTimeResponse = await appointmentApi.getWaitTime(item.id)
        item.wait_minutes = waitData.wait_minutes
      } catch (e) {
        item.wait_minutes = null
      }
    }

    queue.value = waiting
  } catch (e) {
    // 错误已由 request.ts 处理
  }
}

const getPriorityText = (priority: string) => {
  const p = String(priority).toLowerCase()
  if (p === 'emergency') return '急诊'
  if (p === 'urgent') return '加急'
  return '普通'
}

const getCardClass = (priority: string) => {
  const p = String(priority).toLowerCase()
  if (p === 'emergency') return 'bg-danger text-white border-danger emergency-card'
  if (p === 'urgent') return 'bg-warning text-dark border-warning urgent-card'
  return 'bg-secondary text-white'
}

const getBadgeClass = (priority: string) => {
  const p = String(priority).toLowerCase()
  if (p === 'emergency') return 'bg-white text-danger'
  if (p === 'urgent') return 'bg-dark text-warning'
  return 'bg-dark text-white'
}

onMounted(() => {
  document.body.style.background = '#212529'
  doctorStore.loadDoctors()
  timer = window.setInterval(() => {
    if (selectedDoctor.value) refreshQueue()
  }, 10000)
})

onUnmounted(() => {
  document.body.style.background = ''
  if (timer) clearInterval(timer)
})
</script>

<style scoped>
.screen-container {
  margin-top: 0px;
  padding-top: 0px;
}

.pulse-anim {
  animation: pulse 2s infinite;
}
@keyframes pulse {
  0% {
    opacity: 1;
    transform: scale(1);
  }
  50% {
    opacity: 0.8;
    transform: scale(0.98);
  }
  100% {
    opacity: 1;
    transform: scale(1);
  }
}

.queue-card {
  border-radius: 16px;
  transform: translateY(0);
  transition: transform 0.4s cubic-bezier(0.175, 0.885, 0.32, 1.275);
}
.queue-card:hover {
  transform: translateY(-10px);
}

.card-glare {
  position: absolute;
  top: -50%;
  left: -50%;
  width: 200%;
  height: 200%;
  background: linear-gradient(
    to bottom right,
    rgba(255, 255, 255, 0) 0%,
    rgba(255, 255, 255, 0.03) 40%,
    rgba(255, 255, 255, 0.15) 50%,
    rgba(255, 255, 255, 0) 60%,
    rgba(255, 255, 255, 0) 100%
  );
  transform: rotate(30deg);
  pointer-events: none;
}

.emergency-card {
  animation: emergencyBg 3s infinite alternate;
}
@keyframes emergencyBg {
  0% {
    background-color: #dc3545;
  }
  100% {
    background-color: #b02a37;
  }
}

.name-text {
  font-size: 2.2rem;
  letter-spacing: 1px;
}
.queue-number {
  font-family: 'Courier New', Courier, monospace;
}
</style>
