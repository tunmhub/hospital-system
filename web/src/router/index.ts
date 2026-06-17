import { createRouter, createWebHistory, type RouteRecordRaw } from 'vue-router'

const routes: Array<RouteRecordRaw> = [
  {
    path: '/',
    name: 'Home',
    component: () => import('@/views/Home.vue')
  },
  {
    path: '/patient',
    name: 'Patient',
    component: () => import('@/views/patient/index.vue'),
    meta: { title: '挂号终端' }
  },
  {
    path: '/doctor',
    name: 'Doctor',
    component: () => import('@/views/doctor/index.vue'),
    meta: { title: '医生叫号工作台' }
  },
  {
    path: '/screen',
    name: 'Screen',
    component: () => import('@/views/screen/index.vue'),
    meta: { title: '大厅排队显示屏' }
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

router.beforeEach((to, _from, next) => {
  if (to.meta.title) {
    document.title = `${to.meta.title} - 医院挂号系统`
  }
  next()
})

export default router
