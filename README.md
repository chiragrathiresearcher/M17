# M17 — Autonomous Celestial Discovery Framework

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**M17** is a framework for autonomous astronomical discovery, implementing
real-time uncertainty quantification, relativistic orbital mechanics, and
machine learning-enhanced pattern recognition. Developed by Chirag Rathi,
M17 is built around the idea that noise is often undiscovered signal —
aiming to help surface celestial phenomena that conventional analysis
filters out.

## 🎯 Core Capabilities

- **Uncertainty Quantification**: Covariance propagation for scientific measurements
- **Relativistic Physics**: Post-Newtonian orbital mechanics
- **AI-Enhanced Memory**: Case-based reasoning with deep learning classification
- **Real-Time Processing**: Stream processing pipeline
- **Multi-Language Performance**: C++20, Fortran 2018, Rust, and Python integration
- **Astronomical Standards**: Built with IAU/CODATA/IEEE conventions in mind

## 🚀 Quick Start

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt update && sudo apt install -y \
    build-essential cmake ninja-build \
    libeigen3-dev libboost-all-dev \
    gfortran python3-dev \
    libopenmpi-dev libomp-dev

# macOS
brew install cmake ninja eigen boost gcc python@3.11 open-mpi libomp

# Scientific Python stack
pip install numpy scipy astropy matplotlib pandas scikit-learn
```

### Installation

```bash
git clone --recursive https://github.com/chiragrathiresearcher/m17-framework.git
cd m17-framework
mkdir build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja
sudo ninja install

# Python bindings
cd ../src/python
pip install -e .
```

> **Status note**: the CMake build wires together C++20, Eigen, Boost,
> optional CUDA, and pybind11. If you hit build issues, please open an
> issue with your platform and the full error — this is still an early,
> actively-developed build and reports genuinely help.

### Basic Usage

```python
import m17 as m17

# Create uncertain observation
position = m17.UncertainQuantity(1000.0, 0.1, "GPS")
velocity = m17.UncertainQuantity(7500.0, 0.5, "radar")

# Propagate uncertainty through orbital mechanics
state = m17.OrbitalState([position, 0, 0], [0, velocity, 0], epoch=0.0)
future_state = m17.propagate_orbit(state, time_span=3600.0)

# Real-time anomaly detection
processor = m17.RealTimeProcessor(num_workers=8)
processor.start()

for observation in data_stream:
    processor.submit(observation)
    if processor.has_result():
        result = processor.get_result()
        if result.is_anomalous():
            print(f"Anomaly detected: {result}")
```

```cpp
#include <chiragrathiresearcher/m17.hpp>
using namespace ChiragRathi::m17;

// High-performance C++ API
uncertainty::UncertainQuantity obs(1000.0, 0.1, "measurement");
auto result = uncertainty::propagate(
    [](const Eigen::VectorXd& x) { return x[0] * x[0]; },
    {obs}
);

// Physics simulation
physics::RelativisticOrbitalState state(position, velocity, epoch);
auto acceleration = physics_engine.compute_acceleration(state);
```

## 📊 Performance

Target performance characteristics (design goals, not yet independently
benchmarked and published — see `tools/validation/` to run the suite
yourself and help verify these):

| Component | Target Throughput | Target Latency |
|-----------|------------|---------|
| Uncertainty Propagation | high-throughput, SIMD-accelerated | microsecond-scale |
| Orbital Mechanics | — | — |
| Anomaly Classification | — | — |
| Stream Processing | — | — |

Once real numbers are captured from `tools/validation/validate_m17.py`
and `benchmarks/main_benchmark.cpp` on a specific machine, they'll replace
this table with reproducible, dated results (hardware spec included).

## 🧪 Scientific Validation

M17 includes a validation suite (`tools/validation/validate_m17.py`) with
test cases designed to check outputs against known physical benchmarks,
including:

- GPS relativistic time dilation (target: matching the well-known
  ~38.6 μs/day correction)
- Orbital mechanics sanity checks (e.g., ISS orbital period)
- Gravitational lensing detection on known reference events

These are currently validation *targets* the suite is designed to check —
run `./tools/validation/validate_m17.py --verbose` yourself and see
[Contributing](CONTRIBUTING.md) if you'd like to help expand real,
published validation results.

## 🏗️ Architecture

### Multi-Language Design

```
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│   Python API    │  │   C++ Core      │  │   Fortran       │
│  Research UI    │  │  Performance    │  │  Numerical      │
└─────────────────┘  └─────────────────┘  └─────────────────┘
         │                     │                     │
         └─────────────────────┼─────────────────────┘
                               │
                    ┌─────────────────┐
                    │   Rust          │
                    │  Parallel/Safe  │
                    │  Concurrency    │
                    └─────────────────┘
