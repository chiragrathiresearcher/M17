# K-Framework (Project M17): Physics-Invariant Satellite Auditing
**Version:** 1.0.0  
**Status:** Research Grade  
**Primary Objective:** Detection of anomalous behavior in decommissioned orbital assets via High-Fidelity Physics Filtering.

---

## 🌌 1. Executive Summary
The **K-Framework**, internally designated as **Project M17**, is an automated analytical pipeline designed to audit the "death state" of decommissioned satellites. While official registries may list an asset as retired, decommissioned, or non-functional, M17 treats these labels as hypotheses rather than facts. 

By ingesting raw Two-Line Element (TLE) data and processing it through a multi-layered physics engine, the framework detects deviations from expected passive orbital decay. If a "dead" satellite maintains an orbital state that violates Newtonian constraints for unpowered bodies—such as maintaining a constant altitude or circularizing its orbit without external influence—the system flags the asset as a **"Zombie Satellite."**



---

## 🛠 2. System Architecture & Methodology

The framework is built on a modular "K-Series" architecture, ensuring that data ingestion, physical computation, and scientific memory remain decoupled yet synchronized.

### 🛰 K20: The Physics Core
The **K20 Engine** serves as the ground truth generator. It utilizes the SGP4 (Simplified General Perturbations) model to propagate TLE data into Cartesian state vectors (Position/Velocity). 
* **Multi-Body Simulation:** Capable of simulating Newtonian gravity across multiple celestial bodies.
* **State Mapping:** Converts raw TLE strings into highly precise `OrbitalState` objects, calculating eccentricity, inclination, and semi-major axis.
* **Invariant Checking:** Establishes the "Passive Baseline"—the path a satellite *must* follow if it has no active propulsion.

### 📉 K19: Uncertainty Quantification (UQ)
In orbital mechanics, a measurement without uncertainty is noise. The **K19 Engine** ensures scientific integrity by applying **Rule 1: No value exists without uncertainty.**
* **Propagation Methods:** Supports Jacobian-based analytical propagation and high-iteration Monte Carlo simulations.
* **Trust-Aware Inflation:** If data is sourced from low-confidence ground radar, K19 automatically inflates the uncertainty covariance to prevent "False Positive" anomaly detections.

### 🧠 K21: Scientific Memory & Case Evolution
Unlike standard logging systems, **K21** manages "Scientific Cases." It tracks the life cycle of an audit.
* **State Transitions:** Cases move through states: `CONSISTENT` → `TENSION` → `ANOMALOUS`.
* **Epistemic Judgments:** Records *why* a satellite was flagged, preserving the logic for peer review.
* **Persistence:** All anomalies are recorded in the `k21_registry.json`, creating a historical record of "Zombie" behavior over months of observation.

---

## 📊 3. Data Ingestion & Trust Modeling

Project M17 utilizes the `observation_ingestion.py` module to sanitize incoming data. Not all observations are equal. The system applies an **Observation Trust Model** to weight incoming data based on the source instrument.

| Instrument Type | Precision | Reliability | Weighting Factor |
| :--- | :--- | :--- | :--- |
| **GPS (Onboard)** | 0.99 | 0.99 | 1.0 |
| **HST (Hubble)** | 0.95 | 0.98 | 0.9 |
| **Ground Radar** | 0.75 | 0.85 | 0.6 |
| **Default/Other** | 0.80 | 0.80 | 0.7 |



---

## 📂 4. Output Structures

The framework generates high-density JSON reports. These files are designed to be ingested by visualization tools or used as the basis for published research papers.

### **Sample Audit Log (`data/outputs/satellite_audit_log.json`)**
This output represents a single pass of the M17 filter on a specific NORAD asset.

```json
{
  "audit_meta": {
    "timestamp": "2026-03-08T17:15:00Z",
    "framework_version": "1.0.0-M17",
    "engine": "K20-Physics-Core"
  },
  "asset_identity": {
    "norad_id": "25994",
    "name": "TERRA",
    "launch_year": 1999,
    "official_status": "Decommissioned"
  },
  "physics_results": {
    "current_eccentricity": 0.00012,
    "expected_decay_rate": "-0.005 km/day",
    "actual_decay_rate": "+0.001 km/day",
    "anomalous_thrust_detected": true,
    "zombie_score": 0.88
  },
  "uncertainty_analysis": {
    "method": "Monte Carlo",
    "samples": 10000,
    "confidence_interval": "95%",
    "sigma_deviation": 4.1
  },
  "k21_verdict": {
    "current_state": "ANOMALOUS",
    "previous_state": "TENSION",
    "justification": "Satellite is counteracting atmospheric drag without documented propulsion capability."
  }
}
