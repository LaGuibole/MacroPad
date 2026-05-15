import { ref } from 'vue'

const UART_SERVICE_UUID		= '6e400001-b5a3-f393-e0a9-e50e24dcca9e'
const UART_TX_UUID 			= '6e400003-b5a3-f393-e0a9-e50e24dcca9e'
const UART_RX_UUID 			= '6e400002-b5a3-f393-e0a9-e50e24dcca9e'

export type BleStatus = 'disconnected' | 'connecting' | 'connected' | 'error'

const status = ref<BleStatus>('disconnected')
const lastError = ref<string | null>(null)

let rxChar: BluetoothRemoteGATTCharacteristic | null = null
let txChar: BluetoothRemoteGATTCharacteristic | null = null

async function connect() {
	console.log('Trying to connect via connect() in useBLE.ts')
	try {
		status.value	= 'connecting'
		lastError.value	= null

		const device = await navigator.bluetooth.requestDevice({
			filters: [{ name: 'MacroPad' }],
			optionalServices: [UART_SERVICE_UUID]
		})

		device.addEventListener('gattserverdisconnected', () => {
			status.value = 'disconnected'
			rxChar = null
			txChar = null
		})

		const server = await device.gatt!.connect()
		const service = await server.getPrimaryService(UART_SERVICE_UUID)
		
		rxChar = await service.getCharacteristic(UART_RX_UUID)
		txChar = await service.getCharacteristic(UART_TX_UUID)

		await txChar.startNotifications()
		txChar.addEventListener('characteristicvaluechanged', onNotify)

		status.value = 'connected'
	} catch (e: any) {
		console.log('ERREUR BLE:', e?.message, e?.name, e)
		status.value = 'error'
		lastError.value = e?.message ?? 'Unknown Error'
	}
}

function onNotify(event: Event) {
	const value = (event.target as BluetoothRemoteGATTCharacteristic).value!
	const text = new TextDecoder().decode(value)
	console.log('BLE RX ', text)
}

async function sendCommand(json: object) {
	if (!rxChar) return
	const encoded = new TextEncoder().encode(JSON.stringify(json))
	await rxChar.writeValueWithoutResponse(encoded)
}

export function useBLE() {
	return { status, lastError, connect, sendCommand }
}