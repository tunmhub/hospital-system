<template>
  <div class="screen-page min-h-screen bg-slate-950 text-white relative overflow-hidden">
    <!-- 背景网格 -->
    <div class="fixed inset-0 opacity-[0.03] pointer-events-none"
      style="background-image: linear-gradient(rgba(255,255,255,.1) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,.1) 1px, transparent 1px); background-size: 60px 60px;">
    </div>

    <!-- 主容器 -->
    <div class="relative z-10 max-w-7xl mx-auto px-6 py-6 flex flex-col min-h-screen">

      <!-- ========== 顶部区域 ========== -->
      <div class="flex items-center justify-between mb-6">
        <!-- 左侧：返回 + 标题 -->
        <div class="flex items-center gap-4">
          <router-link
            to="/"
            class="w-9 h-9 rounded-lg bg-white/5 border border-white/10 flex items-center justify-center text-gray-500 hover:text-white hover:bg-white/10 transition-all"
          >
            <i class="bi bi-arrow-left text-sm"></i>
          </router-link>
          <div>
            <h1 class="text-2xl font-black tracking-wide leading-none">
              <span class="text-cyan-400">门诊</span><span class="text-white">排队大屏</span>
            </h1>
            <div class="text-xs text-gray-500 mt-1 tracking-wider">OUTPATIENT QUEUE DISPLAY</div>
          </div>
        </div>

        <!-- 右侧：医生选择 + 时间 -->
        <div class="flex items-center gap-4">
          <el-select
            v-model="selectedDoctor"
            placeholder="选择投屏诊室"
            size="large"
            class="screen-doctor-select"
            @change="refreshQueue"
          >
            <el-option
              v-for="doc in doctorStore.doctors"
              :key="doc.id"
              :label="`${doc.name} · ${doc.department}`"
              :value="doc.id"
            />
          </el-select>
          <div class="text-right">
            <div class="text-xs text-gray-500 tracking-wider">AUTO REFRESH</div>
            <div class="text-sm font-mono text-cyan-300/60 mt-0.5">10s</div>
          </div>
        </div>
      </div>

      <!-- ========== 未选择状态 ========== -->
      <div v-if="!selectedDoctor" class="flex-1 flex flex-col items-center justify-center opacity-40">
        <div class="w-24 h-24 rounded-full border-2 border-dashed border-gray-600 flex items-center justify-center mb-6">
          <i class="bi bi-cast text-4xl text-gray-500"></i>
        </div>
        <div class="text-xl text-gray-400 font-light tracking-wider">请选择投屏诊室</div>
        <div class="text-sm text-gray-600 mt-2">在右上角下拉框中选择一位医生</div>
      </div>

      <!-- ========== 空队列状态 ========== -->
      <div v-else-if="queue.length === 0" class="flex-1 flex flex-col items-center justify-center">
        <div class="relative mb-6">
          <div class="w-24 h-24 rounded-full bg-emerald-500/10 flex items-center justify-center pulse-glow">
            <i class="bi bi-check-circle text-4xl text-emerald-400"></i>
          </div>
        </div>
        <div class="text-2xl font-bold text-emerald-400 tracking-wider">当前诊室无人排队</div>
        <div class="text-sm text-gray-500 mt-2">系统将自动刷新 · 每 10 秒</div>
      </div>

      <!-- ========== 队列展示区 ========== -->
      <div v-else class="flex-1 flex flex-col">
        <!-- 顶部统计条 -->
        <div class="grid grid-cols-3 gap-4 mb-6">
          <div class="rounded-xl bg-white/[0.03] border border-white/[0.06] px-5 py-3 text-center">
            <div class="text-3xl font-black text-cyan-400 leading-none">{{ queue.length }}</div>
            <div class="text-xs text-gray-500 mt-1.5 tracking-wider">等候人数</div>
          </div>
          <div class="rounded-xl bg-white/[0.03] border border-white/[0.06] px-5 py-3 text-center">
            <div class="text-3xl font-black text-amber-400 leading-none">
              {{ queue.filter(i => String(i.priority).toLowerCase() === 'urgent').length }}
            </div>
            <div class="text-xs text-gray-500 mt-1.5 tracking-wider">加急</div>
          </div>
          <div class="rounded-xl bg-white/[0.03] border border-white/[0.06] px-5 py-3 text-center">
            <div class="text-3xl font-black text-red-400 leading-none">
              {{ queue.filter(i => String(i.priority).toLowerCase() === 'emergency').length }}
            </div>
            <div class="text-xs text-gray-500 mt-1.5 tracking-wider">急诊</div>
          </div>
        </div>

        <!-- 队列卡片网格 -->
        <div class="grid grid-cols-1 md:grid-cols-2 xl:grid-cols-3 gap-4 flex-1">
          <div
            v-for="(item, index) in queue"
            :key="item.id"
            class="queue-card group relative rounded-2xl border transition-all duration-500"
            :class="getCardClass(item.priority)"
            :style="{ animationDelay: `${index * 60}ms` }"
          >
            <!-- 发光边框 -->
            <div class="absolute inset-0 rounded-2xl opacity-0 group-hover:opacity-100 transition-opacity duration-500 pointer-events-none glow-cyan"></div>

            <div class="relative p-5 flex flex-col h-full">
              <!-- 顶部：号码 + 优先级 -->
              <div class="flex items-start justify-between mb-3">
                <div class="queue-number text-5xl font-black leading-none tracking-tighter"
                  :class="{
                    'text-red-300': String(item.priority).toLowerCase() === 'emergency',
                    'text-amber-300': String(item.priority).toLowerCase() === 'urgent',
                    'text-white/80': String(item.priority).toLowerCase() === 'normal'
                  }">
                  #{{ item.queue_number }}
                </div>
                <span class="px-3 py-1 rounded-full text-xs font-bold tracking-wider"
                  :class="getBadgeClass(item.priority)">
                  {{ getPriorityText(item.priority) }}
                </span>
              </div>

              <!-- 中部：患者姓名 -->
              <div class="text-xl font-bold text-white/90 mb-4 truncate">
                {{ item.patient_name || `患者${item.patient_id}` }}
              </div>

              <!-- 底部：等待时间 -->
              <div class="mt-auto flex items-center gap-2 text-sm"
                :class="item.wait_minutes !== null ? 'text-gray-400' : 'text-gray-600'">
                <i class="bi bi-hourglass-split"></i>
                <span v-if="item.wait_minutes !== null">
                  预计等待 <span class="font-bold text-white/70">{{ item.wait_minutes }}</span> 分钟
                </span>
                <span v-else class="animate-pulse">计算中...</span>
              </div>

              <!-- 呼叫中指示器（第一位） -->
              <div v-if="index === 0" class="absolute top-3 right-3">
                <span class="flex h-3 w-3">
                  <span class="animate-ping absolute inline-flex h-full w-full rounded-full bg-cyan-400 opacity-75"></span>
                  <span class="relative inline-flex rounded-full h-3 w-3 bg-cyan-500"></span>
                </span>
              </div>
            </div>
          </div>
        </div>

        <!-- 底部提示条 -->
        <div class="mt-6 flex items-center justify-center gap-6 text-xs text-gray-600">
          <span class="flex items-center gap-1.5">
            <span class="w-2 h-2 rounded-full bg-cyan-500/50"></span> 自动刷新 · 每 10 秒
          </span>
          <span class="flex items-center gap-1.5">
            <span class="w-2 h-2 rounded-full bg-emerald-500/50 animate-pulse"></span> 实时同步中
          </span>
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
/* 深色背景覆盖 */
.screen-page {
  font-family: 'SF Pro Display', -apple-system, 'PingFang SC', 'Microsoft YaHei', sans-serif;
}