```

*(GPU/CUDA support exists in `src/gpu/` and is optional at build time,
gated behind `find_package(CUDAToolkit QUIET)`.)*

### Core Components

- **`src/core/uncertainty/`**: Uncertainty propagation engine
- **`src/core/physics/`**: Relativistic orbital mechanics
- **`src/core/memory/`**: AI-enhanced scientific memory
- **`src/core/fusion/`**: Multi-source data fusion
- **`src/fortran/`**: High-precision numerical routines
- **`src/rust/`**: Parallel processing
- **`src/gpu/`**: Optional CUDA acceleration

## 🔬 Research Applications (Intended Use Cases)

### Astronomical Discovery

- Gravitational lensing detection in survey data
- Orbital anomaly analysis
- Variable star classification
- Exoplanet transit detection with uncertainty tracking

### Space Situational Awareness

- Satellite orbit determination
- Debris collision proximity analysis
- Spacecraft attitude/thruster anomaly detection
- Orbital decay modeling

## 🛠️ Development

### Testing

```bash
# C++ test suite (27 test cases covering uncertainty, physics, memory, fusion)
cd build && ninja test

# Python
cd src/python && python -m pytest

# Validation suite
./tools/validation/validate_m17.py --verbose
```

### Code Quality

```bash
# Python formatting
black src/python/
isort src/python/
flake8 src/python/

# Rust formatting
cd src/rust && cargo fmt && cargo clippy
```

## 🤝 Contributing

Contributions are welcome from the astronomical and computational
communities — see [CONTRIBUTING.md](CONTRIBUTING.md) for the full guide.

### Getting Started

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/your-change`
3. Read the [Contributing Guide](CONTRIBUTING.md)
4. Submit a Pull Request

### Areas for Contribution

- **Algorithms**: New uncertainty propagation methods
- **Physics**: Additional relativistic effects
- **ML Models**: Improved anomaly detection
- **Performance**: Build verification, SIMD optimizations
- **Documentation**: Tutorials, examples, reproducible benchmark results
- **Validation**: New test cases and independently-verified benchmarks

## 📄 License

M17 is released under the [MIT License](LICENSE).

## 🎖️ Citation

If you use M17 in your research, please cite:

```bibtex
@software{m17_framework_2026,
  title = {{M17: Autonomous Celestial Discovery Framework}},
  author = {{Chirag Rathi}},
  year = {2026},
  url = {https://github.com/chiragrathiresearcher/m17-framework},
  version = {2.0.0}
}
```

*(A DOI will be added here once a Zenodo release is published and
confirmed live.)*

## 🙏 Acknowledgments

- **Chirag Rathi** — original M17 concept and design
- International Astronomical Union — standards and coordinate system references
- ESA Gaia Mission and NASA JPL — publicly available data used for validation targets

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/chiragrathiresearcher/m17-framework/issues)
- **Discussions**: [GitHub Discussions](https://github.com/chiragrathiresearcher/m17-framework/discussions)

---

**M17** — *Transforming noise into discovery*
