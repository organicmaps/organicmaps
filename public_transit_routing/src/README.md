# job_github_bounty_organicmaps

## Overview
This repository provides a routing service built on OpenStreetMap (OSM) data.  
It includes tools to generate routing indices, a Go‑based HTTP API, and a Python test suite that validates the API.

---

## 1️⃣ Build Routing Indices (Zurich example)

### Prerequisites
- **Go 1.22+** (for the index‑building binary)  
- **wget / curl** (to fetch OSM extracts)  
- **7‑zip** (or any tool that can extract `.pbf` files)

### Step‑by‑step

1. **Create a working directory**