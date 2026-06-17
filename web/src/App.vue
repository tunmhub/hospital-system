<template>
  <nav class="navbar navbar-expand-lg navbar-light fixed-top" v-if="showHeader">
    <div class="container">
      <router-link class="navbar-brand fw-bold" to="/">
        <i class="bi bi-hospital text-primary"></i> 医院挂号系统
      </router-link>
      <div class="d-flex align-items-center">
        <span class="badge bg-success rounded-pill px-3 py-2">
          <span class="live-indicator"></span>系统运行中
        </span>
      </div>
    </div>
  </nav>

  <div :class="showHeader ? 'main-content-pt' : ''">
    <!-- Router View -->
    <router-view v-slot="{ Component }">
      <transition name="fade" mode="out-in">
        <component :is="Component" />
      </transition>
    </router-view>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useRoute } from 'vue-router'

const route = useRoute()
// 排队大屏由于是电视机显示，通常不显示导航栏
const showHeader = computed(() => route.path !== '/screen')
</script>

<style>
.main-content-pt {
  padding-top: 70px;
}
.live-indicator {
  display: inline-block;
  width: 8px;
  height: 8px;
  background: #fff;
  border-radius: 50%;
  margin-right: 0.5rem;
  animation: blink 1.5s infinite;
}
@keyframes blink {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.3; }
}

.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.2s ease;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}
</style>
