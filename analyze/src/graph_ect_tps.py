import pandas as pd
import matplotlib.pyplot as plt


def fig1() -> None:
    df = pd.read_csv("out_ecu/20250830_012451.csv")

    df["logger_us"] = df["logger_us"] / 1_000_000.0

    print(df)

    fig = plt.figure(figsize=(8, 5))
    ax1 = fig.add_subplot(111)

    ax1.plot(df["logger_us"], df["ect"], label="ECT", color="C0")

    ax2 = ax1.twinx()
    ax2.plot(df["logger_us"], df["tps"], label="TPS", color="C1")

    ax1.set_xlabel("logger [s]")
    ax1.set_ylabel("ECT [C]")
    ax2.set_ylabel("TPS [V]")

    ax1.set_ylim(60, 80)
    # ax1.set_xlim(1500, 1700)

    ax1.grid()

    print("saving fig2...")
    fig.savefig("fig2.jpg", dpi=300)


if __name__ == "__main__":
    fig1()