/* 医生选择器深色主题 */
.screen-doctor-select :deep(.el-input__wrapper) {
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 0.75rem;
  box-shadow: none;
}
.screen-doctor-select :deep(.el-input__inner) {
  color: #e2e8f0;
}
.screen-doctor-select :deep(.el-input__wrapper:hover),
.screen-doctor-select :deep(.el-input.is-focus .el-input__wrapper) {
  border-color: rgba(34, 211, 238, 0.4);
  box-shadow: 0 0 0 1px rgba(34, 211, 238, 0.2);
}

/* 队列卡片入场动画 */
.queue-card {
  animation: cardSlideIn 0.5s cubic-bezier(0.16, 1, 0.3, 1) both;
  border: 1px solid rgba(255, 255, 255, 0.06);
  background: rgba(255, 255, 255, 0.02);
}
@keyframes cardSlideIn {
  from { opacity: 0; transform: translateY(20px) scale(0.97); }
  to   { opacity: 1; transform: translateY(0) scale(1); }
}

/* 急诊卡片 - 红色系 */
.queue-card.emergency-card,
.queue-card[class*="bg-danger"] {
  background: linear-gradient(135deg, rgba(239, 68, 68, 0.15) 0%, rgba(220, 38, 38, 0.08) 100%);
  border-color: rgba(239, 68, 68, 0.3);
  animation: cardSlideIn 0.5s cubic-bezier(0.16, 1, 0.3, 1) both, emergencyPulse 3s ease-in-out infinite;
}
@keyframes emergencyPulse {
  0%, 100% { box-shadow: 0 0 0 0 rgba(239, 68, 68, 0); }
  50%      { box-shadow: 0 0 30px -5px rgba(239, 68, 68, 0.15); }
}

/* 加急卡片 - 琥珀系 */
.queue-card.urgent-card,
.queue-card[class*="bg-warning"] {
  background: linear-gradient(135deg, rgba(245, 158, 11, 0.12) 0%, rgba(217, 119, 6, 0.06) 100%);
  border-color: rgba(245, 158, 11, 0.25);
}

/* 普通卡片 */
.queue-card[class*="bg-secondary"] {
  background: rgba(255, 255, 255, 0.03);
  border-color: rgba(255, 255, 255, 0.08);
}

/* 发光效果 */
.glow-cyan    { box-shadow: 0 0 30px -5px rgba(34, 211, 238, 0.2); }
.glow-red     { box-shadow: 0 0 30px -5px rgba(239, 68, 68, 0.3); }
.glow-amber   { box-shadow: 0 0 30px -5px rgba(245, 158, 11, 0.2); }

/* 排队号码字体 */
.queue-number {
  font-family: 'SF Mono', 'Fira Code', 'Cascadia Code', monospace;
}

/* 等待中脉冲光晕 */
.pulse-glow {
  animation: pulseGlow 2.5s ease-in-out infinite;
}
@keyframes pulseGlow {
  0%, 100% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0.2); }
  50%      { box-shadow: 0 0 40px 10px rgba(16, 185, 129, 0.1); }
}
</style>
