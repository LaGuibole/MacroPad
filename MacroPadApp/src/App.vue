<script setup lang="ts">
import BleStatus from './components/BleStatus.vue'
import ProfileSelector from './components/ProfileSelector.vue';
import MacroGrid from './components/MacroGrid.vue';
import MacroEditorModal from './components/MacroEditorModal.vue';

</script>

<template>
  <main>
    <h1>MacroPad</h1>
    <BleStatus />
    <h1>MacroPad Configurator :</h1>
    
    <ProfileSelector />

    <MacroGrid />

    <MacroEditorModal />
    
  </main>
</template>

<style scoped>
main {
  min-height: 100vh;
  background: #111;
  color:white;
  padding: 40px;
  font-family: Inter, sans-serif;
}

h1 {
  margin-top: 0;
  margin-bottom: 32px;
}
</style>