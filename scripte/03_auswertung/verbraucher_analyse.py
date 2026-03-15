#!/usr/bin/env python3
"""
Verbraucher-Analyse: Heizstab, Heizungs-WP, Begleitheizung, Warmwasser-WP, Wallbox, Roller

Methoden:
  Heizstab       : heizungsunterstuetzung_an == 1 → 1500 W
  Heizungs-WP    : waermepumpen_abluft_temperatur in (0, 7) → 420 W
  Begleitheizung : begleitheizung_leistung (W direkt)
  Warmwasser-WP  : wasser_wp_leistung (W, Standby 0–2 W filtern)
  Wallbox        : auto_laden_ist_an == 1 → auto_benoetigte_ladeleistung_in_w W
  Roller         : roller_laden_ist_an == 1 → roller_benoetigte_ladeleistung_in_w W
  Solar-EV       : solarerzeugung - einspeisung (= selbst genutzter Solarstrom)

Spaltentrennlinien: || zwischen Verbraucher-Gruppen, | innerhalb
"""
import pandas as pd
import numpy as np

CSV_PATH              = '/mnt/solar.csv'
WALLBOX_LEISTUNG_W    = 2300   # Fallback wenn auto_benoetigte_ladeleistung_in_w fehlt
ROLLER_LEISTUNG_W     = 840    # Fallback wenn roller_benoetigte_ladeleistung_in_w fehlt
HEIZSTAB_LEISTUNG_W   = 1500
WP_HEIZUNG_LEISTUNG_W = 420
WP_HEIZUNG_MAX_TEMP   = 7.0
STANDBY_MAX_W         = 2
EUR_PRO_KWH           = 0.33
DAILY_COUNT           = 14

# Spaltenbreiten: einheitlich in allen Tabellen
W_KWH  = 10   # "99999 kWh" = 9 chars
W_EUR  =  9   # " 9999.99 €" = 9 chars  (f'{val:>7.2f} €')
W_PCT  =  7   # "100.0 %" = 7 chars     (f'{val:>5.1f} %')
W_TAKT =  5   # "  99" = up to 5 chars


# --- Laden & Filtern ---
print("Lade Daten...")
df = pd.read_csv(CSV_PATH, index_col=False)
df = df[df['zeitpunkt'] != 0].copy()
df = df.dropna(subset=['zeitpunkt', 'datum', 'jahr', 'monat',
                        'solarerzeugung_in_w', 'netzbezug_in_w', 'stromverbrauch_in_w'])

df['datum'] = pd.to_datetime(df['datum'])
df['jahr']  = df['jahr'].astype(int)
df['monat'] = df['monat'].astype(int)
df = df[df['jahr'] >= 2026].copy()

print(f"  {len(df):,} Zeilen, {df['datum'].min().date()} bis {df['datum'].max().date()}")


# --- Leistungen berechnen (jede Zeile = 1 Minute) ---

if 'heizungsunterstuetzung_an' in df.columns:
    df['heizstab_w'] = np.where(
        df['heizungsunterstuetzung_an'].fillna(0).astype(int) == 1,
        HEIZSTAB_LEISTUNG_W, 0
    )
else:
    df['heizstab_w'] = 0

if 'waermepumpen_abluft_temperatur' in df.columns:
    temp = df['waermepumpen_abluft_temperatur'].fillna(0)
    df['heiz_wp_w'] = np.where(
        (temp > 0) & (temp < WP_HEIZUNG_MAX_TEMP),
        WP_HEIZUNG_LEISTUNG_W, 0
    )
else:
    df['heiz_wp_w'] = 0

if 'begleitheizung_leistung' in df.columns:
    df['begleit_w'] = df['begleitheizung_leistung'].fillna(0).clip(lower=0)
else:
    df['begleit_w'] = 0

if 'wasser_wp_leistung' in df.columns:
    ww = df['wasser_wp_leistung'].fillna(0)
    df['ww_wp_w'] = np.where(ww > STANDBY_MAX_W, ww, 0)
else:
    df['ww_wp_w'] = 0

if 'auto_laden_ist_an' in df.columns:
    auto_an = df['auto_laden_ist_an'].fillna(0).astype(int) == 1
    leistung = df['auto_benoetigte_ladeleistung_in_w'].fillna(WALLBOX_LEISTUNG_W) \
               if 'auto_benoetigte_ladeleistung_in_w' in df.columns \
               else WALLBOX_LEISTUNG_W
    df['wallbox_w'] = np.where(auto_an, leistung, 0)
