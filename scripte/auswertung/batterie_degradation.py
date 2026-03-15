#!/usr/bin/env python3
"""
Batterie-Degradation ermitteln aus solar.log.

Referenzkapazität: 7.65 kWh (BYD HVS 7.7 Brutto-Nennkapazität)
Methode: Halbzyklen (Peak→Valley, Valley→Peak) per SoC-Extrema identifizieren,
         Energie integrieren (W × 60s), auf 1000‰ Hub normalisieren.
Vorzeichen solarakku_zuschuss_in_w: negativ=laden, positiv=entladen
"""

import pandas as pd
import numpy as np
from datetime import datetime, timedelta

# --- Parameter ---
INPUT_CSV       = '/mnt/solar.csv'
NOMINAL_KWH     = 7.65     # BYD HVS 7.7 Brutto
SAMPLE_S        = 60       # Messintervall in Sekunden
MIN_SOC_SWING   = 300      # Mindest-Hub in Promille (= 30%) für gültigen Halbzyklus
SMOOTH_WINDOW   = 15       # Glättungsfenster SoC (Minuten)
PEAK_PROMINENCE = 150      # Mindest-Prominenz für Extremum (Promille)
PEAK_DISTANCE   = 30       # Mindestabstand zwischen Extrema (Messpunkte = Minuten)
OUTLIER_LO      = 0.30     # Kapazitätswerte unter 30% der Nennkapazität = Ausreißer
OUTLIER_HI      = 1.50     # Kapazitätswerte über 150% der Nennkapazität = Ausreißer


def load_and_filter(path):
    print("Lade Daten...")
    df = pd.read_csv(path, low_memory=False, index_col=False)
    print(f"  Rohdaten: {len(df):,} Zeilen")

    # SoC == 0 oder fehlend → fehlende Messung
    df = df[df['solarakku_ladestand_in_promille'] != 0].copy()
    df = df[df['solarakku_ladestand_in_promille'].notna()].copy()
    df = df[df['solarakku_zuschuss_in_w'].notna()].copy()
    # datum == 0 oder fehlend → unvollständige Zeile
    df = df[df['datum'].notna()].copy()
    df = df[df['datum'].astype(str) != '0'].copy()
    # zeitpunkt == 0 → unvollständige Zeile
    df = df[df['zeitpunkt'] > 0].copy()
    df = df.sort_values('zeitpunkt').reset_index(drop=True)

    print(f"  Nach Filter: {len(df):,} Zeilen")
    print(f"  Zeitraum: {df['datum'].iloc[0]} bis {df['datum'].iloc[-1]}")
    return df


def find_peaks_valleys(arr, min_distance, min_prominence):
    """Peak/valley detection ohne scipy via pandas rolling."""
    s   = pd.Series(arr)
    w   = min_distance * 2 + 1
    w_p = min_distance * 12 + 1  # größeres Fenster für Prominenz-Prüfung

    roll_max      = s.rolling(w,   center=True, min_periods=1).max()
    roll_min      = s.rolling(w,   center=True, min_periods=1).min()
    roll_min_wide = s.rolling(w_p, center=True, min_periods=1).min()
    roll_max_wide = s.rolling(w_p, center=True, min_periods=1).max()

    # lokales Maximum UND Prominenz >= Schwelle
    peak_cand   = np.where((s >= roll_max - 0.01) & (s - roll_min_wide >= min_prominence))[0]
    valley_cand = np.where((s <= roll_min + 0.01) & (roll_max_wide - s >= min_prominence))[0]

    def dedup(indices, keep_max):
        if len(indices) == 0:
            return np.array([], dtype=int)
        result = [int(indices[0])]
        for idx in map(int, indices[1:]):
            if idx - result[-1] >= min_distance:
                result.append(idx)
            elif (keep_max and arr[idx] > arr[result[-1]]) or \
                 (not keep_max and arr[idx] < arr[result[-1]]):
                result[-1] = idx
        return np.array(result)

    return dedup(peak_cand, keep_max=True), dedup(valley_cand, keep_max=False)


