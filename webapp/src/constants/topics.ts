import type { Topic } from "@/types/topic"

export const topics: Topic[] = [
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
