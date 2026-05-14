#!/usr/bin/env python3
"""Diagramm der letzten 3 Tage: Ablufttemperatur, Temperaturdifferenz, Heiz-WP-Status.

Verwendung:
  python3 heizung_wp_chart.py
"""

import os
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

SCRIPTE_DIR = os.path.dirname(os.path.abspath(__file__))
CSV = os.path.join(SCRIPTE_DIR, "..", "solar.csv")
OUTPUT_DIR = "."
WP_HEIZUNG_MAX_TEMP = 9.5  # Proxy: WP gilt als an, wenn Abluft <= diesem Wert (spiegelt config.h:heizung_max_ablufttemperatur_wenn_aktiv)


def lade_letzte_3_tage() -> pd.DataFrame:
    df = pd.read_csv(CSV, index_col=False, low_memory=False)
    df = df[df["zeitpunkt"] != 0].copy()
    df["ts"] = pd.to_datetime(df["zeitpunkt"], unit="s")

    letzter_ts = df["ts"].max()
    vor_3_tagen = letzter_ts - pd.Timedelta(days=3)
    df = df[df["ts"] >= vor_3_tagen].sort_values("ts").copy()

    df["waermepumpen_abluft_temperatur"] = pd.to_numeric(
        df["waermepumpen_abluft_temperatur"], errors="coerce"
    ).replace(0, float("nan"))

    df["heizungs_temperatur_differenz_in_grad"] = pd.to_numeric(
        df["heizungs_temperatur_differenz_in_grad"], errors="coerce"
    )

    df["heizung_relay_ist_an"] = pd.to_numeric(
        df["heizung_relay_ist_an"], errors="coerce"
    ).fillna(0)

    df["wp_laeuft_proxy"] = (
        df["waermepumpen_abluft_temperatur"].notna()
        & (df["waermepumpen_abluft_temperatur"] <= WP_HEIZUNG_MAX_TEMP)
    ).astype(int)

    return df


def erstelle_chart(df: pd.DataFrame):
    letzter_ts = df["ts"].max()
    titel_datum = letzter_ts.strftime("%Y-%m-%d")

    fig, ax1 = plt.subplots(figsize=(16, 6))

    ax1.set_xlabel("Datum/Uhrzeit")
    ax1.set_ylabel("Temperatur [°C]")

    line_abluft, = ax1.plot(
        df["ts"], df["waermepumpen_abluft_temperatur"],
        color="tab:orange", linewidth=1.5, label="Abluft-Temp [°C]"
    )
    line_diff, = ax1.plot(
        df["ts"], df["heizungs_temperatur_differenz_in_grad"],
        color="tab:blue", linewidth=1.5, label="Temp-Differenz [°C]"
    )

    ax2 = ax1.twinx()
    ax2.set_ylabel("Heiz-WP", color="tab:red")
    flaeche_wp = ax2.fill_between(
        df["ts"], 0, df["wp_laeuft_proxy"],
        color="tab:green", alpha=0.25, step="post", label="WP läuft (Proxy)"
    )
    line_relay, = ax2.step(
        df["ts"], df["heizung_relay_ist_an"],
        color="tab:red", linewidth=2, label="Überlad-Relay", where="post"
    )
    ax2.set_ylim(-0.1, 1.5)
    ax2.set_yticks([0, 1])
    ax2.set_yticklabels(["Aus", "An"])
    ax2.tick_params(axis="y", labelcolor="tab:red")

    ax1.xaxis.set_major_formatter(mdates.DateFormatter("%d.%m %H:%M"))
    ax1.xaxis.set_major_locator(mdates.HourLocator(interval=6))
    plt.setp(ax1.xaxis.get_majorticklabels(), rotation=45, ha="right")

    handles = [line_abluft, line_diff, flaeche_wp, line_relay]
    ax1.legend(handles, [h.get_label() for h in handles], loc="upper left")

    plt.title(f"Heizungs-WP: letzte 3 Tage (bis {titel_datum})")
    plt.tight_layout()

    out = os.path.join(OUTPUT_DIR, f"heizung_wp_3tage_{titel_datum}.png")
    plt.savefig(out, dpi=120)
    plt.close()
    print(f"Gespeichert: {out}")


def main():
    df = lade_letzte_3_tage()
    erstelle_chart(df)


if __name__ == "__main__":
    main()
