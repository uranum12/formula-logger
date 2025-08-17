import type { ChartConfiguration, LinearScaleOptions } from "chart.js"
import {
  Chart,
  Legend,
  LinearScale,
  LineController,
  LineElement,
  PointElement,
  Title,
  Tooltip,
} from "chart.js"
import { batch, createEffect, createSignal, onCleanup, onMount } from "solid-js"

import { checkbox, checkboxLabel, input } from "./style/input.css"
import { main, section } from "./style/section.css"

Chart.register(
  LineController,
  LineElement,
  PointElement,
  LinearScale,
  Title,
  Legend,
  Tooltip,
)

type ChartDataItem = {
  x: number
  y: number
}

type ChartData = {
  data0: ChartDataItem[]
  data1: ChartDataItem[]
  data2: ChartDataItem[]
}

type ChartLimit = {
  data0?: {
    min: number
    max: number
  }
  data1?: {
    min: number
    max: number
  }
  data2?: {
    min: number
    max: number
  }
}

type ResItem = {
  usec: number
  temp: number
  pres: number
  hum: number
  x: number
  y: number
  z: number
  in: number
  out: number
}

function App() {
  let canvasRef: HTMLCanvasElement
  let chart: Chart
  let timer: number

  const [data, setData] = createSignal<ChartData>({
    data0: [],
    data1: [],
    data2: [],
  })
  const [chartLimit, setChartLimit] = createSignal<ChartLimit>({})

  const [lastUpdate, setLastUpdate] = createSignal("")
  const [topic, setTopic] = createSignal("temp")
  const [limit, setLimit] = createSignal(100)
  const [stop, setStop] = createSignal(false)
  const [align, setAlign] = createSignal(false)

  const fetchData = async () => {
    if (stop()) return

    try {
      const res = await fetch(`/${topic()}?limit=${limit()}`, {
        signal: AbortSignal.timeout(1000),
      })
      const json = await res.json()
      const dataRes = json.data.map((e: { payload: string }) =>
        JSON.parse(e.payload),
      ) as ResItem[]

      if (topic() === "temp") {
        const data0 = dataRes.map((entry) => ({
          x: entry.usec,
          y: entry.temp,
        }))
        const data1 = dataRes.map((entry) => ({
          x: entry.usec,
          y: entry.pres,
        }))
        const data2 = dataRes.map((entry) => ({
          x: entry.usec,
          y: entry.hum,
        }))

        const y0 = data0.map((e) => e.y).filter((e) => Number.isFinite(e))
        const y1 = data1.map((e) => e.y).filter((e) => Number.isFinite(e))
        const y2 = data2.map((e) => e.y).filter((e) => Number.isFinite(e))

        const y0_min = Math.min(...y0)
        const y0_max = Math.max(...y0)
        const y0_diff = y0_max - y0_min

        const y1_min = Math.min(...y1)
        const y1_max = Math.max(...y1)
        const y1_diff = y1_max - y1_min

        const y2_min = Math.min(...y2)
        const y2_max = Math.max(...y2)
        const y2_diff = y2_max - y2_min

        batch(() => {
          setData({ data0, data1, data2 })
          setChartLimit({
            data0: { min: y0_min - y0_diff * 0.1, max: y0_max + y0_diff * 0.1 },
            data1: { min: y1_min - y1_diff * 0.1, max: y1_max + y1_diff * 0.1 },
            data2: { min: y2_min - y2_diff * 0.1, max: y2_max + y2_diff * 0.1 },
          })
        })
      } else if (topic() === "acc") {
        const data0 = dataRes.map((entry) => ({
          x: entry.usec,
          y: entry.x,
        }))
        const data1 = dataRes.map((entry) => ({
          x: entry.usec,
          y: entry.y,
        }))
        const data2 = dataRes.map((entry) => ({
          x: entry.usec,
          y: entry.z,
        }))

        if (align()) {
          const y0 = data0.map((e) => e.y).filter((e) => Number.isFinite(e))
          const y1 = data1.map((e) => e.y).filter((e) => Number.isFinite(e))
          const y2 = data2.map((e) => e.y).filter((e) => Number.isFinite(e))
          const min = Math.min(...y0, ...y1, ...y2)
          const max = Math.max(...y0, ...y1, ...y2)

          const diff = max - min
          const margin = diff * 0.1

          batch(() => {
            setData({ data0, data1, data2 })
            setChartLimit({
              data0: { min: min - margin, max: max + margin },
              data1: { min: min - margin, max: max + margin },
              data2: { min: min - margin, max: max + margin },
            })
          })
        } else {
          const y0 = data0.map((e) => e.y).filter((e) => Number.isFinite(e))
          const y1 = data1.map((e) => e.y).filter((e) => Number.isFinite(e))
          const y2 = data2.map((e) => e.y).filter((e) => Number.isFinite(e))

          const y0_min = Math.min(...y0)
          const y0_max = Math.max(...y0)
          const y0_diff = y0_max - y0_min

          const y1_min = Math.min(...y1)
          const y1_max = Math.max(...y1)
          const y1_diff = y1_max - y1_min

          const y2_min = Math.min(...y2)
          const y2_max = Math.max(...y2)
          const y2_diff = y2_max - y2_min

          batch(() => {
            setData({ data0, data1, data2 })
            setChartLimit({
              data0: {
                min: y0_min - y0_diff * 0.1,
                max: y0_max + y0_diff * 0.1,
              },
              data1: {
                min: y1_min - y1_diff * 0.1,
                max: y1_max + y1_diff * 0.1,
              },
              data2: {
                min: y2_min - y2_diff * 0.1,
                max: y2_max + y2_diff * 0.1,
              },
            })
          })
        }
      } else if (topic() === "water") {
        const data0 = dataRes.map((entry) => ({
          x: entry.usec,
          y: entry.in,
        }))
        const data1 = dataRes.map((entry) => ({
          x: entry.usec,
          y: entry.out,
        }))
        if (align()) {
          const y0 = data0.map((e) => e.y).filter((e) => Number.isFinite(e))
          const y1 = data1.map((e) => e.y).filter((e) => Number.isFinite(e))
          const min = Math.min(...y0, ...y1)
          const max = Math.max(...y0, ...y1)

          const diff = max - min
          const margin = diff * 0.1

          batch(() => {
            setData({ data0, data1, data2: [] })
            setChartLimit({
              data0: { min: min - margin, max: max + margin },
              data1: { min: min - margin, max: max + margin },
              data2: undefined,
            })
          })
        } else {
          const y0 = data0.map((e) => e.y).filter((e) => Number.isFinite(e))
          const y1 = data1.map((e) => e.y).filter((e) => Number.isFinite(e))

          const y0_min = Math.min(...y0)
          const y0_max = Math.max(...y0)
          const y0_diff = y0_max - y0_min

          const y1_min = Math.min(...y1)
          const y1_max = Math.max(...y1)
          const y1_diff = y1_max - y1_min

          batch(() => {
            setData({ data0, data1, data2: [] })
            setChartLimit({
              data0: {
                min: y0_min - y0_diff * 0.1,
                max: y0_max + y0_diff * 0.1,
              },
              data1: {
                min: y1_min - y1_diff * 0.1,
                max: y1_max + y1_diff * 0.1,
              },
              data2: undefined,
            })
          })
        }
      }

      const now = new Date()
      setLastUpdate(now.toLocaleTimeString())
    } catch (err) {
      console.error("error :", err)
    }
  }

  onMount(() => {
    const config: ChartConfiguration = {
      type: "line",
      data: {
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
      },
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

    const ctx = canvasRef.getContext("2d")
    if (!ctx) return
    chart = new Chart(ctx, config)

    timer = setInterval(fetchData, 1000)
  })

  onCleanup(() => {
    clearInterval(timer)
  })

  createEffect(() => {
    if (!chart) return

    const chartdata = data()

    chart.data.datasets[0].data = chartdata.data0
    chart.data.datasets[1].data = chartdata.data1
    chart.data.datasets[2].data = chartdata.data2

    const scales = chart.options.scales as Record<
      "y0" | "y1" | "y2",
      LinearScaleOptions
    >
    const limits = chartLimit()
    if (limits.data0) {
      scales.y0.min = limits.data0.min
      scales.y0.max = limits.data0.max
    }
    if (limits.data1) {
      scales.y1.min = limits.data1.min
      scales.y1.max = limits.data1.max
    }
    if (limits.data2) {
      scales.y2.min = limits.data2.min
      scales.y2.max = limits.data2.max
    } else {
      scales.y2.min = 0
      scales.y2.max = 1
    }

    chart.update()
  })

  createEffect(() => {
    if (!chart) return

    const scales = chart.options.scales as Record<
      "y0" | "y1" | "y2",
      LinearScaleOptions
    >
    if (topic() === "temp") {
      chart.data.datasets[0].label = "temperature (C)"
      chart.data.datasets[1].label = "pressure (hPa)"
      chart.data.datasets[2].label = "humidity (%)"
      scales.y0.title.text = "temperature"
      scales.y1.title.text = "pressure"
      scales.y2.title.text = "humidity"
    } else if (topic() === "acc") {
      chart.data.datasets[0].label = "x (g)"
      chart.data.datasets[1].label = "y (g)"
      chart.data.datasets[2].label = "z (g)"
      scales.y0.title.text = "x"
      scales.y1.title.text = "y"
      scales.y2.title.text = "z"
    } else if (topic() === "water") {
      chart.data.datasets[0].label = "in (K)"
      chart.data.datasets[1].label = "out (K)"
      chart.data.datasets[2].label = "null"
      scales.y0.title.text = "in"
      scales.y1.title.text = "out"
      scales.y2.title.text = "null"
    }

    chart.update()
  })

  return (
    <main class={main}>
      <canvas
        ref={(el) => {
          canvasRef = el
        }}
      ></canvas>
      <section class={section}>
        <div>last update: {lastUpdate()}</div>
      </section>
      <section class={section}>
        <select
          class={input}
          onChange={(e) => {
            setTopic(e.target.value)
          }}
        >
          <option value="temp">温湿度計</option>
          <option value="acc">加速度センサ</option>
          <option value="water">冷却液 温度センサ</option>
        </select>
        <input
          class={input}
          onChange={(e) => {
            setLimit(parseInt(e.target.value, 10))
          }}
          type="number"
          step="1"
          value="100"
        />
        <label class={checkboxLabel}>
          <input
            class={checkbox}
            onChange={(e) => {
              setStop(e.target.checked)
            }}
            type="checkbox"
          />
          ストップ
        </label>
        <label class={checkboxLabel}>
          <input
            class={checkbox}
            onChange={(e) => {
              setAlign(e.target.checked)
            }}
            type="checkbox"
          />
          Y軸を揃える
        </label>
      </section>
    </main>
  )
}

export default App
