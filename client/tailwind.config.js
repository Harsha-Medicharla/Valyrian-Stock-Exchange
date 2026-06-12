/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      fontFamily: {
        sans: ['DM Sans', 'sans-serif'],
        mono: ['IBM Plex Mono', 'monospace'],
      },
      colors: {
        bg: '#0a0a0a',
        surface: '#111111',
        border: '#1f1f1f',
        'text-primary': '#e8e8e8',
        'text-muted': '#6b6b6b',
        profit: '#22c55e',
        loss: '#ef4444',
        info: '#3b82f6',
        warn: '#f59e0b',
      },
    },
  },
  plugins: [],
}
