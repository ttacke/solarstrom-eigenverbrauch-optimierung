#!/usr/bin/env python3
"""
Energiebilanz: monatlich und jahresweise

Spalten:
  Solarertrag   = solarerzeugung_in_w  → kWh
  Netzbezug     = netzbezug_in_w > 0  → kWh
  Einspeisung   = netzbezug_in_w < 0  → kWh
  Eigenverbrauch= Gesamtverbrauch - Netzbezug
  Autarkie      = Eigenverbrauch / Gesamtverbrauch × 100
"""
import pandas as pd
import numpy as np

CSV_PATH = '/mnt/solar.csv'

# --- Laden & Filtern ---
df = pd.read_csv(CSV_PATH, index_col=False)
df = df[df['zeitpunkt'] != 0]
df = df.dropna(subset=['zeitpunkt', 'solarerzeugung_in_w',
                        'netzbezug_in_w', 'stromverbrauch_in_w'])
df['datum'] = pd.to_datetime(df['datum'])
df['jahr']  = df['datum'].dt.year
df['monat'] = df['datum'].dt.month

# --- Aggregation (jede Zeile = 1 Minute → / 60000 = kWh) ---
def agg_energie(group):
    solar    = group['solarerzeugung_in_w'].sum() / 60000
    bezug    = group['netzbezug_in_w'].clip(lower=0).sum() / 60000
    einsp    = group['netzbezug_in_w'].clip(upper=0).abs().sum() / 60000
    gesamt   = group['stromverbrauch_in_w'].sum() / 60000
    eigenverb = gesamt - bezug
    autarkie = (eigenverb / gesamt * 100) if gesamt > 0 else 0
    return pd.Series({
        'solar_kwh':      solar,
        'eigenverb_kwh':  eigenverb,
        'bezug_kwh':      bezug,
        'einsp_kwh':      einsp,
        'autarkie_pct':   autarkie,
    })

monthly = df.groupby(['jahr', 'monat']).apply(agg_energie).reset_index()
yearly  = df.groupby('jahr').apply(agg_energie).reset_index()

# --- Tabellen-Helper ---
HEADS  = ['Solar', 'Eigenverbr.', 'Netzbezug', 'Einspeis.', 'Autarkie', 'Ges.Verbr.']
WIDTHS = [10, 12, 10, 10, 9, 12]

def hl(label_w):
    return '+' + '+'.join(['-'*(label_w+2)] + ['-'*(w+2) for w in WIDTHS]) + '+'

def hrow(label, label_w):
    cells = [f' {label:<{label_w}} '] + [f' {h:>{w}} ' for h, w in zip(HEADS, WIDTHS)]
    return '|' + '|'.join(cells) + '|'

def drow(label, label_w, r):
    vals = [
        f'{round(r.solar_kwh):>5} kWh',
        f'{round(r.eigenverb_kwh):>7} kWh',
        f'{round(r.bezug_kwh):>5} kWh',
        f'{round(r.einsp_kwh):>5} kWh',
        f'{r.autarkie_pct:>6.1f} %',
        f'{round(r.eigenverb_kwh + r.bezug_kwh):>8} kWh',
    ]
    cells = [f' {label:<{label_w}} '] + [f' {v:>{w}} ' for v, w in zip(vals, WIDTHS)]
    return '|' + '|'.join(cells) + '|'

# --- Ausgabe monatlich ---
MONTH_W = 7  # "YYYY-MM"
print('\nEnergiebilanz monatlich\n')
print(hl(MONTH_W))
print(hrow('Monat', MONTH_W))
print(hl(MONTH_W))

prev_year = None
for _, r in monthly.iterrows():
    label = f'{int(r.jahr)}-{int(r.monat):02d}'
    if prev_year and prev_year != r.jahr:
        print(hl(MONTH_W))
    prev_year = r.jahr
    print(drow(label, MONTH_W, r))

print(hl(MONTH_W))

# --- Ausgabe jahresweise ---
YEAR_W = 4
print('\nEnergiebilanz jahresweise\n')
print(hl(YEAR_W))
print(hrow('Jahr', YEAR_W))
print(hl(YEAR_W))
for _, r in yearly.iterrows():
    print(drow(str(int(r.jahr)), YEAR_W, r))
print(hl(YEAR_W))
print()
