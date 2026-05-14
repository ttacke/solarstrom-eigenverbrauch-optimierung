#!/usr/bin/env python3
"""
Simuliert einen virtuellen Zusatzspeicher (+2.65 kWh) und berechnet,
wie viel Netzstrom damit monatlich vermieden werden könnte.
"""

import pandas as pd
import numpy as np
from pathlib import Path

CSV_PATH = Path(__file__).parent.parent / "solar.csv"

VIRTUAL_CAPACITY_KWH = 2.65
CHARGE_EFFICIENCY = 0.9
DISCHARGE_EFFICIENCY = 0.9
STROMPREIS = 0.33  # EUR/kWh


def main():
    df = pd.read_csv(CSV_PATH, index_col=False, low_memory=False)
    start_ts = int(pd.Timestamp('2026-01-01', tz='UTC').timestamp())
    df['zeitpunkt'] = pd.to_numeric(df['zeitpunkt'], errors='coerce')
    df = df[(df['zeitpunkt'] > 0) & (df['zeitpunkt'] >= start_ts)].copy()

    df['monat'] = (
        pd.to_datetime(df['zeitpunkt'], unit='s', utc=True)
        .dt.tz_convert('Europe/Berlin')
        .dt.tz_localize(None)
        .dt.to_period('M')
        .astype(str)
    )

    monate = df['monat'].values
    netzbezug = df['netzbezug_in_w'].values.astype(float)

    virtual_battery = 0.0
    monthly_grid = {}
    monthly_saved = {}

    for i in range(len(netzbezug)):
        monat = monate[i]
        nw = netzbezug[i]

        if monat not in monthly_grid:
            monthly_grid[monat] = 0.0
            monthly_saved[monat] = 0.0

        energy_kwh = abs(nw) / 1000.0 / 60.0  # W -> kWh pro Minute

        if nw < 0:  # Einspeisung: virtuellen Speicher laden
            charge = min(energy_kwh * CHARGE_EFFICIENCY, VIRTUAL_CAPACITY_KWH - virtual_battery)
            virtual_battery += charge

        elif nw > 0:  # Netzbezug: erst virtuellen Speicher nutzen
            monthly_grid[monat] += energy_kwh
            if virtual_battery > 0:
                max_usable = virtual_battery * DISCHARGE_EFFICIENCY
                savings = min(max_usable, energy_kwh)
                virtual_battery -= savings / DISCHARGE_EFFICIENCY
                monthly_saved[monat] += savings

    print(f"Virtueller Zusatzspeicher: {VIRTUAL_CAPACITY_KWH} kWh, Wirkungsgrad: {int(CHARGE_EFFICIENCY*100)}%/{int(DISCHARGE_EFFICIENCY*100)}%, Strompreis: {STROMPREIS:.2f} EUR/kWh\n")
    print(f"{'Monat':<10} {'Netzstrom':>12} {'Kosten':>10} {'Ersparnis':>12} {'Einsparung':>12}")
    print(f"{'':10} {'kWh':>12} {'EUR':>10} {'kWh':>12} {'EUR':>12}")
    print("-" * 60)

    total_grid = 0.0
    total_saved = 0.0
    for monat in sorted(monthly_grid):
        grid = monthly_grid[monat]
        saved = monthly_saved[monat]
        total_grid += grid
        total_saved += saved
        print(
            f"{monat:<10} {grid:>12.1f} {grid * STROMPREIS:>10.2f}"
            f" {saved:>12.1f} {saved * STROMPREIS:>12.2f}"
        )

    print("-" * 60)
    print(
        f"{'Gesamt':<10} {total_grid:>12.1f} {total_grid * STROMPREIS:>10.2f}"
        f" {total_saved:>12.1f} {total_saved * STROMPREIS:>12.2f}"
    )


if __name__ == "__main__":
    main()
