# ARMOR Project Evidence

This folder stores selected evidence that documents the physical build,
testing, and demonstration of the ARMOR prototype.

---

## What belongs here

| Evidence type | Description |
|---------------|-------------|
| Wiring photos | Photographs of the breadboard wiring for both sender and receiver |
| Prototype photos | Assembly photos showing the complete hardware setup |
| OLED screenshots / photos | Photos of the OLED display during monitoring and alert states |
| Serial Monitor screenshots | Captures of Serial Monitor output showing sensor readings, alerts, and packet reception |
| Test data CSV files | Exported sensor data recorded during controlled test sessions |
| Links to demonstrations | Links to externally hosted video demonstrations (e.g. YouTube, OneDrive) |

---

## What does NOT belong here

- **Large raw video files** — do not commit video files directly to this Git
  repository. Videos increase repository size significantly and cannot be
  diffed. Host videos externally (e.g. YouTube, university OneDrive) and
  add a link in this README or the relevant documentation file.
- Binary files larger than a few MB — keep the repository lightweight.

---

## File naming convention

Use descriptive, date-prefixed filenames for traceability:

```
YYYY-MM-DD_description.jpg
YYYY-MM-DD_description.png
YYYY-MM-DD_description.csv
```

Example:
```
2025-07-15_sender-breadboard-wiring.jpg
2025-07-15_receiver-oled-alert.jpg
2025-07-15_serial-monitor-fall-alert.png
2025-07-15_test-session-01.csv
```

---

## Current status

Add evidence files as testing and documentation proceeds.
