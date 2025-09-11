import { createForm } from "@modular-forms/solid"
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
import {
  createEffect,
  createResource,
  createSignal,
  For,
  onCleanup,
  onMount,
} from "solid-js"
import * as v from "valibot"
import { type DataItem, fetchLatestData } from "@/api/latest"
import { topics } from "@/constants/topics"
import { button } from "@/style/button.css"
import { input } from "@/style/input.css"
import { main, section } from "@/style/section.css"
import type { Topic } from "@/types/topic"

Chart.register(
  LineController,
  LineElement,
  PointElement,
  LinearScale,
  Title,
  Legend,
  Tooltip,
)

const FormSchema = v.object({
  topic0: v.string(),
  topic1: v.string(),
  topic2: v.string(),
  limit: v.number(),
})

type FormType = v.InferInput<typeof FormSchema>

function App() {
  let canvasRef: HTMLCanvasElement
  let chart: Chart
  let timer: number

  const [, { Form, Field }] = createForm<FormType>()

  const [limit, setLimit] = createSignal<number>(100)
  const [topic0, setTopic0] = createSignal<Topic>()
  const [topic1, setTopic1] = createSignal<Topic>()
  const [topic2, setTopic2] = createSignal<Topic>()

  const [res, { refetch }] = createResource(
    () => ({
      fields: [topic0()?.value, topic1()?.value, topic2()?.value].filter(
        Boolean,
      ) as string[],
      limit: limit() ?? 100,
    }),
    ({ fields, limit }) => fetchLatestData(fields, limit),
  )

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
            title: { display: true, text: "usec" },
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

    timer = setInterval(refetch, 1000)
  })

  onCleanup(() => clearInterval(timer))

  createEffect(() => {
    if (!chart) return
    const json = res() ?? {}

    const d0 = json[topic0()?.value ?? ""] ?? []
    const d1 = json[topic1()?.value ?? ""] ?? []
    const d2 = json[topic2()?.value ?? ""] ?? []

    chart.data.datasets[0].data = d0.map((e: DataItem) => ({
      x: e.usec,
      y: e.value,
    }))
    chart.data.datasets[1].data = d1.map((e: DataItem) => ({
      x: e.usec,
      y: e.value,
    }))
    chart.data.datasets[2].data = d2.map((e: DataItem) => ({
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

    const y0Label = topic0()?.label ?? "y0"
    const y1Label = topic1()?.label ?? "y1"
    const y2Label = topic2()?.label ?? "y2"

    chart.data.datasets[0].label = y0Label
    scales.y0.title.text = y0Label
    chart.data.datasets[1].label = y1Label
    scales.y1.title.text = y1Label
    chart.data.datasets[2].label = y2Label
    scales.y2.title.text = y2Label
  })

  return (
    <main class={main}>
      <canvas
        ref={(el) => {
          canvasRef = el
        }}
      ></canvas>
      <section class={section}>
        <Form
          onSubmit={(values) => {
            setTopic0(topics.find((t) => values.topic0 === t.value))
            setTopic1(topics.find((t) => values.topic1 === t.value))
            setTopic2(topics.find((t) => values.topic2 === t.value))
            setLimit(values.limit)
          }}
        >
          <Field name="topic0" type="string">
            {(field, props) => (
              <select {...props} class={input} value={field.value ?? ""}>
                <option value="">選択なし</option>
                <For each={topics}>
                  {(item) => <option value={item.value}>{item.name}</option>}
                </For>
              </select>
            )}
          </Field>
          <Field name="topic1" type="string">
            {(field, props) => (
              <select {...props} class={input} value={field.value ?? ""}>
                <option value="">選択なし</option>
                <For each={topics}>
                  {(item) => <option value={item.value}>{item.name}</option>}
                </For>
              </select>
            )}
          </Field>
          <Field name="topic2" type="string">
            {(field, props) => (
              <select {...props} class={input} value={field.value ?? ""}>
                <option value="">選択なし</option>
                <For each={topics}>
                  {(item) => <option value={item.value}>{item.name}</option>}
                </For>
              </select>
            )}
          </Field>
          <Field name="limit" type="number">
            {(field, props) => (
              <input
                {...props}
                class={input}
                type="number"
                value={field.value ?? 100}
              />
            )}
          </Field>
          <button type="submit" class={button}>
            Submit
          </button>
        </Form>
      </section>
    </main>
  )
}

export default App