def find_cycles(df):
    df['energie_wh'] = df['solarakku_zuschuss_in_w'] * SAMPLE_S / 3600.0

    soc_smooth = (
        df['solarakku_ladestand_in_promille']
        .rolling(SMOOTH_WINDOW, center=True, min_periods=1)
        .mean()
        .values
    )

    peaks, valleys = find_peaks_valleys(soc_smooth, PEAK_DISTANCE, PEAK_PROMINENCE)
    print(f"  Peaks: {len(peaks)}, Valleys: {len(valleys)}")

    extrema = sorted(
        [(int(i), 'peak',   soc_smooth[i]) for i in peaks] +
        [(int(i), 'valley', soc_smooth[i]) for i in valleys],
        key=lambda x: x[0]
    )

    # Aufeinanderfolgende gleiche Typen zusammenführen: behalte intensiveres
    merged = []
    for ex in extrema:
        if not merged or merged[-1][1] != ex[1]:
            merged.append(list(ex))
        else:
            if ex[1] == 'peak'   and ex[2] > merged[-1][2]:
                merged[-1] = list(ex)
            elif ex[1] == 'valley' and ex[2] < merged[-1][2]:
                merged[-1] = list(ex)

    cycles = []
    for i in range(len(merged) - 1):
        start_idx, start_type, start_soc = merged[i]
        end_idx,   end_type,   end_soc   = merged[i + 1]

        if start_type == end_type:
            continue

        delta_soc = abs(end_soc - start_soc)
        if delta_soc < MIN_SOC_SWING:
            continue

        segment    = df.iloc[start_idx:end_idx + 1]
        energie_wh = segment['energie_wh'].sum()

        # Plausibilität: Entladung (peak→valley) muss positive Energie liefern
        if start_type == 'peak'   and energie_wh <= 0:
            continue
        if start_type == 'valley' and energie_wh >= 0:
            continue

        kapazitaet_kwh = abs(energie_wh) / (delta_soc / 1000.0) / 1000.0

        if not (NOMINAL_KWH * OUTLIER_LO < kapazitaet_kwh < NOMINAL_KWH * OUTLIER_HI):
            continue

        mid_idx = (start_idx + end_idx) // 2
        cycles.append({
            'datum':          df.iloc[mid_idx]['datum'],
            'typ':            'entladung' if start_type == 'peak' else 'ladung',
            'energie_wh':     round(abs(energie_wh), 1),
            'kapazitaet_kwh': round(kapazitaet_kwh, 3),
        })

    print(f"  Gültige Halbzyklen: {len(cycles)}")
    return pd.DataFrame(cycles)


def aggregate_monthly(cycles_df):
    cycles_df = cycles_df.copy()
    cycles_df['datum'] = pd.to_datetime(cycles_df['datum'])

    monthly = (
        cycles_df
        .groupby(cycles_df['datum'].dt.to_period('M'))
        .agg(
            kapazitaet_median=('kapazitaet_kwh', 'median'),
            # Vollzyklus-Äquivalente: Entladeenergie / Nennkapazität
            entlade_wh=       ('energie_wh', lambda x:
                               x[cycles_df.loc[x.index, 'typ'] == 'entladung'].sum()),
        )
        .reset_index()
    )
    monthly['degradation_pct'] = (1.0 - monthly['kapazitaet_median'] / NOMINAL_KWH) * 100
    monthly['vollzyklen_kum']  = (monthly['entlade_wh'].cumsum() / (NOMINAL_KWH * 1000)).round(0).astype(int)
    return monthly


def print_highwater(monthly):
    """Nur Monate ausgeben, deren Wert von keinem späteren Monat übertroffen wird.
    Ergibt eine monoton fallende Sequenz = bereinigter Degradationsverlauf."""
    vals = monthly['kapazitaet_median'].values
    suffix_max = np.maximum.accumulate(vals[::-1])[::-1]
    mask = vals >= suffix_max - 0.001
    hw = monthly[mask].copy()

    hl = '+----------+------+----------+'
    print(f"\nDegradations-Verlauf (bereinigt, nur nicht überbotene Höchstwerte):")
    print(hl)
    print(f'| {"Monat":<8} | {"kWh":>4} | {"Verlust":>8} |')
    print(hl)
    for _, row in hw.iterrows():
        print(f'| {str(row["datum"]):<8} | {row["kapazitaet_median"]:>4.2f} | {row["degradation_pct"]:>+7.1f}% |')
    print(hl)


def print_yearly_cycles(monthly):
    monthly = monthly.copy()
    monthly['jahr'] = monthly['datum'].dt.year

    yearly = (
        monthly
        .groupby('jahr')['entlade_wh']
        .sum()
        .apply(lambda wh: wh / (NOMINAL_KWH * 1000))
    )

    hl = '+------+------------+'
    print(f"\nVollzyklen pro Jahr:")
    print(hl)
    print(f'| {"Jahr":^4} | {"Vollzyklen":>10} |')
    print(hl)
    for jahr, vz in yearly.items():
        print(f'| {jahr:^4} | {vz:>10.1f} |')
    print(hl)
    total = yearly.sum()
    print(f'  Gesamt: {total:.1f} Vollzyklen')


def print_projection(monthly):
    START_DATE  = datetime(2022, 12, 1)
    TARGET      = 6000
    today       = datetime.today()

    total_vz    = monthly['vollzyklen_kum'].iloc[-1]
    days_elapsed = (today - START_DATE).days
    rate_per_day = total_vz / days_elapsed
    remaining   = TARGET - total_vz
    days_left   = remaining / rate_per_day
    target_date = today + timedelta(days=days_left)
    years_left  = days_left / 365.25

    print(f"\nHochrechnung auf {TARGET} Vollzyklen:")
    print(f"  Inbetriebnahme:     {START_DATE.strftime('%d.%m.%Y')}")
    print(f"  Tage in Betrieb:    {days_elapsed} Tage")
    print(f"  Vollzyklen bisher:  {total_vz:.1f}")
    print(f"  Zyklen/Tag:         {rate_per_day:.3f}")
    print(f"  Noch fehlend:       {remaining:.1f} Zyklen")
    print(f"  Erreicht ca.:       {target_date.strftime('%d.%m.%Y')}  (in {years_left:.1f} Jahren)")


if __name__ == '__main__':
    df         = load_and_filter(INPUT_CSV)
    cycles_df  = find_cycles(df)
    monthly    = aggregate_monthly(cycles_df)

    print_highwater(monthly)
    print_yearly_cycles(monthly)
    print_projection(monthly)
