#!/usr/bin/env python3
"""
Analyse des Umrechnungsfaktors Vorhersage → Produktion.
Ziel: Herausfinden welche Variablen den Faktor beeinflussen,
      um eine bessere Steuerung der Zusatzverbraucher zu ermöglichen.

Faktor = tatsächliche Produktion (kWh) / Vorhersage (kWh/m²)
       = effektive Anlagenfläche in m² (konstant wenn Vorhersage perfekt)

Ansätze:
  1. Binning nach Vorhersagehöhe
  2. Monatliche Faktoren
  3. Vorhersagequalität: Verhältnis Stunden- zu Tagesvorhersage
  4. Tagesform: Peak-Stunde vs. Tagesdurchschnitt
  5. Temperaturkorrelation
"""
import pandas as pd
import numpy as np

CSV_PATH = '/mnt/solar.csv'

# --- Laden & Filtern ---
print("Lade Daten...")
df = pd.read_csv(CSV_PATH, index_col=False)
df = df[df['zeitpunkt'] != 0]
df = df.dropna(subset=['zeitpunkt', 'solarerzeugung_in_w',
                        'tages_solarstrahlung', 'stunden_solarstrahlung', 'datum'])
df = df[df['tages_solarstrahlung'] > 0]
df = df[df['stunden_solarstrahlung'] >= 0]

df['jahr']  = df['jahr'].astype(int)
df['monat'] = df['monat'].astype(int)
df['tag']   = df['tag'].astype(int)
df['stunde'] = df['stunde'].astype(int)
df['datum'] = pd.to_datetime(df['datum'])

sommer_mask = (
    ((df['monat'] == 3)  & (df['tag'] >= 15)) |
    (df['monat'].between(4, 9))               |
    ((df['monat'] == 10) & (df['tag'] <= 30))
)
df['saison'] = np.where(sommer_mask, 'Sommer', 'Winter')

# Temperatur-Proxy: waermepumpen_zuluft_temperatur falls vorhanden
has_temp = 'waermepumpen_zuluft_temperatur' in df.columns and \
           df['waermepumpen_zuluft_temperatur'].notna().sum() > 1000

# --- Tagesaggregation ---
print("Aggregiere Tagesdaten...")

agg = {}
agg['produktion_kwh'] = ('solarerzeugung_in_w', lambda x: x.sum() / 60000)
agg['vorhersage_tages'] = ('tages_solarstrahlung', 'median')
agg['stunden_sum'] = ('stunden_solarstrahlung', 'sum')
agg['stunden_max'] = ('stunden_solarstrahlung', 'max')
agg['stunden_mean'] = ('stunden_solarstrahlung', 'mean')
if has_temp:
    agg['temp_mean'] = ('waermepumpen_zuluft_temperatur', 'mean')

group_cols = ['datum', 'jahr', 'monat', 'saison']
daily = df.groupby(group_cols).agg(**agg).reset_index()

# Nur Tage mit echter Produktion
daily = daily[daily['produktion_kwh'] > 0.1]
daily = daily[daily['vorhersage_tages'] > 100]

# Faktor
daily['faktor'] = daily['produktion_kwh'] / (daily['vorhersage_tages'] / 1000)

# Vorhersagequalität: Stunden-Summe / Tagesvorhersage
# stunden_solarstrahlung ist W/m² pro Stunde, tages in Wh/m²
# Summe der Stundenwerte * 1 (je eine Stunde) = Wh/m² approximiert Tagessumme
daily['vorh_konsistenz'] = daily['stunden_sum'] / daily['vorhersage_tages']

# Peakanteil: max Stunde / mean Stunde → niedrig = breite Kurve, hoch = schmaler Peak
daily['peak_ratio'] = daily['stunden_max'] / (daily['stunden_mean'] + 1)

print(f"  Tage gesamt: {len(daily)}, Sommer: {(daily['saison']=='Sommer').sum()}, Winter: {(daily['saison']=='Winter').sum()}")
print(f"  Zeitraum: {daily['datum'].min().date()} bis {daily['datum'].max().date()}")

# ============================================================
# AUSGABE-HELPER
# ============================================================
SEP = '-' * 70

def section(title):
    print(f"\n{'='*70}")
    print(f"  {title}")
    print('='*70)

def hl(widths):
    return '+' + '+'.join('-'*(w+2) for w in widths) + '+'

def hrow(headers, widths):
    return '|' + '|'.join(f' {h:>{w}} ' for h, w in zip(headers, widths)) + '|'

