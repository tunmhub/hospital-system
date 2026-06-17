import axios from 'axios'
import Swal from 'sweetalert2'

const request = axios.create({
  // 开发环境下使用 vite 代理，生产环境下使用相对路径
  baseURL: import.meta.env.DEV ? '' : '',
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
    
    // 不要对自动刷新产生的无感错误弹出（如果有需要在API中配置静默标识也可，这里先全拦截）
    Swal.fire({
      icon: 'error',
      title: '系统提示',
      text: errorMsg,
      confirmButtonColor: '#ef4444'
    })
    
    return Promise.reject(error)
  }
)

export default request
