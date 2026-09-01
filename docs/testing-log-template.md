# ARMOR Testing Log Template

Use this template to record physical test sessions with the deployed TinyML
firmware. Copy the table into a new file named by date (e.g.
`testing-log-2025-07-20.md`) or append rows to a shared log file.

---

> ⚠️ **Safety warning:** Use only **controlled movements** and **soft
> surfaces** when simulating fall-like events. Do not deliberately perform
> real falls. The prototype hardware is not designed to withstand impact
> and will not protect a person from injury.

---

## Test log

| Date | Activity | Type | Fall-like confidence | Alert triggered | Receiver received | RSSI (dBm) | SNR (dB) | False positive | Missed detection | Notes |
|------|----------|------|----------------------|-----------------|-------------------|------------|----------|----------------|-----------------|-------|
|      |          |      |                      |                 |                   |            |          |                |                 |       |
|      |          |      |                      |                 |                   |            |          |                |                 |       |

---

## Column definitions

| Column | How to fill it in |
|--------|------------------|
| Date | ISO date of the test session (YYYY-MM-DD) |
| Activity | Brief description (e.g. "falling backwards", "fast squat", "sitting down") |
| Type | `fall_like` or `non_fall` — the expected classification |
| Fall-like confidence | Score shown on sender OLED (`FallRisk: X%`) at the moment of trigger, or peak score observed |
| Alert triggered | Yes / No — did the sender LED turn red and enter the countdown? |
| Receiver received | Yes / No — did the receiver display and buzzer respond? |
| RSSI (dBm) | RSSI shown on receiver OLED or Serial Monitor |
| SNR (dB) | SNR shown on receiver OLED or Serial Monitor |
| False positive | Yes / No — was an alert triggered by a non-fall activity? |
| Missed detection | Yes / No — did a fall-like event fail to trigger the alert? |
| Notes | Any observations, wiring issues, or environmental factors |

---

## Example rows

| Date | Activity | Type | Fall-like confidence | Alert triggered | Receiver received | RSSI (dBm) | SNR (dB) | False positive | Missed detection | Notes |
|------|----------|------|----------------------|-----------------|-------------------|------------|----------|----------------|-----------------|-------|
| 2025-07-20 | Falling backwards | fall_like | 94% | Yes | Yes | -58 | 11.2 | No | No | Indoors, 2 m line-of-sight |
| 2025-07-20 | Sitting down slowly | non_fall | 12% | No | No | — | — | No | — | No false positive |
| 2025-07-20 | Fast squat | non_fall | 61% | No | No | — | — | No | — | Below threshold; consecutive window gate held |
