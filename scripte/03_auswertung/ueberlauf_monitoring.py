#!/usr/bin/env python3
"""Monitoring: Überlauf-Tage ab 2026 bei denen Verbraucher
nicht von der Steuerung aktiviert wurden.
Prüft verbraucher_automatisierung-Logs auf An-Signale vor dem Überlauf.

Verbraucher:
  - auto/roller: Laden (AnWeilGenug, FruehLeerenAn, winterLadenAn, force/winter Start)
  - wasser/heizung: Überladen (AnWeilGenug)
  - begleitheizung/heizstab: Schalten (erlaubt/aus)
"""

import pandas as pd
import os
import glob
from astral import LocationInfo          # pip install astral
from astral.sun import sun as astral_sun

LAT, LON = 50.00, 7.95  # Koordinaten aus zentrale/api/wettervorhersage_api.h

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTE_DIR = os.path.dirname(SCRIPT_DIR)  # scripte/
PROJECT_DIR = os.path.dirname(SCRIPTE_DIR)  # projekt-root/
CSV = os.path.join(SCRIPTE_DIR, "solar.csv")
LOG_DIR = os.path.join(PROJECT_DIR, "sd-karteninhalt")
START = "2026-03-13"
UEBERLAUF_SOC = 99.5
UEBERLAUF_MIN_EINSPEISUNG = -50

VERBRAUCHER = ("auto", "roller", "wasser", "heizung", "begleitheizung", "heizstab")

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
    "begleitheizung": [
        "schalte wasser begleitheizung: >erlaubt",
    ],
    "heizstab": [
        "schalte heizstab: >erlaubt",
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
    "begleitheizung": [
        "schalte wasser begleitheizung: >aus",
    ],
    "heizstab": [
        "schalte heizstab: >aus",
    ],
}

# CSV-Spalten für "tatsächlich aktiv" (None = kein direktes Signal)
LADEN_SPALTE = {
    "auto": "auto_laden_ist_an",
    "roller": "roller_laden_ist_an",
    "wasser": "wasser_relay_ist_an",
    "heizung": "heizung_relay_ist_an",
    "begleitheizung": "begleitheizung_an",
    "heizstab": "heizungsunterstuetzung_an",
}

# Präfix für startswith-Prüfung im Log-Parser (je nach Log-Format)
LOG_STARTSWITH = {
    "auto": "auto>",
    "roller": "roller>",
    "wasser": "wasser>",
    "heizung": "heizung>",
    "begleitheizung": "schalte wasser begleitheizung",
    "heizstab": "schalte heizstab",
}


def _sonnenzeiten(datum):
    """Gibt (aufgang, untergang) als pd.Timestamp zurück."""
    loc = LocationInfo(latitude=LAT, longitude=LON, timezone="Europe/Berlin")
    s = astral_sun(loc.observer, date=datum, tzinfo=loc.timezone)
    aufgang   = pd.Timestamp(s["sunrise"]).tz_localize(None)
    untergang = pd.Timestamp(s["sunset"]).tz_localize(None)
    return aufgang, untergang


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
                    if not msg.startswith(LOG_STARTSWITH[v]):
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


def verbraucher_status_am_tag(events, tag, ue_ts, td, verbraucher, aufgang, untergang):
    # Alle Events des Verbrauchers im Tageslicht-Fenster
    v_events = events[
        (events["verbraucher"] == verbraucher) &
        (events["date"] == tag) &
        (events["ts"] >= aufgang) &
        (events["ts"] <= untergang)
    ].sort_values("ts")

    alle_events = [(ev["ts"], ev["aktion"], ev["detail"]) for _, ev in v_events.iterrows()]

    # Aktiviert = An-Signal im Tageslicht-Fenster VOR dem Überlauf
    aktiviert = any(
        aktion == "an" and ts < ue_ts
        for ts, aktion, _ in alle_events
    )

    laden_col = LADEN_SPALTE[verbraucher]
    vor_ue = td[td["ts"] < ue_ts]
    geladen = False
    if laden_col in vor_ue.columns:
        w = vor_ue[laden_col]
        geladen = bool(w.max() == 1) if w.notna().any() else False

    return {
        "aktiviert": aktiviert,
        "events": alle_events,
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

    if not UEBERLAUF_MIN_EINSPEISUNG:
        ueberlauf_mask = df["soc_pct"] >= UEBERLAUF_SOC
    else:
        ueberlauf_mask = (df["soc_pct"] >= UEBERLAUF_SOC) & (df["netzbezug_in_w"] < UEBERLAUF_MIN_EINSPEISUNG)
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

        aufgang, untergang = _sonnenzeiten(tag)

        status = {}
        for v in VERBRAUCHER:
            status[v] = verbraucher_status_am_tag(events, tag, ue_ts, td, v, aufgang, untergang)

        alle_ok = all(status[v]["aktiviert"] for v in VERBRAUCHER)

        if not alle_ok:
            probleme.append({
                "tag": tag,
                "ue_ts": ue_ts,
                "ueberlauf_zeit": ue_ts.strftime("%H:%M"),
                "soc_40_zeit": soc_40_ts.strftime("%H:%M") if soc_40_ts else "---",
                "einspeisung_min": minuten_einspeisung,
                "aufgang": aufgang.strftime("%H:%M"),
                "untergang": untergang.strftime("%H:%M"),
                "status": status,
            })

    # ── Ausgabe ──
    ok_tage = len(ueberlauf_tage) - len(probleme)
    n = len(VERBRAUCHER)
    print(f"Alle {n} Verbraucher aktiviert: {ok_tage}/{len(ueberlauf_tage)} Tage")
    print()

    if not probleme:
        print("Keine Lücken — Steuerung hat an allen Überlauf-Tagen alle Verbraucher aktiviert.")
        return

    print(f"Lücken: {len(probleme)} Tage")
    print()

    for p in probleme:
        print(f"  {p['tag']}  Überlauf {p['ueberlauf_zeit']}  SOC>40% {p['soc_40_zeit']}  "
              f"Einspeisung: {p['einspeisung_min']}min  "
              f"Tageslicht: {p['aufgang']}–{p['untergang']}")

        for v in VERBRAUCHER:
            s = p["status"][v]
            sym = "✓" if s["aktiviert"] else "✗"
            geladen = "(aktiv)" if s["geladen"] else "(nicht aktiv)"
            print(f"    {v:<15} [{sym}] {geladen}")
            for ts, aktion, detail in s["events"]:
                marker = "<-- Überlauf" if ts >= p["ue_ts"] else ""
                print(f"      {ts.strftime('%H:%M')}  {'an ' if aktion == 'an' else 'aus'}  {detail}  {marker}")
            if not s["events"]:
                print(f"      (keine Signale im Tageslicht-Fenster)")

        print()


if __name__ == "__main__":
    analyse()
