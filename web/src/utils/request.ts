import axios, { type AxiosRequestConfig } from 'axios'
import Swal from 'sweetalert2'

// 扩展 AxiosRequestConfig，支持 silent 配置
declare module 'axios' {
  interface AxiosRequestConfig {
    silent?: boolean
  }
}

const request = axios.create({
  baseURL: '/api',
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json'
  }
})

// 请求拦截器
request.interceptors.request.use(
  (config) => {
    // 可以在这里添加 token 等头信息
    return config
  },
  (error) => {
    return Promise.reject(error)
  }
)

// 响应拦截器
request.interceptors.response.use(
  (response) => {
    // 业务正常时的提取
    return response.data
  },
  (error) => {
    const errorMsg = error.response?.data?.error || error.message || '网络请求异常'

    // 静默模式下不弹出错误提示（用于轮询等场景）
    if (!error.config?.silent) {
      Swal.fire({
        icon: 'error',
        title: '系统提示',
        text: errorMsg,
        confirmButtonColor: '#ef4444'
      })
    }

    return Promise.reject(error)
  }
)

export default request
