markdown
# Public Transit Routing – Zurich Example

![CI](https://github.com/your-repo/public-transit-routing-zurich/actions/workflows/ci.yml/badge.svg)

## Overview

This repository provides a minimal public‑transit routing service for Zurich. The README walks you through:

1. **Building routing indices** using GTFS data for Zurich.  
2. **Running the service locally** with Docker or directly via Python.  
3. **Executing the test suite** that validates the API endpoints.

---

## 1. Build Routing Indices

### Prerequisites
- Python 3.11+  
- `pip` (or `uv`)  
- `docker` (optional, for containerised execution)  
- `wget` or `curl` (to download GTFS data)  

### Step‑by‑step