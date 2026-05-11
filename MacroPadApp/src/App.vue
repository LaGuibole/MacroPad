<script setup lang="ts">
import BleStatus from './components/BleStatus.vue'
</script>

<template>
  <main>
    <h1>MacroPad</h1>
    <BleStatus />
  </main>
</template>

<style scoped>
main {
  padding: 32px;
  display: flex;
  flex-directions: column;
  gap: 24px;
}
</style>