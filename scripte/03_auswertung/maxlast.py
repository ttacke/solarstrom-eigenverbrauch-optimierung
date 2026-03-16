#!/usr/bin/env python3
"""
Script#1: Maximallasten und Überlast
"""
import pandas as pd

CSV_PATH   = '../solar.csv'
LIMIT_W       = 6500  # Lastschutz-Schwellwert (Steuerung)
LIMIT_PHASE_W = 5520  # Phasen-Grenzwert (24A × 230V)
VOLTAGE_V     = 230

def format_duration(minutes):
    h = int(minutes) // 60
    m = int(minutes) % 60
    return f"{h}:{m:02d}"

def fw(w):
    if pd.isna(w):
        return '-'
    return f"{int(w)}W"

def print_simple_table(title, headers, rows_data, W_Y=4):
    widths = [max(len(h), 7) for h in headers]

    def hl():
        return '+' + '+'.join(['-'*(W_Y+2)] + ['-'*(w+2) for w in widths]) + '+'
    def h_row():
        cells = [f' {"Jahr":^{W_Y}} '] + [f' {h:>{w}} ' for h, w in zip(headers, widths)]
        return '|' + '|'.join(cells) + '|'
    def d_row(year, vals):
        cells = [f' {str(year):>{W_Y}} '] + [f' {v:>{w}} ' for v, w in zip(vals, widths)]
        return '|' + '|'.join(cells) + '|'

    print(title)
    print(hl())
    print(h_row())
    print(hl())
    for year, vals in rows_data:
        print(d_row(year, vals))
    print(hl())
    print()

def print_table(title, groups, rows_data, W_Y=4, W_B=6, W_E=11):
    SPAN = W_B + W_E + 3
    n    = len(groups)

    def hl_g():
        return '+' + '+'.join(['-'*(W_Y+2)] + ['-'*(SPAN+2)]*n) + '+'
    def hl_c():
        return '+' + '+'.join(['-'*(W_Y+2)] + (['-'*(W_B+2), '-'*(W_E+2)]*n)) + '+'
    def g_row():
        cells = [f' {"":{W_Y}} '] + [f' {g:^{SPAN}} ' for g in groups]
        return '|' + '|'.join(cells) + '|'
    def h_row():
        cells = [f' {"Jahr":^{W_Y}} ']
        for _ in groups:
            cells += [f' {"Bezug":>{W_B}} ', f' {"Einspeisung":>{W_E}} ']
        return '|' + '|'.join(cells) + '|'
    def d_row(year, vals):
        cells = [f' {str(year):>{W_Y}} ']
        ws    = [W_B, W_E] * n
        for v, w in zip(vals, ws):
            cells.append(f' {v:>{w}} ')
        return '|' + '|'.join(cells) + '|'

    print(title)
    print(hl_g())
    print(g_row())
    print(hl_c())
    print(h_row())
    print(hl_c())
    for year, vals in rows_data:
        print(d_row(year, vals))
    print(hl_c())
    print()

# --- Daten laden ---
df = pd.read_csv(CSV_PATH, index_col=False)
df = df[df['zeitpunkt'] != 0]
df = df.dropna(subset=['zeitpunkt', 'netzbezug_in_w', 'jahr'])
df['jahr'] = df['jahr'].astype(int)

for phase in ['l1', 'l2', 'l3']:
    col = f'{phase}_strom_ma'
    df[f'{phase}_bezug_w']       = df[col].clip(lower=0) * VOLTAGE_V / 1000
    df[f'{phase}_einspeisung_w'] = df[col].clip(upper=0).abs() * VOLTAGE_V / 1000

df['bezug_gesamt_w']       = df['netzbezug_in_w'].clip(lower=0)
df['einspeisung_gesamt_w'] = df['netzbezug_in_w'].clip(upper=0).abs()

# Überlast-Flags
df['ul_ges_bezug'] = df['bezug_gesamt_w']       > LIMIT_W
df['ul_ges_einsp'] = df['einspeisung_gesamt_w'] > LIMIT_W
for phase in ['l1', 'l2', 'l3']:
    df[f'ul_{phase}_bezug'] = df[f'{phase}_bezug_w']       > LIMIT_PHASE_W
    df[f'ul_{phase}_einsp'] = df[f'{phase}_einspeisung_w'] > LIMIT_PHASE_W

# Saison: Sommer 15.3–30.10, sonst Winter
df['monat'] = df['monat'].astype(int)
df['tag']   = df['tag'].astype(int)
sommer_mask = (
    ((df['monat'] == 3)  & (df['tag'] >= 15)) |
    (df['monat'].between(4, 9))               |
    ((df['monat'] == 10) & (df['tag'] <= 30))
)
df['saison'] = 'Winter'
df.loc[sommer_mask, 'saison'] = 'Sommer'

# Tageslicht-Flag für Lastschutz-Analyse
df['ist_tag'] = df['solarerzeugung_in_w'] > 0

years = sorted(df['jahr'].unique())

