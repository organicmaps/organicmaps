# Public Transit Routing

A lightweight, high‑performance public‑transit routing service powered by the **RAPTOR** algorithm.  
The service can ingest GTFS data (e.g., Zurich Open Data) and OSM road networks, build routing indices, and expose a simple HTTP API for journey planning.

---

## Table of Contents
1. [Prerequisites](#prerequisites)  
2. [Building Routing Indices](#building-routing-indices)  
3. [Running the Service Locally](#running-the-service-locally)  
4. [API Overview](#api-overview)  
5. [Running Tests & Sample Calls](#running-tests--sample-calls)  
6. [Docker Compose Details](#docker-compose-details)  
7. [Contributing](#contributing)  
8. [License](#license)  

---

## Prerequisites
| Tool | Version | Why |
|------|---------|-----|
| **Docker** | ≥ 20.10 | Container runtime |
| **Docker‑Compose** | ≥ 2.0 | Orchestrates multi‑container setup |
| **Python** | 3.11 (inside container) | RAPTOR implementation |
| **Git** | any | Clone repository |
| **wget / curl** | any | Fetch data sources |
| **make** (optional) | any | Shortcut for common commands |

> **Note:** All heavy lifting (RAPTOR engine, data processing) runs inside Docker containers, so you only need Docker & Docker‑Compose on your host.

---

## Building Routing Indices

The routing engine requires two data sources:

1. **GTFS** – transit schedules, routes, stops, etc.  
2. **OSM** – road network for walking / biking transfers.

Below is a step‑by‑step example using **Zurich Open Data** (GTFS) and the corresponding OSM extract.