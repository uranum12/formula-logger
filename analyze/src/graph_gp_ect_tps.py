import pandas as pd
import matplotlib.pyplot as plt


def fig1() -> None:
    df = pd.read_csv("out_ecu/20250830_012451.csv")

    # df = df[(df["logger_us"] >= 1.2e8) & (df["logger_us"] <= 2.4e8)]

    df["logger_us"] = df["logger_us"] / 1_000_000.0

    print(df)

    fig = plt.figure(figsize=(8, 5))
    ax1 = fig.add_subplot(111)

    ax1.scatter(df["logger_us"], df["gear"], label="GP")

    ax2 = ax1.twinx()
    ax2.plot(df["logger_us"], df["tps"], label="TPS", color="C1")
    ax2.plot(df["logger_us"], df["iap"], label="IAP", color="C2")

    ax1.set_xlabel("logger [s]")
    ax1.set_ylabel("GP")
    ax2.set_ylabel("TPS and IAP [V]")

    print("saving fig1...")
    fig.savefig("fig1.jpg", dpi=300)


if __name__ == "__main__":
    fig1()
