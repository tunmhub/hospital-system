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
            <div class="stats-number" style="font-size: 2.5rem; color: #2563eb; font-weight: bold;">{{ doctors.length }}</div>
            <div class="text-muted fw-semibold">当班医生总数</div>
          </div>
        </div>
      </div>
      <div class="col-md-4 mb-3">
        <div class="card border-0 shadow-sm h-100 py-3">
          <div class="card-body text-center d-flex flex-column align-items-center justify-content-center">
            <div class="stats-number" style="font-size: 2.5rem; color: #10b981; font-weight: bold;">{{ totalWaiting }}</div>
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
             <p class="text-muted small mb-0">为了保证您获取的排队和负载数据是最新的，系统将每隔 15 秒发起一次隐脉冲拉取动作。</p>
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
        <div v-if="doctors.length === 0" class="empty-state py-5">
          <i class="bi bi-person-x text-secondary" style="font-size: 4rem;"></i>
          <p class="mt-3 text-secondary fs-5">暂无当班医生或网络错误</p>
        </div>
        
        <div class="row gx-4 gy-4">
          <div class="col-md-6 col-lg-4" v-for="doc in doctors" :key="doc.id">
            <div class="bg-white p-3 border rounded shadow-sm d-flex align-items-center justify-content-between h-100 card-hover-effect">
              <div>
                <div class="fw-bold fs-5 mb-1">{{ doc.name }} 
                  <span class="badge bg-secondary ms-1 fs-6 fw-normal">{{ doc.title || '医师' }}</span>
                </div>
                <div class="text-muted small">
                  {{ doc.department }}
                  <span class="ms-2">|</span>
                  等待: <span class="text-primary fw-bold">{{ waitingCounts[doc.id] || 0 }}</span>
                  <span class="ms-2">|</span>
                  负载: <span :class="(doc.current_patients >= doc.max_patients) ? 'text-danger fw-bold' : 'text-primary'">{{ doc.current_patients }}/{{ doc.max_patients }}</span>
                </div>
              </div>
              <button 
                class="btn btn-lg btn-success call-next-btn shadow-sm" 
                @click="callNext(doc)" 
                :disabled="doc.current_patients === 0">
                <i class="bi bi-megaphone fs-5"></i> 叫号
              </button>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'
import request from '@/utils/request'
import Swal from 'sweetalert2'

const doctors = ref<any[]>([])
const waitingCounts = ref<Record<number, number>>({})
let timer: any = null

const totalWaiting = computed(() => {
  return Object.values(waitingCounts.value).reduce((sum, count) => sum + count, 0)
})

const loadDoctors = async () => {
  try {
    doctors.value = await request.get('/api/doctors')
    // 加载每个医生的真正等待人数
    await loadWaitingCounts()
  } catch (e) {
    // 已经被 request.ts 中的阻断拦截报出
  }
}

const loadWaitingCounts = async () => {
  const counts: Record<number, number> = {}
  for (const doc of doctors.value) {
    try {
      const queue: any[] = await request.get(`/api/doctors/${doc.id}/queue`)
      counts[doc.id] = queue.filter(item => item.status === 'waiting').length
    } catch (e) {
      counts[doc.id] = 0
    }
  }
  waitingCounts.value = counts
}

const callNext = async (doc: any) => {
  try {
    const result: any = await request.post(`/api/doctors/${doc.id}/call_next`)
    Swal.fire({
      title: '<strong>大厅呼叫成功</strong>',
      html: `<div style="font-size: 1.5rem; margin: 1rem 0;">
               请下一位患者进入诊室就诊
               <div style="color: #10b981; font-weight: bold; font-size: 2.2rem; margin-top: 1rem;">${result.patient_name || '患者'}</div>
             </div>`,
      icon: 'info',
      timer: 8000,
      timerProgressBar: true,
      confirmButtonText: '收到',
      confirmButtonColor: '#10b981'
    })
    loadDoctors()
  } catch (e) {}
}

onMounted(() => {
  loadDoctors()
  timer = window.setInterval(loadDoctors, 15000)
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
  box-shadow: 0 10px 25px rgba(0,0,0,0.1) !important;
}
.call-next-btn {
  border-radius: 12px;
}
.call-next-btn:not(:disabled):hover {
  transform: scale(1.05);
}
</style>
