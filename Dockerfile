# ChiragRathi M17 Framework Docker Container
# Multi-stage build for optimized deployment
# 
# Author: ChiragRathi
# Version: 2.0.0
# Date: 2026

# ==============================================================================
# Build Stage: Compile M17 Framework
# ==============================================================================

FROM ubuntu:22.04 as builder

LABEL maintainer="ChiragRathi <chirag.rathi@chiragrathi.org>"
LABEL version="2.0.0"
LABEL description="ChiragRathi M17 Autonomous Celestial Discovery Framework"

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    # Build tools
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    git \
    wget \
    curl \
    # Compilers and languages
    gcc-12 \
    g++-12 \
    gfortran-12 \
    rustc \
    cargo \
    python3 \
    python3-dev \
    python3-pip \
    python3-venv \
    # Dependencies
    libeigen3-dev \
    libboost-all-dev \
    libtbb-dev \
    libopenmpi-dev \
    libhdf5-dev \
    libfftw3-dev \
    libblas-dev \
    liblapack-dev \
    # Optional GPU support
    nvidia-cuda-toolkit \
    nvidia-cuda-dev \
    # Testing
    libgtest-dev \
    libgmock-dev \
    # Python scientific stack
    python3-numpy \
    python3-scipy \
    python3-matplotlib \
    python3-astropy \
    python3-pandas \
    # Cleanup cache
    && rm -rf /var/lib/apt/lists/*

# Set compiler versions
ENV CC=gcc-12
ENV CXX=g++-12
ENV FC=gfortran-12

# Create build user
RUN useradd -m -s /bin/bash builder
USER builder
WORKDIR /home/builder

# Copy source code
COPY --chown=builder:builder . /home/builder/M17_ChiragRathi

# Build M17 Framework
WORKDIR /home/builder/M17_ChiragRathi

# Install Python dependencies
RUN python3 -m pip install --user --upgrade pip setuptools wheel
RUN python3 -m pip install --user pybind11[global] numpy scipy matplotlib astropy pandas tqdm

# Create build directory
RUN mkdir -p build

# Configure with CMake
WORKDIR /home/builder/M17_ChiragRathi/build
RUN cmake .. \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/opt/m17 \
    -DBUILD_TESTS=ON \
    -DBUILD_BENCHMARKS=ON \
    -DBUILD_EXAMPLES=ON \
    -DBUILD_PYTHON_BINDINGS=ON \
    -DENABLE_SIMD=ON \
    -DENABLE_GPU=ON \
    -DENABLE_PROFILING=OFF

# Build M17 Framework
RUN ninja -j$(nproc)

# Run tests to verify build
RUN ctest --output-on-failure

# Install to staging area
RUN ninja install DESTDIR=/home/builder/m17-staging

# Build Rust components
WORKDIR /home/builder/M17_ChiragRathi/src/rust
RUN cargo build --release

# Build Python bindings
WORKDIR /home/builder/M17_ChiragRathi/src/python
RUN python3 setup.py build_ext --inplace
RUN python3 setup.py bdist_wheel

# ==============================================================================
# Runtime Stage: Minimal deployment image
# ==============================================================================

FROM ubuntu:22.04 as runtime

LABEL maintainer="ChiragRathi <chirag.rathi@chiragrathi.org>"
LABEL version="2.0.0"
LABEL description="ChiragRathi M17 Runtime Environment"

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    # Runtime libraries
    libboost-system1.74.0 \
    libboost-filesystem1.74.0 \
    libboost-program-options1.74.0 \
    libtbb12 \
    libopenmpi3 \
    libhdf5-103 \
    libfftw3-3 \
    libblas3 \
    liblapack3 \
    # Python runtime
    python3 \
    python3-pip \
    python3-numpy \
    python3-scipy \
    python3-matplotlib \
    python3-astropy \
    python3-pandas \
    # System utilities
    curl \
    wget \
    vim \
    htop \
    # Optional GPU runtime
    nvidia-cuda-runtime \
    && rm -rf /var/lib/apt/lists/*

# Create application user
RUN useradd -m -s /bin/bash -u 1000 m17user
USER m17user
WORKDIR /home/m17user

# Copy built artifacts from builder stage
COPY --from=builder --chown=m17user:m17user /home/builder/m17-staging/opt/m17 /opt/m17
COPY --from=builder --chown=m17user:m17user /home/builder/M17_ChiragRathi/src/rust/target/release /opt/m17/lib/rust
COPY --from=builder --chown=m17user:m17user /home/builder/M17_ChiragRathi/src/python/dist/*.whl /tmp/

# Install Python wheel
RUN python3 -m pip install --user /tmp/*.whl

# Set up environment
ENV PATH="/opt/m17/bin:${PATH}"
ENV LD_LIBRARY_PATH="/opt/m17/lib:${LD_LIBRARY_PATH}"
ENV PYTHONPATH="/opt/m17/lib/python:${PYTHONPATH}"
ENV M17_DATA_DIR="/data"
ENV M17_OUTPUT_DIR="/output"
ENV M17_CONFIG_DIR="/config"

# Create data directories
RUN mkdir -p /home/m17user/data /home/m17user/output /home/m17user/config

# Copy configuration templates
COPY --from=builder --chown=m17user:m17user /home/builder/M17_ChiragRathi/examples/config /home/m17user/config

# Copy examples and documentation
COPY --from=builder --chown=m17user:m17user /home/builder/M17_ChiragRathi/examples /home/m17user/examples
COPY --from=builder --chown=m17user:m17user /home/builder/M17_ChiragRathi/README.md /home/m17user/
COPY --from=builder --chown=m17user:m17user /home/builder/M17_ChiragRathi/docs /home/m17user/docs

# Health check script
COPY --chown=m17user:m17user <<EOF /home/m17user/healthcheck.sh
#!/bin/bash
# M17 Framework Health Check
set -e

echo "Checking M17 Framework health..."

# Check binaries
if ! command -v m17_core >/dev/null 2>&1; then
    echo "ERROR: m17_core not found"
    exit 1
fi

# Check Python module
python3 -c "import m17; print('Python module OK')" || exit 1

# Check basic functionality
timeout 30 m17_core --mode test || exit 1

echo "M17 Framework health check passed"
exit 0
EOF

RUN chmod +x /home/m17user/healthcheck.sh

# Startup script
COPY --chown=m17user:m17user <<EOF /home/m17user/start.sh
#!/bin/bash
# M17 Framework Startup Script

set -e

echo "Starting ChiragRathi M17 Framework v2.0.0"
echo "======================================"

# Display system information
echo "System Information:"
echo "  Hostname: \$(hostname)"
echo "  User: \$(whoami)"
echo "  Working Directory: \$(pwd)"
echo "  CPU Cores: \$(nproc)"
echo "  Memory: \$(free -h | grep Mem | awk '{print \$2}')"
echo ""

# Check GPU availability
if command -v nvidia-smi >/dev/null 2>&1; then
    echo "GPU Information:"
    nvidia-smi --query-gpu=name,memory.total --format=csv,noheader
    echo ""
fi

# Display configuration
echo "M17 Configuration:"
echo "  Data Directory: \$M17_DATA_DIR"
echo "  Output Directory: \$M17_OUTPUT_DIR"
echo "  Config Directory: \$M17_CONFIG_DIR"
echo ""

# Run health check
echo "Running health check..."
./healthcheck.sh
echo ""

# Start M17 Framework with provided arguments
echo "Starting M17 Framework..."
exec m17_core "\$@"
EOF

RUN chmod +x /home/m17user/start.sh

# Expose ports for API access
EXPOSE 8080 8081 8082

# Health check configuration
HEALTHCHECK --interval=30s --timeout=10s --start-period=60s --retries=3 \
  CMD /home/m17user/healthcheck.sh

# Default volumes
VOLUME ["/data", "/output", "/config"]

# Default working directory
WORKDIR /home/m17user

# Default command
ENTRYPOINT ["/home/m17user/start.sh"]
CMD ["--mode", "interactive"]

# ==============================================================================
# Development Stage: Full development environment
# ==============================================================================

FROM builder as development

USER builder
WORKDIR /home/builder/M17_ChiragRathi

# Install additional development tools
USER root
RUN apt-get update && apt-get install -y \
    # Development tools
    gdb \
    valgrind \
    clang-format \
    clang-tidy \
    cppcheck \
    doxygen \
    graphviz \
    # Editors
    vim \
    emacs-nox \
    # Version control
    git \
    git-lfs \
    # Documentation
    pandoc \
    texlive-latex-base \
    texlive-latex-recommended \
    # Jupyter environment
    jupyter-notebook \
    && rm -rf /var/lib/apt/lists/*

USER builder

# Install development Python packages
RUN python3 -m pip install --user \
    jupyter \
    jupyterlab \
    ipython \
    black \
    flake8 \
    mypy \
    pytest \
    pytest-cov \
    pytest-benchmark \
    sphinx \
    sphinx-rtd-theme

# Set up Jupyter
RUN jupyter notebook --generate-config
COPY --chown=builder:builder <<EOF /home/builder/.jupyter/jupyter_notebook_config.py
c.NotebookApp.ip = '0.0.0.0'
c.NotebookApp.port = 8888
c.NotebookApp.open_browser = False
c.NotebookApp.allow_root = False
c.NotebookApp.notebook_dir = '/home/builder/M17_ChiragRathi'
EOF

# Expose Jupyter port
EXPOSE 8888

# Development startup script
COPY --chown=builder:builder <<EOF /home/builder/dev-start.sh
#!/bin/bash
echo "ChiragRathi M17 Development Environment"
echo "=================================="
echo ""
echo "Available commands:"
echo "  ninja           - Build framework"
echo "  ctest           - Run tests"
echo "  jupyter lab     - Start Jupyter Lab"
echo "  m17_core        - Run framework"
echo "  m17_benchmark   - Run benchmarks"
echo "  m17_validate    - Run validation"
echo ""
echo "Starting development shell..."
exec /bin/bash "\$@"
EOF

RUN chmod +x /home/builder/dev-start.sh

# Default for development
ENTRYPOINT ["/home/builder/dev-start.sh"]

# ==============================================================================
# Benchmarking Stage: Performance testing environment  
# ==============================================================================

FROM runtime as benchmark

USER root

# Install benchmarking tools
RUN apt-get update && apt-get install -y \
    # Performance tools
    perf-tools-unstable \
    sysstat \
    iotop \
    htop \
    stress-ng \
    # Monitoring
    prometheus-node-exporter \
    && rm -rf /var/lib/apt/lists/*

USER m17user

# Copy benchmarking scripts and data
COPY --from=builder --chown=m17user:m17user /home/builder/M17_ChiragRathi/benchmarks /home/m17user/benchmarks

# Benchmarking startup script
COPY --chown=m17user:m17user <<EOF /home/m17user/benchmark.sh
#!/bin/bash
echo "M17 Framework Benchmarking Environment"
echo "====================================="

# System information
echo "System Resources:"
echo "  CPU: \$(nproc) cores"
echo "  Memory: \$(free -h | grep Mem | awk '{print \$2}')"
echo "  Storage: \$(df -h . | tail -1 | awk '{print \$4}') available"

if command -v nvidia-smi >/dev/null 2>&1; then
    echo "  GPU: \$(nvidia-smi --query-gpu=count --format=csv,noheader) device(s)"
fi

echo ""

# Run comprehensive benchmarks
echo "Running M17 Framework benchmarks..."
exec m17_benchmark "\$@"
EOF

RUN chmod +x /home/m17user/benchmark.sh

# Default for benchmarking
ENTRYPOINT ["/home/m17user/benchmark.sh"]
CMD ["--iterations", "10000", "--verbose"]
