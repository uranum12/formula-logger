import json
import math
from pathlib import Path

import pandas as pd


def is_json(s: str) -> bool:
    try:
        json.loads(s)
        return True
    except Exception:
        return False


def calc_gear(v: float) -> int:
    v_ref = [0.0, 0.88, 1.10, 1.46, 1.77, 2.09, 2.38, 3.0]
    for i in range(6):
        low = (v_ref[i] + v_ref[i + 1]) / 2.0
        high = (v_ref[i + 1] + v_ref[i + 2]) / 2.0
        if low <= v < high:
            return i + 1
    return 0


def calc_ect(v: float) -> float:
    if math.isclose(v, 0.0):
        return 0.0
    v_i = 5.0 - v
    return 24.21 * v_i - 26.8


def ecu(base_path: Path) -> None:
    out_path = base_path / "ecu"
    out_path.mkdir(exist_ok=True)

    for path in Path("data").glob("*.csv"):
        print(path)

        df = pd.read_csv(path, on_bad_lines="skip", engine="python")

        df = df[df["topic"] == "ecu"]
        df = df[df["payload"].apply(is_json)]

        payload_df = df["payload"].apply(json.loads).apply(pd.Series)

        df = pd.concat([df[["time"]], payload_df], axis=1)

        df = df.assign(
            server_ms=df["time"],
            logger_us=df["sec"] * 1_000_000 + df["usec"],
            ect=df["ect"].apply(calc_ect),
            gear=df["gp"].apply(calc_gear),
        )

        df = df[["server_ms", "logger_us", "ect", "tps", "iap", "gear"]]

        print(df)
        df.to_csv(out_path / path.name, index=False)


def rpm(base_path: Path) -> None:
    out_path = base_path / "rpm"
    out_path.mkdir(exist_ok=True)

    for path in Path("data").glob("*.csv"):
        print(path)

        df = pd.read_csv(path, on_bad_lines="skip", engine="python")

        df = df[df["topic"] == "rpm"]
        df = df[df["payload"].apply(is_json)]

        payload_df = df["payload"].apply(json.loads).apply(pd.Series)

        df = pd.concat([df[["time"]], payload_df], axis=1)

        df = df.assign(
            server_ms=df["time"],
            logger_us=df["sec"] * 1_000_000 + df["usec"],
        )

        df = df[["server_ms", "logger_us", "rpm"]]

        print(df)
        df.to_csv(out_path / path.name, index=False)


def water(base_path: Path) -> None:
    out_path = base_path / "water"
    out_path.mkdir(exist_ok=True)

    for path in Path("data").glob("*.csv"):
        print(path)

        df = pd.read_csv(path, on_bad_lines="skip", engine="python")

        df = df[df["topic"] == "water"]
        df = df[df["payload"].apply(is_json)]

        payload_df = df["payload"].apply(json.loads).apply(pd.Series)

        df = pd.concat([df[["time"]], payload_df], axis=1)

        df = df.assign(
            server_ms=df["time"],
            logger_us=df["sec"] * 1_000_000 + df["usec"],
            inlet_temp=df["inlet_temp"] - 273.15,
            outlet_temp=df["outlet_temp"] - 273.15,
        )

        df = df[["server_ms", "logger_us", "inlet_temp", "outlet_temp"]]

        print(df)
        df.to_csv(out_path / path.name, index=False)


def stroke_front(base_path: Path) -> None:
    out_path = base_path / "stroke_front"
    out_path.mkdir(exist_ok=True)

    for path in Path("data").glob("*.csv"):
        print(path)

        df = pd.read_csv(path, on_bad_lines="skip", engine="python")

        df = df[df["topic"] == "stroke/front"]
        df = df[df["payload"].apply(is_json)]

        payload_df = df["payload"].apply(json.loads).apply(pd.Series)

        df = pd.concat([df[["time"]], payload_df], axis=1)

        df = df.assign(
            server_ms=df["time"],
            logger_us=df["sec"] * 1_000_000 + df["usec"],
        )

        df = df[["server_ms", "logger_us", "left", "right"]]

        print(df)
        df.to_csv(out_path / path.name, index=False)


