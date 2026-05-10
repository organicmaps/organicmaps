# Public Transit Routing – Zurich Example

## Overview

This repository contains a minimal public‑transit routing service. The README below walks you through:

1. **Building routing indices** (using open‑source GTFS data for Zurich)  
2. **Running the service locally** (Docker + Python)  
3. **Executing Python tests** that call the API (sample request/response)

---

## 1. Build Routing Indices

### Prerequisites
- Python 3.11+  
- `pip` (or `uv`)  
- `docker` (optional, for containerised execution)  
- `wget` or `curl` (to download GTFS data)

### Step‑by‑step