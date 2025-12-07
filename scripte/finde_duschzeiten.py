# apt install python3-matplotlib
import matplotlib.pyplot as plt
import datetime as dt

# Liste der Zeitpunkte > in Perl so ausgeben
timestamps = [
"2025-11-08 17:12",
"2025-11-08 21:00",
"2025-11-09 15:44",
"2025-11-09 18:29",
"2025-11-09 21:41",
"2025-11-10 20:30",
"2025-11-10 21:32",
"2025-11-11 20:11",
"2025-11-12 0:04",
"2025-11-12 20:36",
"2025-11-12 22:00",
"2025-11-13 12:11",
"2025-11-13 20:59",
"2025-11-14 0:05",
"2025-11-14 20:50",
"2025-11-14 22:07",
"2025-11-15 22:15",
"2025-11-16 10:57",
"2025-11-16 19:56",
"2025-11-16 21:11",
"2025-11-16 22:40",
"2025-11-17 7:11",
"2025-11-17 19:09",
"2025-11-17 21:21",
"2025-11-18 20:02",
"2025-11-19 0:48",
"2025-11-19 20:54",
"2025-11-19 23:35",
"2025-11-20 6:51",
"2025-11-20 20:43",
"2025-11-20 23:11",
"2025-12-01 1:00",
"2025-12-01 9:55",
"2025-12-01 20:02",
"2025-12-01 21:02",
"2025-12-01 23:23",
"2025-12-02 19:20",
"2025-12-02 21:56",
]

# Umwandeln in datetime-Objekte
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
