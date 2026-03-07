# Project M17: K-Framework
### *Research-Grade Satellite Audit & Anomaly Detection System*

Project M17 is a specialized scientific pipeline designed to audit decommissioned NASA satellites (e.g., TERRA, AQUA, LANDSAT) for "residual functionality." By integrating multi-body orbital mechanics with rigorous uncertainty quantification, the framework identifies "Zombie" satellites—assets that are officially dead but show orbital behavior inconsistent with passive debris.



---

## 🏗 System Architecture

The project is built on the **K-Series Modular Logic**, ensuring a strict separation between data acquisition, physical simulation, and scientific memory.

### Core Modules
* **`run_research.py`**: The central entry point. Manages the research lifecycle via CLI.
* **`src/workflow.py`**: The orchestrator. Connects the physics engine to the uncertainty models.
* **`src/k19_uncertainty.py`**: **The Uncertainty Engine.** Enforces *Rule 1*: No value exists without uncertainty. Uses Monte Carlo propagation.
* **`src/k20_physics.py`**: **The Physics Core.** Handles SGP4 orbital propagation and multi-body simulations.
* **`src/k21_memory.py`**: **Scientific Memory.** A persistent registry that tracks the "state" of a satellite case without deleting history.
* **`src/satellite_bridge.py`**: Interface for Celestrak and Space-Track APIs.

---

## 🧪 Mathematical Methodology

The K-Framework operates on the principle of **Trust-Aware Propagation**. Unlike standard filters that may discard noisy data, M17 "inflates" uncertainty to preserve potential anomalies.

### 1. Uncertainty Quantification (K19)
We use a **Monte Carlo (MC) Propagation** method. For any input $X$, the system generates $N$ samples (default $10,000$) based on the source's trust profile:
$$X_{samples} \sim \mathcal{N}(\mu, \sigma_{inflated})$$

### 2. Orbital Physics (K20)
States are propagated using the SGP4 (Simplified General Perturbations) model. The core detection metric is the **Delta-V Residual**:
$$\Delta V_{res} = |V_{observed} - V_{ballistic}|$$
If $\Delta V_{res}$ exceeds the 99.9th percentile of the propagated uncertainty, the satellite is flagged for manual review.

---

## 🌐 Data Bridge & API Integration

The system pulls real-time orbital data through `src/satellite_bridge.py`. It is configured to interface with:

* **Celestrak GP API**: Used for fetching General Perturbations (GP) data and TLEs for "active" and "decommissioned" groups.
* **Space-Track (Optional)**: Support for authenticated TLE history requests.
* **Trust Calibration**: Every data source is passed through `observation_ingestion.py`, where its **Trust Score** (0.0 to 1.0) is calculated based on instrument precision (e.g., GPS has higher trust than Ground Radar).



---

## 🤝 Contributor Guidelines

We welcome contributions. To maintain scientific integrity, all contributors must follow:

1.  **Respect Rule 1**: Any new function returning a physical value MUST return an `UncertainQuantity` object.
2.  **No Backward Time Travel**: When updating `k21_memory.py`, never overwrite history. Evolve the case state instead.
3.  **Stateless Physics**: Keep `k20_physics.py` functions pure (input -> calculation -> output).

### Pull Request Process
1.  Branch: `feature/k[XX]-description` (e.g., `feature/k20-new-integrator`).
2.  Validation: Run `python -m src.k19_uncertainty` to ensure the math engine is intact.

---

## 🚀 Quick Start

```bash
# Clone the repo
git clone [https://github.com/chiragrathiresearcher/m17-k-framework.git](https://github.com/chiragrathiresearcher/m17-k-framework.git)

# Install dependencies
pip install numpy pandas requests sgp4

# Run a deep-dive audit on a specific satellite (e.g., LANDSAT 7)
python run_research.py --norad 25731
