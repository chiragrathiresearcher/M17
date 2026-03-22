# Contributing to M17

Welcome to the M17 Autonomous Celestial Discovery Framework, developed and maintained by **Chirag Rathi**. Contributions from the community are welcome — please read the guidelines below before opening a PR.

## 🌟 Overview

M17 is a solo research project advancing astronomical discovery through uncertainty quantification, relativistic physics, and AI-enhanced analysis. Contributions are accepted in the following areas:

- **Algorithm improvements** — uncertainty propagation, relativistic effects, ML anomaly detection
- **Performance optimization** — SIMD, GPU kernels, memory layout
- **Testing & validation** — cross-validation against established codes, regression tests
- **Documentation** — tutorials, API reference, examples

## 🚀 Quick Start

```bash
git clone --recursive https://github.com/chiragrathiresearcher/m17-framework.git
cd m17-framework

pip install -r requirements-dev.txt
conda install -c conda-forge eigen boost-cpp cmake ninja

pre-commit install
```

### Development Build

```bash
mkdir build-dev && cd build-dev
cmake .. -GNinja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTS=ON \
  -DBUILD_BENCHMARKS=ON \
  -DBUILD_EXAMPLES=ON \
  -DENABLE_SANITIZERS=ON

ninja
ctest --output-on-failure
```

### Verify Your Setup

```bash
./tests/unit/test_uncertainty_basic
./tests/integration/test_minimal_pipeline

cd ../src/python
python -m pytest tests/ -v

./tools/validation/validate_m17.py --quick
```

## 📋 Contribution Areas

### 1. Core Algorithms (`src/core/`)

**High Priority:**
- Uncertainty propagation optimization
- New relativistic effects implementation
- Advanced ML anomaly detection models
- Numerical stability improvements

### 2. Language Interfaces

**C++ (`src/core/`)** — Header-only preferred, C++20, Doxygen docs  
**Python (`src/python/`)** — NumPy/SciPy/Astropy compatible, type hints  
**Fortran (`src/fortran/`)** — High-precision numerics, BLAS/LAPACK, OpenMP  
**Rust (`src/rust/`)** — Safe parallel algorithms, memory-efficient structures

### 3. Validation & Testing (`tests/`, `tools/validation/`)

- Historical event reproductions
- Cross-validation with established codes
- Performance regression tests
- Numerical accuracy benchmarks

### 4. Documentation (`docs/`)

- Tutorial improvements, API reference updates
- Architecture diagrams, algorithm explanations

## 🔬 Scientific Contribution Guidelines

### Algorithm Implementation

1. Provide complete mathematical derivation with peer-reviewed references
2. Include numerical stability analysis and documented assumptions
3. Validate against analytical solutions and benchmark performance

```cpp
/**
 * @brief Brief description of algorithm
 *
 * Detailed mathematical description with equations.
 * Reference: Author et al., Journal, Year, DOI
 *
 * Mathematical formulation:
 * f(x) = ∑ᵢ aᵢ xᵢ + O(ε²)
 *
 * @param input Description with units and constraints
 * @return Description with uncertainty information
 *
 * @note Numerical stability: stable for |x| < 10¹²
 * @warning May fail for singular inputs
 */
uncertainty::UncertainQuantity algorithm_implementation(
    const std::vector<uncertainty::UncertainQuantity>& input);
```

### Publication and Citation

If your contribution leads to a research paper, add a citation to `CITATIONS.md` and include the preprint reference in your pull request.

## 📝 Development Workflow

### 1. Before Starting

1. Check existing issues for similar proposals
2. Open an issue with a detailed description and implementation plan

### 2. Development Process

```bash
git checkout -b feature/new-uncertainty-method

# implement + test

ninja format-cpp lint-cpp test
python -m black src/python/
python -m pytest src/python/tests/

git commit -m "algorithms: implement Monte Carlo uncertainty propagation

- Add MCUncertaintyPropagator class
- Implement adaptive sampling algorithm
- Validate against analytical solutions
- Add comprehensive unit tests

Fixes #123"

git push origin feature/new-uncertainty-method
```

