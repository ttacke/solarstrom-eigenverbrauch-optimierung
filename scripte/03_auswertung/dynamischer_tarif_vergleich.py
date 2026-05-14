#!/usr/bin/env python3
"""
Vergleicht Fixpreis-Tarif (0.33 EUR/kWh) mit dynamischem Spot-Tarif.
Preise: energy-charts.info (DE-LU Spotmarkt, EUR/MWh)
Verbrauch: solar.csv (minutenweise Netzbezug)
"""

import requests
import pandas as pd
from pathlib import Path
from datetime import date

CSV_PATH = Path(__file__).parent.parent / "solar.csv"

START_DATE = "2025-01-01"
AUFSCHLAG_EUR_KWH = 0.1994  # [These] 19.94 ct/kWh — nicht verifiziert
FIXPREIS_EUR_KWH = 0.33


def hole_preisdaten():
    end_date = date.today().isoformat()
    url = (
        f"https://api.energy-charts.info/price"
        f"?bzn=DE-LU&start={START_DATE}&end={end_date}"
    )
    print(f"Abruf: {url}")
    r = requests.get(url, headers={"accept": "application/json"}, timeout=30)
    r.raise_for_status()
    data = r.json()

    df = pd.DataFrame({
        "ts": pd.to_datetime(data["unix_seconds"], unit="s", utc=True),
        "eur_mwh": data["price"],
    })
    df["dynamisch_eur_kwh"] = df["eur_mwh"] / 1000 + AUFSCHLAG_EUR_KWH
    df["stunde"] = df["ts"].dt.floor("h").dt.tz_convert("Europe/Berlin")
    return df.groupby("stunde")["dynamisch_eur_kwh"].mean()


def hole_verbrauchsdaten():
    df = pd.read_csv(CSV_PATH, index_col=False, low_memory=False)
    df["zeitpunkt"] = pd.to_numeric(df["zeitpunkt"], errors="coerce")
    start_ts = int(pd.Timestamp(START_DATE, tz="UTC").timestamp())
    df = df[(df["zeitpunkt"] > 0) & (df["zeitpunkt"] >= start_ts)].copy()

    df["stunde"] = (
        pd.to_datetime(df["zeitpunkt"], unit="s", utc=True)
        .dt.floor("h")
        .dt.tz_convert("Europe/Berlin")
    )
    df["netzbezug_in_w"] = pd.to_numeric(df["netzbezug_in_w"], errors="coerce").fillna(0)
    # W pro Minute → kWh: /1000/60; nur positive Werte (Bezug, nicht Einspeisung)
    df["bezug_kwh"] = df["netzbezug_in_w"].clip(lower=0) / 1000 / 60
    return df.groupby("stunde")["bezug_kwh"].sum()


def main():
    print("Lade Preisdaten von energy-charts.info ...")
    preise = hole_preisdaten()
    print("Lade Verbrauchsdaten aus solar.csv ...")
    verbrauch = hole_verbrauchsdaten()

    merged = verbrauch.to_frame().join(preise, how="left")
    merged["monat"] = merged.index.strftime("%Y-%m")
    merged["kosten_fix"] = merged["bezug_kwh"] * FIXPREIS_EUR_KWH
    merged["kosten_dyn"] = merged["bezug_kwh"] * merged["dynamisch_eur_kwh"]

    fehlende_preise = merged["dynamisch_eur_kwh"].isna().sum()
    if fehlende_preise:
        print(f"Hinweis: {fehlende_preise} Stunden ohne Preisdaten (werden ignoriert)")

    monthly = merged.groupby("monat").agg(
        bezug_kwh=("bezug_kwh", "sum"),
        kosten_fix=("kosten_fix", "sum"),
        kosten_dyn=("kosten_dyn", "sum"),
    )

    print(f"\nAufschlag: {AUFSCHLAG_EUR_KWH*100:.2f} ct/kWh [These — nicht verifiziert]\n")
    print(f"{'Monat':<10} {'Netzstrom':>12} {'Fix 0.33€':>11} {'Dynamisch':>11} {'Diff':>9}")
    print(f"{'':10} {'kWh':>12} {'EUR':>11} {'EUR':>11} {'EUR':>9}")
    print("-" * 57)

    total_kwh = total_fix = total_dyn = 0.0
    for monat, row in monthly.iterrows():
        diff = row["kosten_dyn"] - row["kosten_fix"]
        total_kwh += row["bezug_kwh"]
        total_fix += row["kosten_fix"]
        total_dyn += row["kosten_dyn"]
        print(
            f"{monat:<10} {row['bezug_kwh']:>12.1f} {row['kosten_fix']:>11.2f}"
            f" {row['kosten_dyn']:>11.2f} {diff:>+9.2f}"
        )

    print("-" * 57)
    diff_total = total_dyn - total_fix
    print(
        f"{'Gesamt':<10} {total_kwh:>12.1f} {total_fix:>11.2f}"
        f" {total_dyn:>11.2f} {diff_total:>+9.2f}"
    )


if __name__ == "__main__":
    main()
