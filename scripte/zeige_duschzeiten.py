# apt install python3-matplotlib
import matplotlib.pyplot as plt
import datetime as dt
# import pandas as pd

timestamps = []
with open("feuchtigkeits_peaks.csv") as file:
    while line := file.readline():
        timestamps.append(line.rstrip())

dates = [dt.datetime.strptime(t, "%Y-%m-%d %H:%M") for t in timestamps]

# Wochentag (0=Montag, 6=Sonntag) und Uhrzeit in Dezimalstunden
x = [d.weekday() for d in dates]
y = [d.hour + d.minute / 60 for d in dates]

# Plot
plt.figure(figsize=(10, 6))
plt.scatter(x, y, color="royalblue", s=70, alpha=0.7)
plt.xticks(range(7), ["Mo", "Di", "Mi", "Do", "Fr", "Sa", "So"])
plt.yticks(range(0, 25, 2))
plt.xlabel("Wochentag")
plt.ylabel("Uhrzeit (Stunden)")
plt.title("Zeitpunkte auf einem Wochen-Zeitstrahl (Mo–So)")
plt.grid(True, linestyle="--", alpha=0.5)
plt.ylim(0, 24)
plt.show()
