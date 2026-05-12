import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import mkcert from 'vite-plugin-mkcert'

// https://vite.dev/config/
export default defineConfig({
  plugins: [vue(), mkcert()],
  server: {
    host: '0.0.0.0',
    https: true,
    watch: { usePolling: true },
  },
})
