# ARMOR Project Evidence

This folder stores selected evidence that documents the physical build,
testing, and demonstration of the ARMOR prototype.

---

## What belongs here

| Evidence type | Subfolder | Description |
|---------------|-----------|-------------|
| Wiring photos | `evidence/` (root) | Photographs of the breadboard wiring for both sender and receiver |
| Prototype photos | `evidence/` (root) | Assembly photos showing the complete hardware setup |
| OLED screenshots / photos | `evidence/` (root) | Photos of the OLED display during monitoring and alert states |
| Serial Monitor screenshots | `evidence/` (root) | Captures of Serial Monitor output showing sensor readings, alerts, and packet reception |
| Test data CSV files | `evidence/` (root) | Exported sensor data recorded during controlled test sessions |
| **Edge Impulse ML screenshots** | **`evidence/ml-screenshots/`** | Screenshots of the Edge Impulse development process (see below) |
| Links to demonstrations | `evidence/` (root) | Links to externally hosted video demonstrations (e.g. YouTube, OneDrive) |

---

## What does NOT belong here

- **Large raw video files** — do not commit video files directly to this Git
  repository.  Videos increase repository size significantly and cannot be
  diffed.  Host videos externally (e.g. YouTube, university OneDrive) and
  add a link in this README or the relevant documentation file.
- Binary files larger than a few MB — keep the repository lightweight.

---

## Edge Impulse ML screenshots (`evidence/ml-screenshots/`)

This subfolder stores screenshots of the Edge Impulse project taken during
model development. Capture and save these directly from your browser.

Recommended screenshots to include:

| Screenshot | What to capture |
|------------|-----------------|
| Data collection summary | The dataset overview page showing sample counts per label (`fall_like`, `non_fall`) |
| Data explorer | The 2D/3D feature scatter plot showing class separation |
| Impulse design | The full impulse pipeline (input block → processing block → learning block) |
| Spectral features | The spectral analysis configuration and example feature output |
| Training results | The training accuracy and loss curves |
| Confusion matrix | The confusion matrix from the model testing tab |
| Model testing output | The per-sample test results page |
| Live classification (optional) | Any on-device live classification output captured via Serial Monitor |

Save each file into `evidence/ml-screenshots/` using the naming convention
described below.

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

Hardware evidence and ML screenshots are in progress. Add files to the
appropriate subfolder as testing and documentation proceeds.
