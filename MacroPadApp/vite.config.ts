import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import mkcert from 'vite-plugin-mkcert'
// import fs from 'fs'

// https://vite.dev/config/
// export default defineConfig({
//   plugins: [vue()],
//   server: {
//     host: 'localhost',
//     port: 5173,
//     https: {
//     key: fs.readFileSync('./key.pem'),
//     cert: fs.readFileSync('./cert.pem'),
//   },
//     watch: { usePolling: true },
//   },
// })

// https://vite.dev/config/
export default defineConfig({
  plugins: [vue(), mkcert()],
  server: {
    host: 'localhost',
    port: 5173,
    https: true,
    watch: { usePolling: true },
  },
})
