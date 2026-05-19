import { defineStore } from 'pinia'
import { computed, ref } from 'vue'
import { useBLE } from '../ble/useBLE'

import type { Action, 
			MacroButton,
			Profile 
} from '../types/macropad.ts'

function createEmptyButton(id: number): MacroButton
{
	return {
		id,
		action: null
	}
}

function createEmptyProfile(id: number): Profile {
	return {
		id,
		name: `Profile ${id + 1}`,
		buttons: Array.from(
			{ length: 9 },
			(_, index) => createEmptyButton(index)
		)
	}
}

export const useMacroPadStore = defineStore('macropad', () => {
	// state
	const profiles = ref<Profile[]>(
		Array.from(
			{ length: 4 },
			(_, index) => createEmptyProfile(index)
		)
	)
	
	const currentProfile = ref(0)

	const selectedButton = ref<number | null> (null)

	const loading = ref(false)

	const syncing = ref(false)

	// getters
	const currentProfileData = computed(() => {
		return profiles.value[currentProfile.value]
	})

	const selectedButtonData = computed(() => {
		if (selectedButton.value === null)
			return null
		return currentProfileData.value.buttons[selectedButton.value]
	})

	// actions
	function selectButton(buttonId: number){
		selectedButton.value = buttonId
	}

	function clearSelectedButton() {
		selectedButton.value = null
	}

	function setButtonAction(profileId: number, buttonId: number, action: string) 
	{
		const profile = profiles.value[profileId]

		if (!profile)
			return

		const button = profile.buttons[buttonId]

		if (!button)
			return

		button.action = {
			value: action
		}
	}

	function setCurrentProfile(profileId: number) {
		if (profileId < 0 || profileId > profiles.value.length)
			return

		currentProfile.value = profileId
	}

async function syncFromDevice() {
	const ble = useBLE()
	loading.value = true
	try {
		await ble.sendCommand({ cmd: 'getall' })
	} catch (e) {
		console.error('syncFromDevice failed', e)
	} finally {
		loading.value = false
	}
}

async function pushButtonToDevice(profileId: number, buttonId: number) {
	const ble = useBLE()
	const action = profiles.value[profileId].buttons[buttonId].action?.value ?? ''
	syncing.value = true
	try {
		await ble.sendCommand({
			cmd: 'set',
			profile: profileId + 1,
			btn: buttonId + 1,
			action
		})
		await ble.sendCommand({ cmd: 'save' })
	} catch (e) {
		console.error('pushButtonToDevice failed', e)
	} finally {
		syncing.value = false
	}
}

function handleBleMessage(json: string) {
	try {
		const msg = JSON.parse(json)
		console.log('handleBleMessage', msg) // debug
		if (!msg.ok)
			return
		if (msg.profiles) 
		{
			for (const p of msg.profiles) {
				const profileIdx = p.profile - 1
				for (const b of p.buttons) {
					const btnIdx = b.btn - 1
					profiles.value[profileIdx].buttons[btnIdx].action = b.action? { value: b.action } : null
				} 
			}
		}
	} catch (e) {
		console.error('handleBleMessage parse error', e)
	}
}

	return {
		//state
		profiles,
		currentProfile,
		selectedButton,
		loading,
		syncing,

		// getters
		currentProfileData,
		selectedButtonData,

		//actions
		selectButton,
		clearSelectedButton,
		setButtonAction,
		setCurrentProfile,

		// sync
		syncFromDevice,
		pushButtonToDevice,
		handleBleMessage
	}
})