else:
    df['wallbox_w'] = 0

if 'roller_laden_ist_an' in df.columns:
    roller_an = df['roller_laden_ist_an'].fillna(0).astype(int) == 1
    leistung = df['roller_benoetigte_ladeleistung_in_w'].fillna(ROLLER_LEISTUNG_W) \
               if 'roller_benoetigte_ladeleistung_in_w' in df.columns \
               else ROLLER_LEISTUNG_W
    df['roller_w'] = np.where(roller_an, leistung, 0)
else:
    df['roller_w'] = 0


# --- Aggregation ---
POWER_COLS = ['heizstab_w', 'heiz_wp_w', 'begleit_w', 'ww_wp_w', 'wallbox_w', 'roller_w']
KWH_COLS   = ['heizstab_kwh', 'heiz_wp_kwh', 'begleit_kwh', 'ww_wp_kwh', 'wallbox_kwh', 'roller_kwh']


def agg_group(grp):
    result = {kwh: grp[w].sum() / 60000 for w, kwh in zip(POWER_COLS, KWH_COLS)}

    solar  = grp['solarerzeugung_in_w'].sum() / 60000
    einsp  = grp['netzbezug_in_w'].clip(upper=0).abs().sum() / 60000
    gesamt = grp['stromverbrauch_in_w'].sum() / 60000
    result['solar_ev_kwh'] = max(solar - einsp, 0)
    result['solar_ev_pct'] = result['solar_ev_kwh'] / gesamt * 100 if gesamt > 0 else 0.0

    n = len(grp)
    result['heizstab_pct']  = (grp['heizstab_w'] > 0).sum() / n * 100 if n > 0 else 0.0
    result['heiz_wp_pct']   = (grp['heiz_wp_w']  > 0).sum() / n * 100 if n > 0 else 0.0

    active = grp['heiz_wp_w'] > 0
    result['heiz_wp_takte'] = int((active & ~active.shift(1).fillna(False)).sum())

    active = grp['wallbox_w'] > 0
    result['wallbox_takte'] = int((active & ~active.shift(1).fillna(False)).sum())

    active = grp['roller_w'] > 0
    result['roller_takte'] = int((active & ~active.shift(1).fillna(False)).sum())

    return pd.Series(result)


df['tag'] = df['datum'].dt.date
monthly = df.groupby(['jahr', 'monat']).apply(agg_group).reset_index()
yearly  = df.groupby('jahr').apply(agg_group).reset_index()
daily   = df.groupby('tag').apply(agg_group).reset_index()


# --- Tabellen-Konfiguration ---
# Reihenfolge: Heizstab, Heizungs-WP, Begleit., WW-WP, Wallbox, Roller, Solar-EV
VERBRAUCHER = [
    ('Heizstab',    'heizstab_kwh'),
    ('Heizungs-WP', 'heiz_wp_kwh'),
    ('Begleit.',    'begleit_kwh'),
    ('WW-WP',       'ww_wp_kwh'),
    ('Wallbox',     'wallbox_kwh'),
    ('Roller',      'roller_kwh'),
]

def build_config(daily_extras=False):
    """Baut Heads, Widths und thick_before-Set für eine Tabelle.
    daily_extras=True: %Tag bei Heizstab, %Tag+Takte bei Heizungs-WP."""
    heads, widths, thick = [], [], set()
    for i, (name, _) in enumerate(VERBRAUCHER):
        if i > 0:
            thick.add(len(widths))
        heads  += [name, '€']
        widths += [W_KWH, W_EUR]
        if daily_extras and name == 'Heizstab':
            heads  += ['%Tag']
            widths += [W_PCT]
        elif daily_extras and name == 'Heizungs-WP':
            heads  += ['%Tag', 'Takte']
            widths += [W_PCT, W_TAKT]
        elif daily_extras and name in ('Wallbox', 'Roller'):
            heads  += ['Takte']
            widths += [W_TAKT]
    thick.add(len(widths))
    heads  += ['Solar-EV', '€', '%Haus']
    widths += [W_KWH, W_EUR, W_PCT]
    return heads, widths, thick

MY_HEADS, MY_WIDTHS, MY_THICK = build_config(daily_extras=False)
D_HEADS,  D_WIDTHS,  D_THICK  = build_config(daily_extras=True)