### 3. Pull Request Requirements

- [ ] Clear description and motivation
- [ ] Tests for new functionality
- [ ] Documentation updates
- [ ] Benchmark results for performance changes
- [ ] Validation against known results
- [ ] Changelog entry

### 4. Review Criteria

- Is the mathematics correct?
- Are edge cases handled?
- Is the code maintainable?
- Are performance impacts acceptable?

## 🧪 Testing Standards

**Unit Tests** (`tests/unit/`)
```cpp
TEST_CASE("UncertaintyPropagation::Addition", "[uncertainty]") {
    auto x = uncertainty::UncertainQuantity(2.0, 0.1, "x");
    auto y = uncertainty::UncertainQuantity(3.0, 0.2, "y");
    auto result = x + y;
    REQUIRE(result.value() == Approx(5.0));
    REQUIRE(result.uncertainty() == Approx(sqrt(0.1*0.1 + 0.2*0.2)));
}
```

**Requirements:** ≥85% coverage for new code, edge cases covered, cross-platform (Linux, macOS, Windows).

## 🎯 Performance Optimization

```bash
# CPU profiling
perf record ./benchmarks/benchmark_uncertainty --benchmark_format=json

# Memory analysis
valgrind --tool=massif ./benchmarks/benchmark_uncertainty

# GPU profiling
nsys profile --stats=true ./benchmarks/gpu_benchmark
```

**Guidelines:** SIMD intrinsics, cache-friendly layouts, minimize allocations in hot paths, profile before and after.

## 📚 Documentation Standards

**C++ (Doxygen):**
```cpp
/**
 * @brief Computes gravitational time dilation for GPS satellites
 *
 * @param orbital_radius Satellite orbital radius [m]
 * @param orbital_velocity Satellite orbital speed [m/s]
 * @return Time dilation factor [dimensionless]
 *
 * @see Ashby, N. (2003). "Relativity in GPS", Living Rev. Relativity
 */
double compute_gps_time_dilation(double orbital_radius, double orbital_velocity);
```

**Python (NumPy style):**
```python
def propagate_uncertainty(inputs, function, method='analytical'):
    """
    Propagate uncertainties through mathematical functions.

    Parameters
    ----------
    inputs : list of UncertainQuantity
    function : callable
    method : {'analytical', 'monte_carlo', 'auto'}, optional

    Returns
    -------
    UncertainQuantity

    References
    ----------
    .. [1] Taylor, J.R. "An Introduction to Error Analysis", 1997
    .. [2] JCGM 100:2008 "GUM - Guide to Uncertainty Measurement"
    """
```

## 🔄 Release Process

Semantic versioning (MAJOR.MINOR.PATCH):

- **MAJOR**: Incompatible API changes
- **MINOR**: Backward-compatible new features
- **PATCH**: Bug fixes

## 📄 Legal and Licensing

M17 is released under the MIT License. By contributing, you confirm your contributions are your original work and grant the project a perpetual, irrevocable license to use them.

### Citation

```bibtex
@software{m17_framework_2026,
  title  = {{M17: Autonomous Celestial Discovery Framework}},
  author = {Rathi, Chirag},
  year   = {2026},
  url    = {https://github.com/chiragrathiresearcher/m17-framework},
  doi    = {10.5281/zenodo.xxxxxxx}
}
```

## 🆘 Getting Help

- **Bug reports / feature requests**: GitHub Issues
- **Questions**: GitHub Discussions
- **Academic collaboration**: [chirag.rathi@example.com](mailto:chirag.rathi@example.com)
- **Documentation**: [chiragrathiresearcher.github.io/m17-framework](https://chiragrathiresearcher.github.io/m17-framework)

---

*For questions about contributing, contact: [contribute@chiragrathi.org](mailto:contribute@chiragrathi.org)*

*Last updated: 2026-03-23*
