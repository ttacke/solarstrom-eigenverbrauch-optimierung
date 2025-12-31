import pandas as pd
import plotly.express as px

def erstelle_interaktives_diagramm(csv_datei):
    # Daten einlesen
    df = pd.read_csv(csv_datei)
    df['Datum'] = pd.to_datetime(df['Datum'])
    df = df.sort_values('Datum')

    # Diagramm erstellen (Interaktivität ist hier Standard)
    # Wir schmelzen das DataFrame, damit Plotly alle 3 Spalten einfach erkennt
    fig = px.line(df, x='Datum', y=['L1', 'L2', 'L3', 'WPan', 'Abluft', 'Zuluft', 'LuftHeizAn', 'LuftHeiz'],
                  title='Interaktive Zeitreihe (Nutze die Maus zum Zoomen)',
                  labels={'value': 'Messwert', 'variable': 'Spalte'})

    # Grafik im Browser öffnen
    fig.show()

if __name__ == "__main__":
    # Ersetze 'daten.csv' durch deinen Dateinamen
    erstelle_interaktives_diagramm('3phasiger_lastverlauf_detail.csv')
