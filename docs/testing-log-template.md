# ARMOR Testing Log Template

Use this template to record every physical test session.  Copy the table into a
new file named by date (e.g. `testing-log-2025-07-15.md`) or append rows to a
shared log file.

Consistent records allow threshold calibration to be traced back to specific
firmware versions and test conditions.

---

> ⚠️ **Safety warning:** Use only **controlled movements** and **soft
> surfaces** when simulating fall-like events.  Do not deliberately perform
> real falls.  The prototype hardware is not designed to withstand impact
> and will not protect a person from injury.  All testing should be conducted
> in a safe environment with appropriate precautions.

---

## Test log

| Date | Git commit / firmware version | Motion threshold (g) | Test activity | Peak magnitude (g) | Sender alert sent | Receiver alert received | RSSI (dBm) | SNR (dB) | False positive | Missed event | Notes |
|------|-------------------------------|----------------------|---------------|--------------------|-------------------|-------------------------|------------|----------|----------------|--------------|-------|
|      |                               |                      |               |                    |                   |                         |            |          |                |              |       |
|      |                               |                      |               |                    |                   |                         |            |          |                |              |       |
|      |                               |                      |               |                    |                   |                         |            |          |                |              |       |

---

## Column definitions

| Column                    | How to fill it in                                                                 |
|---------------------------|-----------------------------------------------------------------------------------|
| Date                      | ISO date of the test session (YYYY-MM-DD)                                         |
| Git commit / firmware version | Short git commit hash or a descriptive firmware label                          |
| Motion threshold (g)      | Value of `MOTION_THRESHOLD_G` in the uploaded firmware                            |
| Test activity             | Brief description (e.g. "resting on desk", "quick arm shake", "dropped onto foam") |
| Peak magnitude (g)        | Highest magnitude value observed in Serial Monitor during the activity            |
| Sender alert sent         | Yes / No — did the sender LED turn red and transmit a LoRa packet?                |
| Receiver alert received   | Yes / No — did the receiver display and buzzer respond?                           |
| RSSI (dBm)                | RSSI value shown on receiver OLED or Serial Monitor at time of receipt            |
| SNR (dB)                  | SNR value shown on receiver OLED or Serial Monitor at time of receipt             |
| False positive            | Yes / No — was the alert triggered by non-fall motion?                            |
| Missed event              | Yes / No — did a fall-like event fail to trigger the alert?                       |
| Notes                     | Any additional observations, wiring issues, or environmental factors              |

---

## Example row

| Date       | Git commit / firmware version | Motion threshold (g) | Test activity              | Peak magnitude (g) | Sender alert sent | Receiver alert received | RSSI (dBm) | SNR (dB) | False positive | Missed event | Notes                        |
|------------|-------------------------------|----------------------|----------------------------|--------------------|-------------------|-------------------------|------------|----------|----------------|--------------|------------------------------|
| 2025-07-15 | a1b2c3d / v0.1                | 1.2                  | Quick downward arm swing   | 1.87               | Yes               | Yes                     | -62        | 9.5      | No             | No           | 2 m line-of-sight, indoors   |
| 2025-07-15 | a1b2c3d / v0.1                | 1.2                  | Resting on desk            | 1.02               | No                | No                      | —          | —        | No             | —            | Baseline noise check         |
