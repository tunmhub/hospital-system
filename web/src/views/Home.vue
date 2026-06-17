<template>
  <div class="home-container">
    <div class="row w-100 justify-content-center">
      <div class="col-md-8 text-center pt-5 text-white">
        <h1 class="mb-4 fw-bold"><i class="bi bi-hospital"></i> 医院挂号系统</h1>
        <p class="lead mb-5">请选择要进入的系统终端</p>
        
        <div class="row gx-4 gy-4">
          <div class="col-md-4">
            <router-link to="/patient" class="text-decoration-none">
              <div class="card h-100">
                <div class="card-body py-5">
                  <i class="bi bi-person-plus text-primary" style="font-size: 3rem;"></i>
                  <h4 class="mt-3 text-dark">自助挂号机</h4>
                  <p class="text-muted small">患者建档、快速挂号</p>
                </div>
              </div>
            </router-link>
          </div>
          
          <div class="col-md-4">
            <router-link to="/doctor" class="text-decoration-none">
              <div class="card h-100">
                <div class="card-body py-5">
                  <i class="bi bi-person-workspace text-success" style="font-size: 3rem;"></i>
                  <h4 class="mt-3 text-dark">门诊工作台</h4>
                  <p class="text-muted small">呼叫下一位、查看队列</p>
                </div>
              </div>
            </router-link>
          </div>
          
          <div class="col-md-4">
            <router-link to="/screen" class="text-decoration-none">
              <div class="card h-100">
                <div class="card-body py-5">
                  <i class="bi bi-display text-dark" style="font-size: 3rem;"></i>
                  <h4 class="mt-3 text-dark">排队大屏</h4>
                  <p class="text-muted small">大厅实时滚动队列播报</p>
                </div>
              </div>
            </router-link>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.home-container {
  min-height: calc(100vh - 70px);
  display: flex;
  align-items: center;
  justify-content: center;
}
.card {
  transition: transform 0.3s;
}
.card:hover {
  transform: translateY(-10px);
  border-bottom: 4px solid var(--primary-color);
}
</style>
