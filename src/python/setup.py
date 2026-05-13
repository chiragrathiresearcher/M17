#!/usr/bin/env python3
"""
ChiragRathi M17 Python Interface Setup

Setup script for the Python bindings to the M17 ChiragRathi framework.
Provides researcher-friendly access to high-performance C++ and Rust components.

Author: ChiragRathi
Version: 2.0.0
Date: 2026
"""

from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup, find_packages
import pybind11
import numpy as np
import os
import platform
import subprocess
from pathlib import Path

# Get version from version file
version = {}
with open("m17/_version.py") as fp:
    exec(fp.read(), version)

# Detect system capabilities
def has_avx2():
    """Check if system supports AVX2 instructions"""
    try:
        if platform.system() == "Linux":
            with open("/proc/cpuinfo", "r") as f:
                return "avx2" in f.read()
        elif platform.system() == "Darwin":
            result = subprocess.run(["sysctl", "-n", "machdep.cpu.features"], 
                                  capture_output=True, text=True)
            return "AVX2" in result.stdout
        elif platform.system() == "Windows":
            # Use wmic or registry check
            return True  # Assume modern Windows systems have AVX2
    except:
        pass
    return False

def has_cuda():
    """Check if CUDA is available"""
    try:
        result = subprocess.run(["nvcc", "--version"], 
                              capture_output=True, text=True)
        return result.returncode == 0
    except:
        return False

# Configure compiler flags
cpp_std = 20
compile_args = []
link_args = []

if platform.system() != "Windows":
    compile_args.extend([
        f"-std=c++{cpp_std}",
        "-O3", "-DNDEBUG",
        "-ffast-math",
        "-march=native", "-mtune=native",
        "-fopenmp"
    ])
    link_args.extend(["-fopenmp"])
    
    if has_avx2():
        compile_args.extend(["-mavx2", "-DHAS_AVX2"])
        print("Building with AVX2 support")
    
else:
    compile_args.extend([
        f"/std:c++{cpp_std}",
        "/O2", "/Ob2", "/DNDEBUG",
        "/arch:AVX2", "/openmp"
    ])

# Include directories
include_dirs = [
    pybind11.get_cmake_dir(),
    np.get_include(),
    "../../core",  # M17 core headers
    "../../external/eigen",
    "../../external/boost",
]

# Library directories
library_dirs = []
libraries = []

if platform.system() != "Windows":
    libraries.extend(["m", "gomp"])  # Math and OpenMP
else:
    libraries.extend(["openmp"])

# CUDA support
if has_cuda():
    include_dirs.extend([
        "/usr/local/cuda/include",
        "/opt/cuda/include"
    ])
    library_dirs.extend([
        "/usr/local/cuda/lib64",
        "/opt/cuda/lib64"
    ])
    libraries.extend(["cudart", "cufft"])
    compile_args.append("-DHAS_CUDA")
    print("Building with CUDA support")

# Define extensions
ext_modules = []

# Only include extensions if binding files exist
try:
    if Path("bindings/uncertainty_bindings.cpp").exists():
        ext_modules.append(
            Pybind11Extension(
                "m17._uncertainty",
                [
                    "bindings/uncertainty_bindings.cpp",
                    "../../core/uncertainty/uncertainty_engine.cpp",
                ],
                include_dirs=include_dirs,
                libraries=libraries,
                library_dirs=library_dirs,
                language='c++',
                cxx_std=cpp_std,
                extra_compile_args=compile_args,
                extra_link_args=link_args,
            )
        )
except:
    pass

try:
    if Path("bindings/physics_bindings.cpp").exists():
        ext_modules.append(
            Pybind11Extension(
                "m17._physics",
                [
                    "bindings/physics_bindings.cpp",
                    "../../core/physics/relativistic_physics.cpp",
                ],
                include_dirs=include_dirs,
                libraries=libraries,
                library_dirs=library_dirs,
                language='c++',
                cxx_std=cpp_std,
                extra_compile_args=compile_args,
                extra_link_args=link_args,
            )
        )
except:
    pass

try:
    if Path("bindings/memory_bindings.cpp").exists():
        ext_modules.append(
            Pybind11Extension(
                "m17._memory",
                [
                    "bindings/memory_bindings.cpp",
                    "../../core/memory/scientific_memory.cpp",
                ],
                include_dirs=include_dirs,
                libraries=libraries,
                library_dirs=library_dirs,
                language='c++',
                cxx_std=cpp_std,
                extra_compile_args=compile_args,
                extra_link_args=link_args,
            )
        )
except:
    pass

# Read README for long description
try:
    with open("../../README.md", "r", encoding="utf-8") as fh:
        long_description = fh.read()
except FileNotFoundError:
    long_description = "ChiragRathi M17 Autonomous Celestial Discovery Framework"

