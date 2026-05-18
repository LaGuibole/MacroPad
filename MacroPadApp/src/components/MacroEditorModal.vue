<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref } from 'vue'
import { useMacroPadStore } from '../stores/macropad';

const store = useMacroPadStore()
const currentShortCut = ref('')

const isOpen = computed (() => {
    return store.selectedButton !== null
})

const selectedButton = computed (() => {
    return store.selectedButtonData
})

function formatKey(event: KeyboardEvent) {
    const keys: string[] = []

    if (event.ctrlKey)
        keys.push('CTRL')
    if (event.shiftKey)
        keys.push('SHIFT')
    if (event.altKey)
        keys.push('ALT')
    if (event.metaKey)
        keys.push('WIN')

    const ignored = [
        'Control',
        'Shift',
        'Alt',
        'Meta'
    ]

    if (!ignored.includes(event.key)){
        keys.push(event.key.toUpperCase())
    }
    return keys.join('+')
}

function handleKeyDown(event: KeyboardEvent) {
    if (!isOpen.value)
        return
    event.preventDefault()
    currentShortCut.value = formatKey(event)
}

function handleSave() {
    if (store.selectedButton === null)
        return
    store.setButtonAction(
        store.currentProfile,
        store.selectedButton,
        currentShortCut.value
    )

    handleClose()
}

function handleClose() {
    store.clearSelectedButton()
    currentShortCut.value = ''
}

onMounted(() => {
    window.addEventListener('keydown', handleKeyDown)
})

onUnmounted(() => {
    window.removeEventListener('keydown', handleKeyDown)
})
</script>


<template>
    <div
        v-if="isOpen"
        class="overlay"
        @click.self="handleClose"
    >
        <div class="modal">
            <h2>
                Button {{ (store.selectedButton ?? 0) + 1}}
            </h2>

            <p class="subtitle">
                Press shortcut.
            </p>

            <div class="shortcut-preview">
                {{ currentShortCut || 'Waiting input ...' }}
            </div>

            <div class="actions">
                <button
                class="secondary"
                @click="handleClose"
                >
                Cancel
                </button>
                <button
                class="primary"
                @click="handleSave"
                >
                Save
                </button>
            </div>
        </div>
    </div>
</template>

<style scoped>
.overlay {
    position: fixed;
    inset: 0;
    background: rgba(0, 0, 0, 0.6);
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: 1000;
}

.modal {
    width: 420px;
    background: #1c1c1c;
    color: white;
    padding: 24px;
    border-radius: 18px;
}

h2 {
    margin-top: 0;
    margin-bottom: 8px;
}

.subtitle {
    opacity: 0.7;
    margin-bottom: 20px;
}

.shortcut-preview {
    height: 70px;
    border-radius: 12px;
    background: #2b2b2b;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 20px;
    font-weight: bold;
    margin-bottom: 24px;
}

.actions {
    display: flex;
    justify-content: flex-end;
    gap: 12px;
}

button {
    border: none;
    border-radius: 10px;
    padding: 10px 16px;
    cursor: pointer;
}

.primary {
    background: #646cff;
    color: white;
}

.secondary {
    background: #333;
    color: white;
}
</style>