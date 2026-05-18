import { defineStore } from 'pinia'
import { computed, ref } from 'vue'

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
		setCurrentProfile
	}
})
