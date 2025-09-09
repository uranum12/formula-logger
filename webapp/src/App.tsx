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
import ky from "ky"
import {
  batch,
  createEffect,
  createSignal,
  For,
  onCleanup,
  onMount,
} from "solid-js"
import * as v from "valibot"
import { button } from "./style/button.css"
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

type ResType = {
  [key: string]: DataItem[]
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
  {
    name: "ストロークセンサ左後",
    value: "stroke/rear/left",
    label: "rear/left (V)",
  },
  {
    name: "ストロークセンサ右後",
    value: "stroke/rear/right",
    label: "rear/right (V)",
  },
  { name: "水温センサ 流入口", value: "water/inlet_temp", label: "inlet (C)" },
  {
    name: "水温センサ 流出口",
    value: "water/outlet_temp",
    label: "outlet (C)",
  },
  {
    name: "ECU ECT",
    value: "ecu/ect",
    label: "ECT (V)",
  },
  {
    name: "ECU TPS",
    value: "ecu/tps",
    label: "TPS (V)",
  },
  {
    name: "ECU IAP",
    value: "ecu/iap",
    label: "IAP (V)",
  },
  {
    name: "ECU GP",
    value: "ecu/gp",
    label: "GP",
  },
  {
    name: "RPM",
    value: "rpm/rpm",
    label: "RPM",
  },
  {
    name: "Rpm",
    value: "rpm/god",
    label: "rpm",
  },
  {
    name: "IMU 加速度 X",
    value: "acc/accel_x",
    label: "Accel X (m/s^2)",
  },
  {
    name: "IMU 加速度 Y",
    value: "acc/accel_y",
    label: "Accel Y (m/s^2)",
  },
  {
    name: "IMU 加速度 Z",
    value: "acc/accel_z",
    label: "Accel Z (m/s^2)",
  },
  {
    name: "IMU ジャイロ X",
    value: "acc/gyro_x",
    label: "Gyro X (dps)",
  },
  {
    name: "IMU ジャイロ Y",
    value: "acc/gyro_y",
    label: "Gyro Y (dps)",
  },
  {
    name: "IMU ジャイロ Z",
    value: "acc/gyro_z",
    label: "Gyro Z (dps)",
  },
  {
    name: "IMU 磁力 X",
    value: "acc/mag_x",
    label: "Mag X (uT)",
  },
  {
    name: "IMU 磁力 Y",
    value: "acc/mag_y",
    label: "Mag Y (uT)",
  },
  {
    name: "IMU 磁力 Z",
    value: "acc/mag_z",
    label: "Mag Z (uT)",
  },
  {
    name: "IMU オイラー角 ヘッディング",
    value: "acc/euler_heading",
    label: "Euler Heading (deg)",
  },
  {
    name: "IMU オイラー角 ロール",
    value: "acc/euler_roll",
    label: "Euler Roll (deg)",
  },
  {
    name: "IMU オイラー角 ピッチ",
    value: "acc/euler_pitch",
    label: "Euler Pitch (deg)",
  },
  {
    name: "IMU クォータニオン W",
    value: "acc/quaternion_w",
    label: "Quaternion W",
  },
  {
    name: "IMU クォータニオン X",
    value: "acc/quaternion_x",
    label: "Quaternion X",
  },
  {
    name: "IMU クォータニオン Y",
    value: "acc/quaternion_y",
    label: "Quaternion Y",
  },
  {
    name: "IMU クォータニオン Z",
    value: "acc/quaternion_z",
    label: "Quaternion Z",
  },
  {
    name: "IMU 線形加速度 X",
    value: "acc/linear_accel_x",
    label: "Linear Accel X (m/s^2)",
  },
  {
    name: "IMU 線形加速度 Y",
    value: "acc/linear_accel_y",
    label: "Linear Accel Y (m/s^2)",
  },
  {
    name: "IMU 線形加速度 Z",
    value: "acc/linear_accel_z",
    label: "Linear Accel Z (m/s^2)",
  },
  {
    name: "IMU 重力 X",
    value: "acc/gravity_x",
    label: "Gravity (m/s^2)",
  },
  {
    name: "IMU 重力 Y",
    value: "acc/gravity_y",
    label: "Gravity Y (m/s^2)",
  },
  {
    name: "IMU 重力 Z",
    value: "acc/gravity_z",
    label: "Gravity Z (m/s^2)",
  },
  {
    name: "IMU 校正状態 システム",
    value: "acc/status_sys",
    label: "Status SYS",
  },
  {
    name: "IMU 校正状態 ジャイロ",
    value: "acc/status_gyro",
    label: "Status Gyro",
  },
  {
    name: "IMU 校正状態 加速度",
    value: "acc/status_accel",
    label: "Status Accel",
  },
  {
    name: "IMU 校正状態 磁力",
    value: "acc/status_mag",
    label: "Status Mag",
  },
]

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

  const [limit, setLimit] = createSignal<number>()
  const [topic0, setTopic0] = createSignal<Topic>()
  const [topic1, setTopic1] = createSignal<Topic>()
  const [topic2, setTopic2] = createSignal<Topic>()
  const [data0, setData0] = createSignal<DataItem[]>([])
  const [data1, setData1] = createSignal<DataItem[]>([])
  const [data2, setData2] = createSignal<DataItem[]>([])

  const fetchData = async () => {
    const fields = [topic0()?.value, topic1()?.value, topic2()?.value].filter(
      (t) => typeof t === "string" && t !== "",
    )

    if (fields.length === 0) {
      return
    }

    try {
      const json = await ky
        .get<ResType>(`/latest/${limit() ?? 100}`, {
          searchParams: { fields: fields.join(",") },
          timeout: 1000,
          retry: {
            limit: 0,
          },
        })
        .json()
      batch(() => {
        setData0(json[topic0()?.value ?? ""] ?? [])
        setData1(json[topic1()?.value ?? ""] ?? [])
        setData2(json[topic2()?.value ?? ""] ?? [])
      })
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

    timer = setInterval(fetchData, 1000)
  })

  onCleanup(() => {
    clearInterval(timer)
  })

  createEffect(() => {
    if (!chart) return

    const d0 = data0()
    const d1 = data1()
    const d2 = data2()

    if (Array.isArray(d0) && d0.length !== 0) {
      chart.data.datasets[0].data = d0.map((e) => ({
        x: e.usec,
        y: e.value,
      }))
    } else {
      chart.data.datasets[0].data = []
    }
    if (Array.isArray(d1) && d1.length !== 0) {
      chart.data.datasets[1].data = d1.map((e) => ({
        x: e.usec,
        y: e.value,
      }))
    } else {
      chart.data.datasets[1].data = []
    }
    if (Array.isArray(d2) && d2.length !== 0) {
      chart.data.datasets[2].data = d2.map((e) => ({
        x: e.usec,
        y: e.value,
      }))
    } else {
      chart.data.datasets[2].data = []
    }

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
