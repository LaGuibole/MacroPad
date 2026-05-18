export interface Action {
    value: string
}

export interface MacroButton {
    id: number
    action: Action | null
}

export interface Profile {
    id: number
    name: string
    buttons: MacroButton[]
}