import json

import requests
import matplotlib.pyplot as plt
import polars as pl


def get_temp():
    res = requests.get("http://192.168.11.10:1234/temp?limit=100")
    df = pl.from_dicts(
        [json.loads(payload["payload"]) for payload in res.json()["data"]]
    )
    return df.with_columns(pl.col("temp") * 3.3 / 1024 * 100)


def main():
    fig = plt.figure(figsize=(8, 5))
    ax = fig.add_subplot(111)

    df = get_temp()

    (lines,) = ax.plot(df["usec"], df["temp"])

    while True:
        df = get_temp()

        lines.set_data(df["usec"], df["temp"])
        x_min = df.select("usec").min().item()
        x_max = df.select("usec").max().item()
        y_min = df.select("temp").min().item() - 1
        y_max = df.select("temp").max().item() + 1
        ax.set_xlim((x_min, x_max))
        ax.set_ylim((y_min, y_max))

        plt.pause(1.0)


if __name__ == "__main__":
    main()
