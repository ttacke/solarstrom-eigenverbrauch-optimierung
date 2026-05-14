#!/usr/bin/env python3
"""Grafische Tagesansicht aus solar.csv: SoC, Haus-Verbrauch, Solar-Erzeugung.

Verwendung:
  python3 solar_tages_chart.py [DATUM]   # DATUM z.B. 2026-03-16, Default: neuester Tag
"""

import sys
import os
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from datetime import date

SCRIPTE_DIR = os.path.dirname(os.path.abspath(__file__))
CSV = os.path.join(SCRIPTE_DIR, "../solar.csv")
OUTPUT_DIR = SCRIPTE_DIR


def lade_tag(ziel_datum: str) -> pd.DataFrame:
    df = pd.read_csv(CSV, index_col=False, low_memory=False)
    df = df[df["zeitpunkt"] != 0].copy()
    df["ts"] = pd.to_datetime(df["zeitpunkt"], unit="s")
    df["date"] = df["ts"].dt.date.astype(str)

    if ziel_datum not in df["date"].values:
        verfuegbar = sorted(df["date"].unique())
        neuester = verfuegbar[-1]
        print(f"Kein Datum {ziel_datum} in CSV. Neuester verfügbarer Tag: {neuester}")
        ziel_datum = neuester

    tages_df = df[df["date"] == ziel_datum].sort_values("ts").copy()
    return tages_df, ziel_datum


def erstelle_chart(df: pd.DataFrame, datum: str):
    df = df.copy()
    df["soc_pct"]        = df["solarakku_ladestand_in_promille"] / 10
    df["solar_kw"]       = df["solarerzeugung_in_w"] / 1000
    df["verbrauch_kw"]   = df["stromverbrauch_in_w"] / 1000

    fig, ax1 = plt.subplots(figsize=(14, 6))

    ax1.set_xlabel("Uhrzeit")
    ax1.set_ylabel("Leistung [kW]", color="tab:blue")

    line_solar,    = ax1.plot(df["ts"], df["solar_kw"],     color="tab:orange", linewidth=1.5, label="Solar [kW]")
    line_verbrauch, = ax1.plot(df["ts"], df["verbrauch_kw"], color="tab:blue",   linewidth=1.5, label="Haus-Verbrauch [kW]")

    ax1.tick_params(axis="y", labelcolor="tab:blue")
    ax1.set_ylim(bottom=0)

    ax2 = ax1.twinx()
    ax2.set_ylabel("SoC [%]", color="tab:green")
    line_soc, = ax2.plot(df["ts"], df["soc_pct"], color="tab:green", linewidth=2, label="SoC [%]")
    ax2.tick_params(axis="y", labelcolor="tab:green")
    ax2.set_ylim(0, 105)

    ax1.xaxis.set_major_formatter(mdates.DateFormatter("%H:%M"))
    ax1.xaxis.set_major_locator(mdates.HourLocator(interval=1))
    plt.setp(ax1.xaxis.get_majorticklabels(), rotation=45, ha="right")

    lines = [line_solar, line_verbrauch, line_soc]
    labels = [l.get_label() for l in lines]
    ax1.legend(lines, labels, loc="upper left")

    plt.title(f"Solar-Tagesansicht {datum}")
    plt.tight_layout()

    os.makedirs(OUTPUT_DIR, exist_ok=True)
    out = os.path.join(OUTPUT_DIR, f"solar_tages_chart_{datum}.png")
    plt.savefig(out, dpi=120)
    print(f"Gespeichert: {out}")


def main():
    ziel = sys.argv[1] if len(sys.argv) > 1 else str(date.today())
    df, verfuegbares_datum = lade_tag(ziel)
    erstelle_chart(df, verfuegbares_datum)


if __name__ == "__main__":
    main()
