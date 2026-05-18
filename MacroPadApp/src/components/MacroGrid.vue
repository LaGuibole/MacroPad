<script setup lang="ts">
import MacroButton from './MacroButton.vue';
import { useMacroPadStore } from '../stores/macropad';

const store = useMacroPadStore()

function handleButtonClick(buttonId: number) {
    store.selectButton(buttonId)
}
</script>

<template>
    <div class="grid">
        <MacroButton
            v-for="button in store.currentProfileData.buttons"
            :key="button.id"
            :button="button"
            @click="handleButtonClick"
        />
    </div>
</template>

<style scoped>
.grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 16px;
    max-width: 500px;
}
</style>