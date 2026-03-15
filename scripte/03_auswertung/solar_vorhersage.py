#!/usr/bin/env python3
"""
Solarvorhersage vs. Realität

Vergleich tages_solarstrahlung (Wh/m², open-meteo) mit tatsächlicher
Produktion (solarerzeugung_in_w). Faktor = Prod.(kWh) / Vorh.(kWh/m²)
entspricht effektiver Anlagenfläche (m²) und bleibt konstant, wenn die
Vorhersage proportional zur Realität ist.
IQR = Interquartilabstand des Faktors → Maß für Vorhersagezuverlässigkeit.

Sommer: 15.3.–30.10.   Winter: alle übrigen Tage
"""
import pandas as pd
import numpy as np

CSV_PATH = '/mnt/solar.csv'

# --- Laden & Filtern ---
df = pd.read_csv(CSV_PATH, index_col=False)
df = df[df['zeitpunkt'] != 0]
df = df.dropna(subset=['zeitpunkt', 'solarerzeugung_in_w',
                        'tages_solarstrahlung', 'datum'])
df = df[df['tages_solarstrahlung'] > 0]

df['jahr']  = df['jahr'].astype(int)
df['monat'] = df['monat'].astype(int)
df['tag']   = df['tag'].astype(int)
df['datum'] = pd.to_datetime(df['datum'])

sommer_mask = (
    ((df['monat'] == 3)  & (df['tag'] >= 15)) |
    (df['monat'].between(4, 9))               |
    ((df['monat'] == 10) & (df['tag'] <= 30))
)
df['saison'] = np.where(sommer_mask, 'Sommer', 'Winter')

# --- Tagesaggregation ---
# Produktion: sum(W) * 1min / 60 = Wh → / 1000 = kWh
daily_prod = (
    df.groupby(['datum', 'jahr', 'saison'])
    .agg(produktion_kwh=('solarerzeugung_in_w', lambda x: x.sum() / 60000))
    .reset_index()
)

# Vorhersage: Median des Tages (tages_solarstrahlung ist tagesweit konstant)
daily_vorh = (
    df.groupby('datum')
    .agg(vorhersage=('tages_solarstrahlung', 'median'))
    .reset_index()
)

daily = daily_prod.merge(daily_vorh, on='datum', how='inner')

# Faktor: kWh / (Wh/m² / 1000) = m² effektive Anlagenfläche
daily['faktor'] = daily['produktion_kwh'] / (daily['vorhersage'] / 1000)

# Nur Tage mit echter Produktion (kein reiner Bewölkungstag mit 0 Produktion)
daily = daily[daily['produktion_kwh'] > 0.1]

# --- Jahres-/Saison-Aggregation ---
stats = (
    daily
    .groupby(['jahr', 'saison'])
    .agg(
        n          =('faktor',        'count'),
        vorh_median=('vorhersage',    'median'),
        prod_median=('produktion_kwh','median'),
        fakt_median=('faktor',        'median'),
        fakt_q25   =('faktor',        lambda x: x.quantile(0.25)),
        fakt_q75   =('faktor',        lambda x: x.quantile(0.75)),
    )
    .reset_index()
)
stats['fakt_iqr'] = stats['fakt_q75'] - stats['fakt_q25']

# --- Tabellenausgabe ---
def hl_groups(W_Y, spans):
    return '+' + '+'.join(['-'*(W_Y+2)] + ['-'*(s+2) for s in spans]) + '+'

def hl_cols(W_Y, widths):
    return '+' + '+'.join(['-'*(W_Y+2)] + ['-'*(w+2) for w in widths]) + '+'

def group_row(W_Y, groups, spans):
    cells = [f' {"":{W_Y}} '] + [f' {g:^{s}} ' for g, s in zip(groups, spans)]
    return '|' + '|'.join(cells) + '|'

def header_row(W_Y, headers, widths):
    cells = [f' {"Jahr":^{W_Y}} '] + [f' {h:>{w}} ' for h, w in zip(headers, widths)]
    return '|' + '|'.join(cells) + '|'

def data_row(W_Y, vals, widths):
    cells = [f' {str(vals[0]):>{W_Y}} '] + [f' {v:>{w}} ' for v, w in zip(vals[1:], widths)]
    return '|' + '|'.join(cells) + '|'

# Spalten pro Saison
COLS    = ['Vorh.Wh/m²', 'Prod.kWh', 'Faktor m²', 'IQR m²']
WIDTHS  = [10,            8,           9,            6]
SPAN = sum(w + 2 for w in WIDTHS) + len(WIDTHS) - 3  # content-Breite der Gruppen-Kopfzeile

W_Y = 4
years = sorted(stats['jahr'].unique())

print("""
Solarvorhersage vs. Realität (jahresweise, Sommer/Winter)
  Vorhersage: tages_solarstrahlung (Wh/m², open-meteo)
  Produktion: solarerzeugung_in_w (W → kWh/Tag)
  Faktor:     Prod.(kWh) / Vorh.(kWh/m²) = effektive Anlagenfläche (m²)
  IQR:        Interquartilabstand Faktor → kleiner = zuverlässigere Vorhersage
  Sommer:     15.3.–30.10.   Winter: alle übrigen Tage
""")

print(hl_groups(W_Y, [SPAN, SPAN]))
print(group_row(W_Y, ['Sommer', 'Winter'], [SPAN, SPAN]))
print(hl_cols(W_Y, WIDTHS * 2))
print(header_row(W_Y, COLS * 2, WIDTHS * 2))
print(hl_cols(W_Y, WIDTHS * 2))

for year in years:
    row_vals = [year]
    for saison in ['Sommer', 'Winter']:
        sub = stats[(stats['jahr'] == year) & (stats['saison'] == saison)]
        if sub.empty:
            row_vals += ['-', '-', '-', '-']
        else:
            r = sub.iloc[0]
            row_vals += [
                f'{r.vorh_median:.0f}',
                f'{r.prod_median:.1f}',
                f'{r.fakt_median:.2f}',
                f'{r.fakt_iqr:.2f}',
            ]
    print(data_row(W_Y, row_vals, WIDTHS * 2))

print(hl_cols(W_Y, WIDTHS * 2))
print()
