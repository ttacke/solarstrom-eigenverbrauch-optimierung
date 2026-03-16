#!/usr/bin/env python3
"""Monitoring: Überlauf-Tage ab 2026 bei denen Verbraucher
nicht von der Steuerung aktiviert wurden.

Überlauf = SOC >= 99.5% UND Einspeisung > 50W.
Prüft verbraucher_automatisierung-Logs auf An-Signale vor dem Überlauf.

Verbraucher:
  - auto/roller: Laden (AnWeilGenug, FruehLeerenAn, winterLadenAn, force/winter Start)
  - wasser/heizung: Überladen (AnWeilGenug)
"""

import pandas as pd
import os
import glob

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTE_DIR = os.path.dirname(SCRIPT_DIR)  # scripte/
PROJECT_DIR = os.path.dirname(SCRIPTE_DIR)  # projekt-root/
CSV = os.path.join(SCRIPTE_DIR, "solar.csv")
LOG_DIR = os.path.join(PROJECT_DIR, "sd-karteninhalt")
START = "2026-03-13"

VERBRAUCHER = ("auto", "roller", "wasser", "heizung")

AN_SIGNALE = {
    "auto": [
        "auto>-solar>AnWeilGenug",
        "auto>-solar>FruehLeerenAn",
        "auto>-solar>winterLadenAn",
        "auto>-force>Start",
        "auto>-winter>Start",
        "auto>-winter>anWeilZeit",
    ],
    "roller": [
        "roller>-solar>AnWeilGenug",
        "roller>-solar>FruehLeerenAn",
        "roller>-solar>winterLadenAn",
        "roller>-force>Start",
        "roller>-winter>Start",
        "roller>-winter>anWeilZeit",
    ],
    "wasser": [
        "wasser>-ueberladen>AnWeilGenug",
    ],
    "heizung": [
        "heizung>-ueberladen>AnWeilGenug",
    ],
}

AUS_SIGNALE = {
    "auto": [
        "auto>-aus>AusWegenZielladestand",
        "auto>-aus>AusWegenZuWenigAkku",
        "auto>-ausAWZ",
        "auto>-solar>ausLastSchutz",
        "auto>-force>Ende",
        "auto>-solar>FruehLeerenAus",
        "auto>_laden_ist_beendet>AusWeilBeendet",
    ],
    "roller": [
        "roller>-aus>AusWegenZielladestand",
        "roller>-aus>AusWegenZuWenigAkku",
        "roller>-ausAWZ",
        "roller>-solar>ausLastSchutz",
        "roller>-force>Ende",
        "roller>-solar>FruehLeerenAus",
        "roller>_laden_ist_beendet>AusWeilBeendet",
    ],
    "wasser": [
        "wasser>-ueberladen>AusWeilAkkuVorhersage",
        "wasser>-ueberladen>AusWeilLadewunsch",
    ],
    "heizung": [
        "heizung>-ueberladen>AusWeilAkkuVorhersage",
        "heizung>-ueberladen>AusWeilLadewunsch",
    ],
}

# CSV-Spalten für "tatsächlich aktiv" (None = kein direktes Signal)
LADEN_SPALTE = {
    "auto": "auto_laden_ist_an",
    "roller": "roller_laden_ist_an",
    "wasser": "wasser_relay_ist_an",
    "heizung": "heizung_relay_ist_an",
}


def parse_logs():
    events = []
    log_files = sorted(glob.glob(os.path.join(LOG_DIR, "verbraucher_automatisierung-202*.log")))
    log_files = [f for f in log_files if ".bak." not in f]

    for lf in log_files:
        with open(lf) as fh:
            for line in fh:
                line = line.strip()
                if not line or ":" not in line:
                    continue
                parts = line.split(":", 1)
                try:
                    ts = int(parts[0])
                except ValueError:
                    continue
                msg = parts[1]

                for v in VERBRAUCHER:
                    if not msg.startswith(v):
                        continue
                    if msg in AN_SIGNALE[v]:
                        events.append({"ts_unix": ts, "verbraucher": v,
                                       "aktion": "an", "detail": msg})
                    elif msg in AUS_SIGNALE[v]:
                        events.append({"ts_unix": ts, "verbraucher": v,
                                       "aktion": "aus", "detail": msg})

    edf = pd.DataFrame(events)
    if len(edf) > 0:
        edf["ts"] = pd.to_datetime(edf["ts_unix"], unit="s")
        edf["date"] = edf["ts"].dt.date
    return edf


