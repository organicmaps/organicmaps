# routing_service

## Overview
This repository provides a routing service built on OpenStreetMap (OSM) data.  
It includes tools to generate routing indices, a Go‑based HTTP API, and a Python test suite that validates the API.

> **Note:** The source code now lives in the `src` directory (previously named `job_github_bounty_organicmaps_organicmaps#5331`).  

---

## 1️⃣ Build Routing Indices (Zurich example)

### Prerequisites
- **Go 1.22+** (for the index‑building binary)  
- **wget** or **curl** (to fetch OSM extracts)  
- **7‑zip** or **tar** (to extract `.pbf` files)  
- **Docker** (optional, for running the service)  

### Steps

1. **Create a working directory**