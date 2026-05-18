<script setup lang="ts">
import { useMacroPadStore } from '../stores/macropad';

const store = useMacroPadStore()

function handleSelect(profileId: number) {
    store.setCurrentProfile(profileId)
}
</script>

<template>
    <div class="profiles">
        <button
            v-for="profile in store.profiles"
            :key="profile.id"
            class="profile-btn"
            :class="{
                active: profile.id === store.currentProfile
            }"    
            @click="handleSelect(profile.id)"
        >
            {{ profile.name }}
        </button>
    </div>
</template>

<style scoped>
.profiles {
    display: flex;
    gap: 12px;
    margin-bottom: 24px;
    flex-wrap: wrap;
}

.profile-btn {
    border: none;
    padding: 12px 16px;
    border-radius: 10px;
    background: #2a2a2a;
    color: white;
    cursor: pointer;
    font-size: 14px;
    transition: 0.15s ease;
}

.profile-btn:hover {
    background: #3a3a3a;
}

.profile-btn.active {
    background: #646cff;
}

</style>