def verbraucher_status_am_tag(events, tag, ue_ts, td, verbraucher):
    v_events = events[(events["verbraucher"] == verbraucher) & (events["date"] == tag)]
    v_vor = v_events[v_events["ts"] < ue_ts].sort_values("ts")

    aktiviert = False
    erste_akt = None
    detail = None
    letzter = None
    for _, ev in v_vor.iterrows():
        if ev["aktion"] == "an":
            aktiviert = True
            if erste_akt is None:
                erste_akt = ev["ts"]
                detail = ev["detail"]
            letzter = "an"
        elif ev["aktion"] == "aus":
            letzter = "aus"

    laden_col = LADEN_SPALTE[verbraucher]
    vor_ue = td[td["ts"] < ue_ts]
    geladen = False
    if laden_col in vor_ue.columns:
        w = vor_ue[laden_col]
        geladen = bool(w.max() == 1) if w.notna().any() else False

    return {
        "aktiviert": aktiviert,
        "erste_akt": erste_akt,
        "detail": detail,
        "aktiv_bei_ue": letzter == "an",
        "geladen": geladen,
    }


def analyse():
    events = parse_logs()

    if len(events) == 0:
        print(f"Keine Steuerungs-Events gefunden in {LOG_DIR}")
        print("Prüfe ob verbraucher_automatisierung-202*.log Dateien vorhanden sind.")
        return

    df = pd.read_csv(CSV, index_col=False, low_memory=False)
    df = df[df["zeitpunkt"] != 0].copy()
    df["ts"] = pd.to_datetime(df["zeitpunkt"], unit="s")
    df["date"] = df["ts"].dt.date
    df["soc_pct"] = df["solarakku_ladestand_in_promille"] / 10

    df = df[df["ts"] >= START]
    if len(df) == 0:
        print(f"Keine Daten ab {START}.")
        return

    ueberlauf_mask = (df["soc_pct"] >= 99.5) & (df["netzbezug_in_w"] < -50)
    ueberlauf_tage = sorted(df.loc[ueberlauf_mask, "date"].unique())

    if not ueberlauf_tage:
        print(f"Keine Überlauf-Tage ab {START}.")
        return

    print(f"Daten: {df['ts'].min().date()} bis {df['ts'].max().date()}")
    print(f"Überlauf-Tage ab {START}: {len(ueberlauf_tage)}")
    print()

    probleme = []

    for tag in ueberlauf_tage:
        td = df[df["date"] == tag].sort_values("ts")
        erste_ue = td[ueberlauf_mask[td.index]]
        if len(erste_ue) == 0:
            continue
        ue_ts = erste_ue.iloc[0]["ts"]
        minuten_einspeisung = int(ueberlauf_mask[td.index].sum())

        soc_40 = td[td["soc_pct"] > 40]
        soc_40_ts = soc_40.iloc[0]["ts"] if len(soc_40) > 0 else None

        status = {}
        for v in VERBRAUCHER:
            status[v] = verbraucher_status_am_tag(events, tag, ue_ts, td, v)

        alle_ok = all(status[v]["aktiviert"] for v in VERBRAUCHER)

        if not alle_ok:
            probleme.append({
                "tag": tag,
                "ueberlauf_zeit": ue_ts.strftime("%H:%M"),
                "soc_40_zeit": soc_40_ts.strftime("%H:%M") if soc_40_ts else "---",
                "einspeisung_min": minuten_einspeisung,
                "status": status,
            })

    # ── Ausgabe ──
    ok_tage = len(ueberlauf_tage) - len(probleme)
    print(f"Alle 4 Verbraucher aktiviert: {ok_tage}/{len(ueberlauf_tage)} Tage")
    print()

    if not probleme:
        print("Keine Lücken — Steuerung hat an allen Überlauf-Tagen alle Verbraucher aktiviert.")
        return

    print(f"Lücken: {len(probleme)} Tage")
    print()

    for p in probleme:
        print(f"  {p['tag']}  Überlauf {p['ueberlauf_zeit']}  SOC>40% {p['soc_40_zeit']}  "
              f"Einspeisung: {p['einspeisung_min']}min")

        for v in VERBRAUCHER:
            s = p["status"][v]
            sym = "✓" if s["aktiviert"] else "✗"

            if s["aktiviert"]:
                geladen = "aktiv" if s["geladen"] else "nicht aktiv"
                zeit = s["erste_akt"].strftime("%H:%M")
                print(f"    {v:<8} [{sym}] {zeit} ({geladen})")
            else:
                print(f"    {v:<8} [{sym}] NICHT AKTIVIERT")

        print()


if __name__ == "__main__":
    analyse()