def drow(vals, widths):
    return '|' + '|'.join(f' {v:>{w}} ' for v, w in zip(vals, widths)) + '|'


# ============================================================
# 1. BINNING NACH VORHERSAGEHÖHE
# ============================================================
section("1. Faktor-Binning nach Vorhersagehöhe (Wh/m²)")

bins   = [0, 1000, 2000, 3000, 4500, 6000, 9999]
labels = ['<1000', '1-2k', '2-3k', '3-4.5k', '4.5-6k', '>6k']
daily['vorh_bin'] = pd.cut(daily['vorhersage_tages'], bins=bins, labels=labels)

for saison in ['Sommer', 'Winter']:
    d = daily[daily['saison'] == saison]
    print(f"\n  {saison}:")
    ws = [7, 5, 8, 8, 6, 6]
    heads = ['Klasse', 'Tage', 'Fakt.Med', 'Fakt.Std', 'Q25', 'Q75']
    print(hl(ws))
    print(hrow(heads, ws))
    print(hl(ws))
    for label in labels:
        g = d[d['vorh_bin'] == label]['faktor']
        if len(g) < 3:
            continue
        print(drow([label, len(g), f'{g.median():.2f}', f'{g.std():.2f}',
                    f'{g.quantile(0.25):.2f}', f'{g.quantile(0.75):.2f}'], ws))
    print(hl(ws))


# ============================================================
# 2. MONATLICHE FAKTOREN
# ============================================================
section("2. Faktor pro Monat")

ws = [4, 5, 8, 8, 6, 6]
heads = ['Mon', 'Tage', 'Fakt.Med', 'Fakt.Std', 'Q25', 'Q75']
print(hl(ws))
print(hrow(heads, ws))
print(hl(ws))
for monat in sorted(daily['monat'].unique()):
    g = daily[daily['monat'] == monat]['faktor']
    print(drow([monat, len(g), f'{g.median():.2f}', f'{g.std():.2f}',
                f'{g.quantile(0.25):.2f}', f'{g.quantile(0.75):.2f}'], ws))
print(hl(ws))


# ============================================================
# 3. VORHERSAGE-KONSISTENZ (Stunden-Summe / Tagesvorhersage)
# ============================================================
section("3. Faktor vs. Vorhersage-Konsistenz (stunden_sum / tages_vorhersage)")
print("  (Wert ~1 = konsistente Vorhersage, <1 oder >1 = Abweichung)")

# stunden_solarstrahlung ist W/m² für jede Minute → sum / 60 = Wh/m²
# Korrektur:
daily['vorh_konsistenz_korr'] = (daily['stunden_sum'] / 60) / daily['vorhersage_tages']

kbins = [0, 0.5, 0.8, 1.2, 1.5, 99]
klabs = ['<0.5', '0.5-0.8', '0.8-1.2', '1.2-1.5', '>1.5']
daily['k_bin'] = pd.cut(daily['vorh_konsistenz_korr'], bins=kbins, labels=klabs)

ws = [8, 5, 8, 8]
heads = ['Konsis.', 'Tage', 'Fakt.Med', 'Fakt.Std']
print(hl(ws))
print(hrow(heads, ws))
print(hl(ws))
for lab in klabs:
    g = daily[daily['k_bin'] == lab]['faktor']
    if len(g) < 3:
        continue
    print(drow([lab, len(g), f'{g.median():.2f}', f'{g.std():.2f}'], ws))
print(hl(ws))

# Korrelation
corr_k = daily[['vorh_konsistenz_korr', 'faktor']].corr().iloc[0, 1]
print(f"  Korrelation Konsistenz↔Faktor: {corr_k:.3f}")


# ============================================================
# 4. TAGESFORM: PEAK-RATIO
# ============================================================
section("4. Faktor vs. Tagesform (Peak-Stunde / Tagesmittel)")
print("  Niedrig = breite Kurve (klarer Sommertag), Hoch = schmaler Peak")

pbins = [0, 2, 3, 5, 8, 999]
plabs = ['<2', '2-3', '3-5', '5-8', '>8']
daily['p_bin'] = pd.cut(daily['peak_ratio'], bins=pbins, labels=plabs)

ws = [5, 5, 8, 8]
heads = ['Peak', 'Tage', 'Fakt.Med', 'Fakt.Std']
print(hl(ws))
print(hrow(heads, ws))
print(hl(ws))
for lab in plabs:
    g = daily[daily['p_bin'] == lab]['faktor']
    if len(g) < 3:
        continue
    print(drow([lab, len(g), f'{g.median():.2f}', f'{g.std():.2f}'], ws))
