markdown
# Public Transit Routing – Zurich Example

![CI](https://github.com/your-repo/public-transit-routing-zurich/actions/workflows/ci.yml/badge.svg)

## Disclaimer
**This repository contains an experimental backend service that is NOT part of the Organic Maps core.**  
It is provided solely as a proof‑of‑concept for public‑transit routing in Zurich. If you intend to use it in production, you should fork it into a separate GitHub repository and treat it as an independent service.

## Overview
This repository provides a minimal public‑transit routing service for the city of Zurich. It demonstrates how to:

1. **Build routing indices** from GTFS data.  
2. **Run the service** locally via Docker or directly with Python.  
3. **Execute the test suite** to validate API endpoints.

## Experiment Description & Scope
- **Goal:** Show how GTFS data can be transformed into a simple routing API that returns possible journeys between stops.
- **Scope:** The implementation covers a single city (Zurich) and a limited set of features (basic trip planning, no real‑time updates, no multi‑modal integration).  
- **Limitations:** It is not production‑grade, lacks scalability, security hardening, and does not integrate with the Organic Maps mobile or web clients.  

## Architecture Overview