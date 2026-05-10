# Public Transit Routing Service

## Overview
The **public_transit_routing** package provides a lightweight HTTP API for computing public‑transit routes.  
It ships with a data‑processing pipeline that can ingest open‑source GTFS (General Transit Feed Specification) feeds, build routing indices, and expose a fast query endpoint.

> **Note:** The original source folder name `job_github_bounty_organicmaps_organicmaps#5331` was renamed to a simpler, more conventional layout (`public_transit_routing`).  

---

## Prerequisites
| Tool | Minimum version |
|------|-----------------|
| Python | 3.9 |
| pip   | 22.0 |
| Docker (optional) | 20.10 |
| `uvicorn` (ASGI server) | 0.24 |
| `networkx` | 3.2 |
| `gtfs-kit` | 5.0 |
| `requests` (for tests) | 2.31 |