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
import { createEffect, createSignal, For, onCleanup, onMount } from "solid-js"
import ky from "ky"

import { input } from "./style/input.css"
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

type Topic = {
  name: string
  value: string
  label: string
}

type DataItem = {
  usec: number
  value: number
}

const topics: Topic[] = [
  {
    name: "ストロークセンサ左前",
    value: "stroke/front/left",
    label: "front/left (V)",
  },
  {
    name: "ストロークセンサ右前",
    value: "stroke/front/right",
    label: "front/right (V)",
  },
  { name: "水温センサ 流入口", value: "water/inlet_temp", label: "inlet (C)" },
  {
    name: "水温センサ 流出口",
    value: "water/outlet_temp",
    label: "outlet (C)",
  },
]

function App() {
  let canvasRef: HTMLCanvasElement
  let chart: Chart
  let timer: number

  const [topic, setTopic] = createSignal("")
  const [data, setData] = createSignal<DataItem[]>([])

  const fetchData = async () => {
    try {
      const json = await ky
        .get(`/latest/100`, {
          searchParams: {
            fields: topic(),
          },
          timeout: 1000,
          retry: {
            limit: 0,
          },
        })
        .json()
      setData(json[topic()])
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

    chart.data.datasets[0].data = data().map((e) => ({
      x: e.usec,
      y: e.value,
    }))

    chart.update()
  })

  createEffect(() => {
    if (!chart) return

    const scales = chart.options.scales as Record<
      "y0" | "y1" | "y2",
      LinearScaleOptions
    >

    const y0Label: string = topics.find((e) => e.value === topic())?.label!

    chart.data.datasets[0].label = y0Label
    scales.y0.title.text = y0Label
  })

  return (
    <main class={main}>
      <canvas
        ref={(el) => {
          canvasRef = el
        }}
      ></canvas>
      <section class={section}>
        <select
          class={input}
          onChange={(e) => {
            setTopic(e.target.value)
          }}
        >
          <For each={topics}>
            {(item) => <option value={item.value}>{item.name}</option>}
          </For>
        </select>
      </section>
    </main>
  )
}

export default App
