import Chart, { type ChartConfiguration } from "chart.js/auto"

const el_canvas = document.getElementById("realtimeChart") as HTMLCanvasElement
const el_time = document.getElementById("lastUpdate") as HTMLDivElement
const el_select = document.getElementById("topicSelect") as HTMLSelectElement
const el_check = document.getElementById("checkStop") as HTMLInputElement

let topic: string = "temp"
let stop: boolean = false

el_select.addEventListener("change", () => {
  topic = el_select.value
})

el_check.addEventListener("change", () => {
  stop = el_check.checked
})

const ctx = el_canvas.getContext("2d")

const data = {
  datasets: [
    {
      label: "data0",
      data: [],
      borderColor: "rgba(255,99,132,1)",
      backgroundColor: "rgba(255,99,132,0.2)",
      yAxisID: "y0",
      fill: false,
      tension: 0,
      spanGaps: true,
    },
    {
      label: "data1",
      data: [],
      borderColor: "rgba(54,162,235,1)",
      backgroundColor: "rgba(54,162,235,0.2)",
      yAxisID: "y1",
      fill: false,
      tension: 0,
      spanGaps: true,
    },
    {
      label: "data2",
      data: [],
      borderColor: "rgba(75,192,192,1)",
      backgroundColor: "rgba(75,192,192,0.2)",
      yAxisID: "y2",
      fill: false,
      tension: 0,
      spanGaps: true,
    },
  ],
}

const config: ChartConfiguration = {
  type: "line",
  data: data,
  options: {
    animation: false,
    responsive: true,
    scales: {
      x: {
        type: "linear",
        title: { display: true, text: "x" },
        bounds: "data",
      },
      y0: {
        type: "linear",
        position: "left",
        title: { display: true, text: "y0" },
        beginAtZero: false,
      },
      y1: {
        type: "linear",
        position: "left",
        title: { display: true, text: "y1" },
        beginAtZero: false,
        grid: {
          drawOnChartArea: false,
        },
      },
      y2: {
        type: "linear",
        position: "left",
        offset: true,
        title: { display: true, text: "y2" },
        beginAtZero: false,
        grid: {
          drawOnChartArea: false,
        },
      },
    },
    plugins: {
      legend: {
        display: true,
      },
    },
  },
}

const chart = new Chart(ctx!, config)

type Item = {
  usec: number
  temp?: number
  pres?: number
  hum?: number
  x?: number
  y?: number
  z?: number
}

async function fetchDataAndUpdateChart() {
  if (stop) return

  try {
    const res = await fetch(`/${topic}`)
    const json = await res.json()
    const data = json.data.map((entry) => JSON.parse(entry.payload)) as Item[]

    if (!Array.isArray(json.data)) return

    if (topic === "temp") {
      chart.data.datasets[0].label = "temperature (C)"
      chart.data.datasets[1].label = "pressure (hPa)"
      chart.data.datasets[2].label = "humidity (%)"
      chart.options.scales.y0!.title!.text = "temperature (C)"
      chart.options.scales.y1!.title!.text = "pressure (hPa)"
      chart.options.scales.y2!.title!.text = "humidity (%)"

      chart.data.datasets[0].data = data.map((entry) => ({
        x: entry.usec,
        y: entry.temp!,
      }))
      chart.data.datasets[1].data = data.map((entry) => ({
        x: entry.usec,
        y: entry.pres!,
      }))
      chart.data.datasets[2].data = data.map((entry) => ({
        x: entry.usec,
        y: entry.hum!,
      }))
    } else if (topic === "acc") {
      chart.data.datasets[0].label = "x"
      chart.data.datasets[1].label = "y"
      chart.data.datasets[2].label = "z"
      chart.options.scales.y0!.title!.text = "x"
      chart.options.scales.y1!.title!.text = "y"
      chart.options.scales.y2!.title!.text = "z"

      chart.data.datasets[0].data = data.map((entry) => ({
        x: entry.usec,
        y: entry.x!,
      }))
      chart.data.datasets[1].data = data.map((entry) => ({
        x: entry.usec,
        y: entry.y!,
      }))
      chart.data.datasets[2].data = data.map((entry) => ({
        x: entry.usec,
        y: entry.z!,
      }))
    } else {
      console.error(`invalid topic: ${topic}`)
    }

    chart.update()

    const now = new Date()
    const formatted = now.toLocaleTimeString()
    if (el_time) el_time.textContent = `last update: ${formatted}`
  } catch (err) {
    console.error("error :", err)
  }
}

setInterval(fetchDataAndUpdateChart, 1000)