def stroke_rear(base_path: Path) -> None:
    out_path = base_path / "stroke_rear"
    out_path.mkdir(exist_ok=True)

    for path in Path("data").glob("*.csv"):
        print(path)

        df = pd.read_csv(path, on_bad_lines="skip", engine="python")

        df = df[df["topic"] == "stroke/rear"]
        df = df[df["payload"].apply(is_json)]

        payload_df = df["payload"].apply(json.loads).apply(pd.Series)

        df = pd.concat([df[["time"]], payload_df], axis=1)

        df = df.assign(
            server_ms=df["time"],
            logger_us=df["sec"] * 1_000_000 + df["usec"],
        )

        df = df[["server_ms", "logger_us", "left", "right"]]

        print(df)
        df.to_csv(out_path / path.name, index=False)


def acc(base_path: Path) -> None:
    out_path = base_path / "acc"
    out_path.mkdir(exist_ok=True)

    for path in Path("data").glob("*.csv"):
        print(path)

        df = pd.read_csv(path, on_bad_lines="skip", engine="python")

        df = df[df["topic"] == "acc"]
        df = df[df["payload"].apply(is_json)]

        payload_df = df["payload"].apply(json.loads).apply(pd.Series)

        df = pd.concat([df[["time"]], payload_df], axis=1)

        df = df.assign(
            server_ms=df["time"],
            logger_us=df["sec"] * 1_000_000 + df["usec"],
            accel_x=df["ax"] / 100.0,
            accel_y=df["ay"] / 100.0,
            accel_z=df["az"] / 100.0,
            gyro_x=df["gx"] / 16.0,
            gyro_y=df["gy"] / 16.0,
            gyro_z=df["gz"] / 16.0,
            mag_x=df["mx"] / 16.0,
            mag_y=df["my"] / 16.0,
            mag_z=df["mz"] / 16.0,
            euler_heading=df["h"] / 16.0,
            euler_roll=df["r"] / 16.0,
            euler_pitch=df["p"] / 16.0,
            quaternion_w=df["qw"] / 16384.0,
            quaternion_x=df["qx"] / 16384.0,
            quaternion_y=df["qy"] / 16384.0,
            quaternion_z=df["qz"] / 16384.0,
            linear_accel_x=df["lx"] / 100.0,
            linear_accel_y=df["ly"] / 100.0,
            linear_accel_z=df["lz"] / 100.0,
            gravity_x=df["x"] / 100.0,
            gravity_y=df["y"] / 100.0,
            gravity_z=df["z"] / 100.0,
            status_sys=df["ss"],
            status_gyro=df["sg"],
            status_accel=df["sa"],
            status_mag=df["sm"],
        )

        df = df[
            [
                "server_ms",
                "logger_us",
                "accel_x",
                "accel_y",
                "accel_z",
                "gyro_x",
                "gyro_y",
                "gyro_z",
                "mag_x",
                "mag_y",
                "mag_z",
                "euler_heading",
                "euler_roll",
                "euler_pitch",
                "quaternion_w",
                "quaternion_x",
                "quaternion_y",
                "quaternion_z",
                "linear_accel_x",
                "linear_accel_y",
                "linear_accel_z",
                "gravity_x",
                "gravity_y",
                "gravity_z",
                "status_sys",
                "status_gyro",
                "status_accel",
                "status_mag",
            ]
        ]

        print(df)
        df.to_csv(out_path / path.name, index=False)


def main() -> None:
    out_path = Path("out")
    out_path.mkdir(exist_ok=True)
    ecu(out_path)
    rpm(out_path)
    water(out_path)
    stroke_front(out_path)
    stroke_rear(out_path)
    acc(out_path)


if __name__ == "__main__":
    main()
