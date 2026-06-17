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
            <input type="text" class="form-control" v-model="patientName" placeholder="患者姓名" required>
          </div>
          <div class="col-4">
            <button type="button" class="btn btn-outline-primary w-100" @click="findOrCreatePatient">
              <i class="bi bi-search"></i> 查询/注册
            </button>
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

const departments = ['内科', '外科', '妇产科', '儿科', '急诊科', '眼科', '口腔科', '皮肤科']
const doctors = ref<any[]>([])
const patientList = ref<any[]>([])

const patientName = ref('')
const currentPatient = reactive({ id: '', name: '' })

const form = reactive({
  department: '',
  doctorId: '',
  isUrgent: false,
  isEmergency: false
})

const loadDoctors = async () => {
  try {
    doctors.value = await request.get('/api/doctors')
  } catch (e) {}
}

const loadPatients = async () => {
  try {
    patientList.value = await request.get('/api/patients')
  } catch (e) {}
}

onMounted(() => {
  loadDoctors()
  loadPatients()
})

const findOrCreatePatient = async () => {
  if (!patientName.value.trim()) {
    Swal.fire({ icon: 'warning', title: '请输入患者姓名' })
    return
  }
  try {
    const phone = '138' + String(Math.floor(Math.random() * 100000000)).padStart(8, '0')
    const idCard = '11010119900101' + String(Math.floor(Math.random() * 10000)).padStart(4, '0')
    
    // axios 会返回已解包的 response.data，所以这其实是包含 id 的实体
    const patient: any = await request.post('/api/patients', {
      name: patientName.value,
      phone: phone,
      id_card: idCard,
      age: Math.floor(Math.random() * 50) + 20,
      gender: Math.random() > 0.5 ? '男' : '女'
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

const selectPatient = (p: any) => {
  currentPatient.id = p.id
  currentPatient.name = p.name
  patientName.value = p.name
}

const submitAppointment = async () => {
  let priority = 'normal'
  if (form.isEmergency) priority = 'emergency'
  else if (form.isUrgent) priority = 'urgent'

  const body: any = {
    patient_id: parseInt(currentPatient.id, 10),
    priority
  }

  if (form.doctorId) {
    body.doctor_id = parseInt(form.doctorId, 10)
  } else {
    body.department = form.department
  }

  try {
    const endpoint = form.doctorId ? '/api/appointments' : '/api/appointments/auto'
    const result: any = await request.post(endpoint, body)
    
    // 如果不指定医生的话，后端会自动分配一个
    Swal.fire({
      icon: 'success',
      title: '挂号成功',
      html: `<div class="text-start mt-3">
               <p><strong>挂号单号：</strong>${result.appointment_id || result.id || 'N/A'}</p>
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