# Setup configuration
setup(
    name="m17",
    version=version["__version__"],
    author="ChiragRathi",
    author_email="chirag.rathi@chiragrathi.org",
    description="ChiragRathi M17 Autonomous Celestial Discovery Framework",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/chiragrathiresearcher/m17-framework",
    project_urls={
        "Bug Tracker": "https://github.com/chiragrathiresearcher/m17-framework/issues",
        "Documentation": "https://chiragrathiresearcher.github.io/m17-framework/",
        "Source Code": "https://github.com/chiragrathiresearcher/m17-framework",
        "Research Paper": "https://arxiv.org/abs/2026.xxxxx",
    },
    packages=find_packages(),
    include_package_data=True,
    package_data={
        "m17": [
            "data/*.json",
            "data/*.dat", 
            "data/*.txt",
            "models/*.pkl",
            "models/*.h5",
        ]
    },
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Science/Research",
        "Topic :: Scientific/Engineering :: Astronomy",
        "Topic :: Scientific/Engineering :: Physics",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: C++",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Operating System :: POSIX :: Linux",
        "Operating System :: MacOS",
        "Operating System :: Microsoft :: Windows",
        "Natural Language :: English",
    ],
    keywords=[
        "astronomy", "physics", "uncertainty", "orbital-mechanics", 
        "machine-learning", "gravitational-lensing", "dark-matter",
        "satellite-tracking", "anomaly-detection", "real-time"
    ],
    python_requires=">=3.9",
    install_requires=[
        "numpy>=1.21.0",
        "scipy>=1.7.0", 
        "pandas>=1.3.0",
        "matplotlib>=3.4.0",
        "astropy>=4.3.0",
        "scikit-learn>=1.0.0",
        "numba>=0.56.0",
        "tqdm>=4.62.0",
        "h5py>=3.3.0",
        "pyyaml>=5.4.0",
    ],
    extras_require={
        "ml": [
            "tensorflow>=2.8.0",
            "torch>=1.11.0", 
            "jax>=0.3.0",
            "optax>=0.1.0",
            "transformers>=4.20.0",
        ],
        "gpu": [
            "cupy>=10.0.0",
            "nvidia-ml-py>=11.0.0",
        ],
        "astronomy": [
            "astroquery>=0.4.6",
            "astroplan>=0.8.0",
            "photutils>=1.5.0",
            "regions>=0.5.0",
            "specutils>=1.7.0",
        ],
        "visualization": [
            "plotly>=5.0.0",
            "seaborn>=0.11.0",
            "bokeh>=2.4.0",
            "ipywidgets>=7.6.0",
        ],
        "docs": [
            "sphinx>=4.0.0",
            "sphinx-rtd-theme>=1.0.0",
            "myst-parser>=0.15.0",
            "sphinx-autoapi>=1.8.0",
            "nbsphinx>=0.8.0",
        ],
        "test": [
            "pytest>=6.0.0",
            "pytest-cov>=2.12.0",
            "pytest-benchmark>=3.4.0",
            "hypothesis>=6.14.0",
        ],
        "dev": [
            "black>=21.6.0",
            "isort>=5.9.0",
            "flake8>=3.9.0",
            "mypy>=0.910",
            "pre-commit>=2.13.0",
            "jupyter>=1.0.0",
        ],
        "all": [
            "tensorflow>=2.8.0", "torch>=1.11.0", "jax>=0.3.0",
            "cupy>=10.0.0", "astroquery>=0.4.6", "plotly>=5.0.0",
            "sphinx>=4.0.0", "pytest>=6.0.0", "black>=21.6.0",
        ]
    },
    entry_points={
        "console_scripts": [
            "m17-uncertainty=m17.cli.uncertainty:main",
            "m17-physics=m17.cli.physics:main", 
            "m17-discovery=m17.cli.discovery:main",
            "m17-benchmark=m17.cli.benchmark:main",
            "m17-validate=m17.cli.validate:main",
        ],
    },
    zip_safe=False,
    platforms=["Linux", "macOS", "Windows"],
    license="MIT",
    
    # Build configuration
    options={
        "build_ext": {
            "parallel": True,  # Parallel compilation
        }
    },
    
    # Metadata for academic citation
    metadata={
        "Citation": (
            "Rathi, Chirag (2026). M17: Autonomous Celestial Discovery Framework. "
            "GitHub: https://github.com/chiragrathiresearcher/m17-framework"
        ),
        "DOI": "10.5281/zenodo.xxxxxxx",
        "ORCID": "https://orcid.org/0000-0000-0000-0000",
    },
)

# Post-installation message
print("\n" + "="*60)
print("ChiragRathi M17 Framework Installation Complete")
print("="*60)
print("Framework capabilities:")
if has_avx2():
    print("  ✓ AVX2 SIMD acceleration enabled")
else:
    print("  ⚠ AVX2 not available - reduced performance")

if has_cuda():
    print("  ✓ CUDA GPU acceleration enabled")
else:
    print("  ⚠ CUDA not available - CPU-only mode")

print("\nQuick start:")
print("  import m17 as m17")
print("  help(m17)")
print("\nDocumentation: https://chiragrathiresearcher.github.io/m17-framework/")
print("Examples: https://github.com/chiragrathiresearcher/m17-framework/tree/main/examples")
print("="*60)