# --- Tabellen-Helper (|| zwischen Gruppen, | innerhalb) ---
def hl(label_w, widths, thick):
    line = '+' + '-' * (label_w + 2) + '++'
    for i, w in enumerate(widths):
        line += '-' * (w + 2)
        line += ('+' if i == len(widths) - 1 else
                 '++' if (i + 1) in thick else '+')
    return line


def trow(label, label_w, vals, widths, thick):
    line = '|' + f' {label:<{label_w}} ' + '||'
    for i, (v, w) in enumerate(zip(vals, widths)):
        line += f' {v:>{w}} '
        line += ('|' if i == len(vals) - 1 else
                 '||' if (i + 1) in thick else '|')
    return line


def fmt_eur(kwh):
    return f'{kwh * EUR_PRO_KWH:>7.2f} €'


def build_monthly_vals(r):
    vals = []
    for _, col in VERBRAUCHER:
        kwh = getattr(r, col)
        vals += [f'{round(kwh):>5} kWh', fmt_eur(kwh)]
    kwh = r.solar_ev_kwh
    vals += [f'{round(kwh):>5} kWh', fmt_eur(kwh), f'{r.solar_ev_pct:>5.1f} %']
    return vals


def build_daily_vals(r):
    vals = []
    for name, col in VERBRAUCHER:
        kwh = getattr(r, col)
        vals += [f'{kwh:>5.2f} kWh', fmt_eur(kwh)]
        if name == 'Heizstab':
            vals += [f'{r.heizstab_pct:>5.1f} %']
        elif name == 'Heizungs-WP':
            vals += [f'{r.heiz_wp_pct:>5.1f} %', f'{int(r.heiz_wp_takte):>5}']
        elif name == 'Wallbox':
            vals += [f'{int(r.wallbox_takte):>5}']
        elif name == 'Roller':
            vals += [f'{int(r.roller_takte):>5}']
    kwh = r.solar_ev_kwh
    vals += [f'{kwh:>5.2f} kWh', fmt_eur(kwh), f'{r.solar_ev_pct:>5.1f} %']
    return vals


# --- Ausgabe monatlich ---
MONTH_W = 7
print('\nVerbraucher-Analyse monatlich\n')
print(hl(MONTH_W, MY_WIDTHS, MY_THICK))
print(trow('Monat', MONTH_W, MY_HEADS, MY_WIDTHS, MY_THICK))
print(hl(MONTH_W, MY_WIDTHS, MY_THICK))

prev_year = None
for _, r in monthly.iterrows():
    label = f'{int(r.jahr)}-{int(r.monat):02d}'
    if prev_year and prev_year != r.jahr:
        print(hl(MONTH_W, MY_WIDTHS, MY_THICK))
    prev_year = r.jahr
    print(trow(label, MONTH_W, build_monthly_vals(r), MY_WIDTHS, MY_THICK))

print(hl(MONTH_W, MY_WIDTHS, MY_THICK))

# --- Ausgabe jahresweise ---
YEAR_W = 4
print('\nVerbraucher-Analyse jahresweise\n')
print(hl(YEAR_W, MY_WIDTHS, MY_THICK))
print(trow('Jahr', YEAR_W, MY_HEADS, MY_WIDTHS, MY_THICK))
print(hl(YEAR_W, MY_WIDTHS, MY_THICK))
for _, r in yearly.iterrows():
    print(trow(str(int(r.jahr)), YEAR_W, build_monthly_vals(r), MY_WIDTHS, MY_THICK))
print(hl(YEAR_W, MY_WIDTHS, MY_THICK))

# --- Ausgabe letzte 14 Tage tageweise ---
DAY_W  = 10
last_n = daily.sort_values('tag').tail(DAILY_COUNT)
print(f'\nVerbraucher-Analyse letzte {DAILY_COUNT} Tage\n')
print(hl(DAY_W, D_WIDTHS, D_THICK))
print(trow('Tag', DAY_W, D_HEADS, D_WIDTHS, D_THICK))
print(hl(DAY_W, D_WIDTHS, D_THICK))
for _, r in last_n.iterrows():
    print(trow(str(r.tag), DAY_W, build_daily_vals(r), D_WIDTHS, D_THICK))
print(hl(DAY_W, D_WIDTHS, D_THICK))
print()