# --- Ausgabe ---
print(f"""
Maximallasten und Überlast – Jahresauswertung
  Per-Phase:  Fronius-Messung, Annahme {VOLTAGE_V}V pro Phase
  Gesamt:     Smart Meter (netzbezug_in_w)
  Überlast Gesamt:    > {LIMIT_W}W  (Hausanschluss-Auslegung)
  Überlast Pro Phase: > {LIMIT_PHASE_W}W  ({LIMIT_PHASE_W // VOLTAGE_V}A × {VOLTAGE_V}V)
""")

# Tabelle 1: Maximalwerte
t1_rows = []
for year in years:
    y = df[df['jahr'] == year]
    t1_rows.append((year, [
        fw(y['l1_bezug_w'].max()),          fw(y['l1_einspeisung_w'].max()),
        fw(y['l2_bezug_w'].max()),          fw(y['l2_einspeisung_w'].max()),
        fw(y['l3_bezug_w'].max()),          fw(y['l3_einspeisung_w'].max()),
        fw(y['bezug_gesamt_w'].max()),      fw(y['einspeisung_gesamt_w'].max()),
    ]))

print_table(
    "Tabelle 1: Maximale Leistung pro Jahr",
    ['L1', 'L2', 'L3', 'Gesamt'],
    t1_rows
)

# Tabelle 2: Überlast-Dauer
t2_rows = []
for year in years:
    y = df[df['jahr'] == year]
    t2_rows.append((year, [
        format_duration(y['ul_l1_bezug'].sum()),   format_duration(y['ul_l1_einsp'].sum()),
        format_duration(y['ul_l2_bezug'].sum()),   format_duration(y['ul_l2_einsp'].sum()),
        format_duration(y['ul_l3_bezug'].sum()),   format_duration(y['ul_l3_einsp'].sum()),
        format_duration(y['ul_ges_bezug'].sum()),  format_duration(y['ul_ges_einsp'].sum()),
    ]))

print_table(
    f"Tabelle 2: Überlast-Dauer (hh:mm) | Gesamt >{LIMIT_W}W | Pro Phase >{LIMIT_PHASE_W}W",
    ['L1', 'L2', 'L3', 'Gesamt'],
    t2_rows
)

# Tabelle 3: Grundverbrauch (Median stromverbrauch_in_w)
SOMMER_DEF = '15. März – 30. Oktober'
WINTER_DEF = 'alle übrigen Tage'
t3_rows = []
for year in years:
    y = df[df['jahr'] == year].dropna(subset=['stromverbrauch_in_w'])
    med_s = y.loc[y['saison'] == 'Sommer', 'stromverbrauch_in_w'].median()
    med_w = y.loc[y['saison'] == 'Winter', 'stromverbrauch_in_w'].median()
    t3_rows.append((year, [fw(med_s), fw(med_w)]))

print_simple_table(
    f"Tabelle 3: Grundverbrauch (Median) | Sommer: {SOMMER_DEF} | Winter: {WINTER_DEF}",
    ['Sommer', 'Winter'],
    t3_rows
)

# Tabelle 4: Lastschutz-Statistik
# auto/roller: Energie verhindert = benoetigte_ladeleistung × blockierte Minuten / 60 / 1000
# heizung/wasser: Relay = Überlad-Signal (WP läuft ggf. trotzdem normal) → Energie nicht quantifizierbar
LASTSCHUTZ = [
    ('Auto (Wallbox)', 'auto_lastschutz',    'auto_benoetigte_ladeleistung_in_w',   2300),
    ('Roller',         'roller_lastschutz',  'roller_benoetigte_ladeleistung_in_w', 840),
    ('Heizung-WP',     'heizung_lastschutz', None,                                  None),
    ('Wasser-WP',      'wasser_lastschutz',  None,                                  None),
]

def print_lastschutz_table(year, y):
    WC = 16; WB = 9; WT = 9; WE = 11

    def hl():
        return f"+{'-'*(WC+2)}+{'-'*(WB+2)}+{'-'*(WT+2)}+{'-'*(WE+2)}+"

    print(f"Tabelle 4: Lastschutz-Statistik — {year}  (Schwellwert: {LIMIT_W}W)")
    print(f"  tagsüber = solarerzeugung_in_w > 0")
    print(hl())
    print(f"| {'Verbraucher':{WC}} | {'Blockiert':>{WB}} | {'Tagsüber':>{WT}} | {'Energie':>{WE}} |")
    print(hl())
    for label, flag_col, leistung_col, fallback_w in LASTSCHUTZ:
        if flag_col not in y.columns:
            print(f"| {label:{WC}} | {'n.v.':>{WB}} | {'n.v.':>{WT}} | {'n.v.':>{WE}} |")
            continue
        blocked = y[flag_col] == 1
        n_blocked = int(blocked.sum())
        n_tag     = int((blocked & y['ist_tag']).sum())
        if leistung_col and leistung_col in y.columns:
            lp = y.loc[blocked, leistung_col].fillna(fallback_w)
            lp = lp.where(lp > 0, fallback_w)
            energie_kwh = (lp / 60 / 1000).sum()
            energie_str = f"{energie_kwh:.1f} kWh"
        else:
            energie_str = '-'
        print(f"| {label:{WC}} | {format_duration(n_blocked):>{WB}} | {format_duration(n_tag):>{WT}} | {energie_str:>{WE}} |")
    print(hl())
    print()

for year in years:
    print_lastschutz_table(year, df[df['jahr'] == year])
