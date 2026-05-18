<script setup lang="ts">
import type { MacroButton } from '../types/macropad';

defineProps<{
    button: MacroButton
}>()

const emit = defineEmits<{
    click: [buttonId: number]
}>()

function handleClick(id: number) {
    emit('click', id)
}
</script>

<template>
    <button
        class="macro-btn"
        @click="handleClick(button.id)"
    >
        <div class="btn-id">
            {{ button.id + 1 }}
        </div>
        <div class="btn-action">
            {{ button.action?.value || 'EMPTY' }}
        </div>
    </button>
</template>

<style scoped>
.macro-btn {
    aspect-ratio: 1;
    border: none;
    border-radius: 18px;
    background: #202020;
    color: white;
    cursor: pointer;

    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;

    padding: 12px;

    transition: 0.15s ease;
}

.macro-btn:hover {
    background: #2f2f2f;
    transform: translateY(-2px);
}

.btn-id {
    font-size: 12px;
    opacity: 0.5;
    margin-bottom: 10px;
}

.btn-action {
    font-size: 14px;
    font-weight: 600;
    text-align: center;
    word-break: break-word;
}
</style>