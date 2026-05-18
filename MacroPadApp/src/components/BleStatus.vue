<script setup lang="ts">
import { useBLE } from '../ble/useBLE'
import { useMacroPadStore } from '../stores/macropad'

const { status, lastError, connect } = useBLE()
const store = useMacroPadStore()

const label: Record<string, string> = {
    disconnected: 'Connect',
    connecting: 'Connecting ...',
    connected: 'Connected !',
    error: 'Try again',
}

async function handleConnect() {
    await connect()
    if (status.value === 'connected') {
        await store.syncFromDevice()
    }
}
</script>

<template>
    <div class="ble-status">
        <button 
            :disabled="status === 'connecting'" 
            @click="handleConnect"
        >
            {{ label[status] }}
        </button>
        <span :class="['dot', status]" />
        <p v-if="status === 'error'" class="error">{{ lastError }}</p>
    </div>
</template>

<style scoped>
.ble-status {
    display: flex;
    align-items: center;
    gap: 12px;
}

.dot {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: gray;
}

.dot.connected {
    background: #22c55e;
}

.dot.connecting {
    background: #f59e0b;
}

.dot.error {
    background: #ef4444;
}

.dot.disconnected {
    background: #6b7280;
}

.error {
    color: #ef4444;
    font-size: 13px;
}
</style>