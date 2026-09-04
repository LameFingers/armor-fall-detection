# Evidence

This folder contains photographic and screenshot evidence of the ARMOR
prototype build, ML training progression, and on-device validation.

---

## Structure

| Folder | Contents |
|--------|----------|
| [`hardware/`](hardware/) | Breadboard build photos and close-up shots of both boards |
| [`ml-training/`](ml-training/) | Edge Impulse screenshots for all six training attempts plus final model configuration (dataset, impulse, spectral features, classifier) |
| [`on-device/`](on-device/) | Validation test results from running the model on real hardware |

---

## File naming convention

| Prefix | Meaning |
|--------|---------|
| `breadboard1–3-*` | Breadboard assembly and wiring photos |
| `receiver-*` / `sender-*` | Close-up shots of the finished receiver or sender board |
| `attempt1–6-training` | Edge Impulse training screenshot for that attempt |
| `ei-data` | Final dataset page — sample count, split, and labels |
| `ei-impulse` | Final impulse design — input block, processing block, learning block |
| `ei-features` | Final spectral features — DSP filter and FFT parameters |
| `ei-classifier` | Final classifier — architecture, training settings, and validation results |
| `validation-*` | On-device test result screenshots |
