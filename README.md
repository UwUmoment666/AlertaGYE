# AlertaGYE

**Flood early warning for Guayaquil: a live risk map that combines rain forecast, river tide and street reports to show where it may flood and how to get around it.**

Built for **Hack2Impact 2026** – Guayaquil heavy-rain challenge.

   🔗 **Live demo:** [uwumoment666.github.io/AlertaGYE](https://uwumoment666.github.io/AlertaGYE/)

## Features

- **Flood risk map:** 10 flood-prone points drawn as green / yellow / red zones on a Leaflet + OpenStreetMap map.
- **Rain forecast:** hourly rain for the next 12 hours from Open-Meteo, refreshed every 30 minutes.
- **Tide data:** today's official INOCAR tide table for Guayaquil (Río Guayas), with the current height estimated between high and low tide.
- **Simulated water-level sensor** at Paso elevado Florida Norte (km 8 vía a Daule). Its level rises with rain and drains more slowly at high tide. It is clearly labeled as simulated.
- **Alerts:** an on-screen panel appears when any zone turns red, showing the time and the reason.
- **Safe routes:** click an origin and a destination. The app asks OSRM for driving routes and recommends the one that crosses the fewest high-risk zones.
- **Citizen reports:** mark flooding on the map as ankle-deep, knee-deep or impassable. Impassable reports make nearby routes riskier.
- **Storm demo button:** replaces the real forecast with heavy rain so you can see the alerts and routing react.
- **Sensor firmware prototype:** ESP32 + ultrasonic sensor, simulated in [Wokwi](https://wokwi.com/projects/478980965654087681). Code in `wokwi-sensor/`.

## How the risk score works

Every few seconds, each zone gets a score:

1. **Rain:** the more rain expected in the next 3 hours, the more points, up to 50 (reached at 20 mm).
2. **Tide:** +25 points when the river is above 4.0 m, because high water blocks the city's drains.
3. **Vulnerability:** places with a worse flooding history count more. Rain + tide points are multiplied by 0.8 (low), 1.0 (medium) or 1.2 (high).
4. **Sensor:** where there is a sensor, measured water adds up to 40 points (reached at 30 cm).

| Score | Color | Meaning |
|---|---|---|
| under 30 | 🟢 Green | Normal conditions |
| 30–59 | 🟡 Yellow | Watch: streets could flood |
| 60 or more | 🔴 Red | Warning: flooding likely, avoid the area (triggers an alert) |

**Routes** get +3 for each red zone they cross, +1 for each yellow zone, and +2 for each "impassable" citizen report within 200 m. The lowest score is recommended (bold blue line).

All numbers live in one place (`RISK` and `ROUTING` in `index.html`), so they are easy to explain and tune.

## Data sources

- **Flood points:** local news reports (Primicias, Expreso, Extra, El Diario). Locations are **approximate**, and each point links to its source.
- **Tides:** INOCAR, *Tabla de Mareas 2026*, Tabla II: Guayaquil (Río Guayas).
- **Rain:** [Open-Meteo](https://open-meteo.com/) forecast API (CC BY 4.0).

## Third-party libraries and services

- [Leaflet](https://leafletjs.com/) 1.9.4: interactive map
- [OpenStreetMap](https://www.openstreetmap.org/copyright): map tiles and data (© OpenStreetMap contributors)
- [OSRM](https://project-osrm.org/) public demo server: driving routes
- [Open-Meteo](https://open-meteo.com/): rain forecast

## AI tools used

- **Claude** (Anthropic): code generation and drafting this README.

## How to run

It is a single `index.html` file with no build step:

- open it directly in a browser, or
- publish the repository with GitHub Pages.

## Team

Ney Gustavo Ruilova Romero
Milena Paulette Andrade Sanchez

## Limitations and future work

- **Sensor is simulated.** Next step: connect real low-cost water-level sensors in critical spots.
- **Reports are stored only on the user's device** (`localStorage`) and never expire. Next step: a shared backend so everyone sees the same reports, with automatic expiry.
- **Alerts only show on screen.** Next step: push alerts by SMS or Telegram to people near a red zone.
- **Tide table is for one day** and must be updated daily. Flood point locations need field verification.
- **The OSRM demo server** is for testing only and sometimes returns a single route. A real deployment needs its own routing server.