print(hl(ws))

corr_p = daily[['peak_ratio', 'faktor']].corr().iloc[0, 1]
print(f"  Korrelation Peak-Ratio↔Faktor: {corr_p:.3f}")


# ============================================================
# 5. TEMPERATURKORRELATION
# ============================================================
if has_temp:
    section("5. Faktor vs. Temperatur")
    daily_t = daily.dropna(subset=['temp_mean'])

    tbins = [0, 5, 10, 15, 20, 25, 30, 50]
    tlabs = ['<5°C', '5-10', '10-15', '15-20', '20-25', '25-30', '>30°C']
    daily_t = daily_t.copy()
    daily_t['t_bin'] = pd.cut(daily_t['temp_mean'], bins=tbins, labels=tlabs)

    ws = [6, 5, 8, 8]
    heads = ['Temp', 'Tage', 'Fakt.Med', 'Fakt.Std']
    print(hl(ws))
    print(hrow(heads, ws))
    print(hl(ws))
    for lab in tlabs:
        g = daily_t[daily_t['t_bin'] == lab]['faktor']
        if len(g) < 3:
            continue
        print(drow([lab, len(g), f'{g.median():.2f}', f'{g.std():.2f}'], ws))
    print(hl(ws))

    corr_t = daily_t[['temp_mean', 'faktor']].corr().iloc[0, 1]
    print(f"  Korrelation Temperatur↔Faktor: {corr_t:.3f}")
else:
    section("5. Temperatur: keine ausreichenden Daten")


# ============================================================
# 6. KOMBINIERT: Vorhersagehöhe × Saison (Empfehlung)
# ============================================================
section("6. Empfehlung: Faktor-Tabelle Saison × Vorhersage-Klasse")
print("  Diese Tabelle kann direkt als Lookup-Tabelle im Steuerungssystem genutzt werden.")
print("  Wert = Median-Faktor (m²). IQR zeigt verbleibende Unsicherheit.\n")

ws = [7, 8, 6, 8, 6]
heads = ['Klasse', 'S-Fakt', 'S-IQR', 'W-Fakt', 'W-IQR']
print(hl(ws))
print(hrow(heads, ws))
print(hl(ws))
for label in labels:
    row = [label]
    for saison in ['Sommer', 'Winter']:
        g = daily[(daily['saison'] == saison) & (daily['vorh_bin'] == label)]['faktor']
        if len(g) < 3:
            row += ['-', '-']
        else:
            iqr = g.quantile(0.75) - g.quantile(0.25)
            row += [f'{g.median():.2f}', f'{iqr:.2f}']
    print(drow(row, ws))
print(hl(ws))

# ============================================================
# 7. ERKLAERTE VARIANZ
# ============================================================
section("7. Erklärte Varianz (wie viel des Faktors erklärt jede Variable?)")
print("  R² = 1 → perfekte Erklärung, 0 → kein Zusammenhang\n")

from sklearn.linear_model import LinearRegression
from sklearn.preprocessing import LabelEncoder

try:
    d = daily.dropna(subset=['faktor', 'vorhersage_tages', 'peak_ratio',
                              'vorh_konsistenz_korr', 'monat'])
    results = []
    for col, label in [
        ('vorhersage_tages',   'Vorhersagehöhe'),
        ('monat',              'Monat'),
        ('peak_ratio',         'Tagesform (Peak)'),
        ('vorh_konsistenz_korr','Konsistenz Std/Tag'),
    ]:
        X = d[[col]].values
        y = d['faktor'].values
        lr = LinearRegression().fit(X, y)
        r2 = lr.score(X, y)
        results.append((label, r2))

    if has_temp:
        d2 = d.dropna(subset=['temp_mean'])
        X = d2[['temp_mean']].values
        y = d2['faktor'].values
        lr = LinearRegression().fit(X, y)
        results.append(('Temperatur', lr.score(X, y)))

    results.sort(key=lambda x: -x[1])
    ws2 = [22, 6]
    print(hl(ws2))
    print(hrow(['Variable', 'R²'], ws2))
    print(hl(ws2))
    for label, r2 in results:
        print(drow([label, f'{r2:.4f}'], ws2))
    print(hl(ws2))
except ImportError:
    print("  sklearn nicht verfügbar, übersprungen.")
