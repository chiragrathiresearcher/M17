"""
ChiragRathi M17 Autonomous Celestial Discovery Framework
===================================================

A comprehensive framework for real-time astronomical anomaly detection,
uncertainty quantification, and relativistic orbital mechanics.

Core Components:
- Uncertainty propagation with μs-level performance
- Relativistic orbital mechanics with μ-arcsecond precision  
- AI-enhanced scientific memory and pattern recognition
- Multi-source data fusion with adaptive trust assessment
- Real-time streaming with guaranteed latency bounds

Quick Start:
>>> import m17 as m17
>>> 
>>> # Create uncertain quantity
>>> position = m17.UncertainQuantity(1000.0, 0.1, "GPS")
>>> 
>>> # Propagate through function
>>> result = m17.propagate(lambda x: x**2, [position])
>>> print(f"Result: {result.value:.3f} ± {result.uncertainty:.3f}")
>>>
>>> # Real-time stream processing
>>> processor = m17.RealTimeProcessor(num_workers=8)
>>> processor.start()
>>> 
>>> for observation in data_stream:
>>>     processor.submit(observation)
>>>     result = processor.get_result(timeout=0.1)

Author: ChiragRathi
Version: 2.0.0
License: MIT
"""

from ._version import __version__, __author__, __email__

# Core uncertainty propagation
from ._uncertainty import (
    UncertainQuantity,
    UncertaintyPropagator, 
    RealTimeStreamProcessor as _RealTimeStreamProcessor,
    propagate_analytical,
    propagate_numerical,
)

# Relativistic physics
from ._physics import (
    OrbitalState,
    PhysicsEngine,
    RelativisticCorrections,
    GravitationalLensing,
    DarkMatterAnalysis,
)

# Scientific memory and AI
from ._memory import (
    ScientificMemory,
    CaseRegistry,
    PatternClassifier,
    AnomalyDetector,
)

# High-level convenience functions
from .convenience import (
    propagate,
    analyze_orbit,
    detect_anomalies,
    compute_lensing,
    assess_uncertainty,
)

# Data processing utilities
from .data import (
    load_astronomical_catalog,
    load_satellite_data,
    preprocess_observations,
    export_results,
)

# Visualization
from .plotting import (
    plot_uncertainty,
    plot_orbit,
    plot_anomaly_timeline,
    plot_performance_metrics,
)

# Configuration and constants
from .config import (
    Config,
    PhysicalConstants,
    PerformanceTargets,
    ValidationThresholds,
)

import logging
import warnings
from typing import Optional, List, Dict, Any, Union
import numpy as np
from pathlib import Path

# Configure logging
def setup_logging(level: str = "INFO") -> None:
    """
    Setup logging configuration for ChiragRathi M17.
    
    Parameters:
    -----------
    level : str
        Logging level ('DEBUG', 'INFO', 'WARNING', 'ERROR')
    """
    logging.basicConfig(
        level=getattr(logging, level.upper()),
        format='%(asctime)s - ChiragRathi-M17 - %(levelname)s - %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S'
    )

# Initialize logging
setup_logging()
logger = logging.getLogger(__name__)

# System capabilities detection
def _detect_capabilities():
    """Detect system capabilities for optimization"""
    capabilities = {
        'simd': False,
        'gpu': False, 
        'openmp': False,
        'avx2': False,
        'avx512': False,
        'cuda': False,
        'num_cores': 1,
    }
    
    try:
        import platform
        import os
        import subprocess
        
        # Detect CPU cores
        capabilities['num_cores'] = os.cpu_count() or 1
        
        # Check for SIMD support
        if platform.system() == "Linux":
            try:
                with open("/proc/cpuinfo", "r") as f:
                    cpuinfo = f.read()
                    capabilities['avx2'] = "avx2" in cpuinfo
                    capabilities['avx512f'] = "avx512f" in cpuinfo
                    capabilities['simd'] = capabilities['avx2'] or capabilities['avx512f']
            except:
                pass
        
        # Check for CUDA
        try:
            result = subprocess.run(['nvcc', '--version'], 
                                  capture_output=True, text=True, timeout=5)
            capabilities['cuda'] = result.returncode == 0
            capabilities['gpu'] = capabilities['cuda']
        except:
            pass
        
        # Check for OpenMP
        try:
            import _uncertainty
            capabilities['openmp'] = hasattr(_uncertainty, 'get_num_threads')
        except:
            pass
            
    except Exception as e:
        logger.warning(f"Failed to detect system capabilities: {e}")
    
    return capabilities

# Detect system capabilities
CAPABILITIES = _detect_capabilities()

# Performance configuration
class PerformanceConfig:
    """Global performance configuration"""
    
    def __init__(self):
        self.num_workers = min(CAPABILITIES['num_cores'], 16)
        self.use_simd = CAPABILITIES['simd']
        self.use_gpu = CAPABILITIES['gpu']
        self.batch_size = 10000 if CAPABILITIES['simd'] else 1000
        self.queue_size = 1000000
        self.enable_fast_math = True
        
    def optimize_for_latency(self):
        """Optimize configuration for low latency"""
        self.batch_size = 100
        self.num_workers = CAPABILITIES['num_cores']
        
    def optimize_for_throughput(self):
        """Optimize configuration for high throughput"""
        self.batch_size = 50000
        self.num_workers = min(CAPABILITIES['num_cores'] * 2, 32)

# Global configuration instance
performance_config = PerformanceConfig()

# Enhanced real-time processor with configuration
class RealTimeProcessor:
    """
    High-performance real-time processor for uncertain quantities.
    
    Automatically optimizes based on system capabilities and provides
    researcher-friendly interface to the underlying C++/Rust implementation.
    
    Parameters:
    -----------
    num_workers : int, optional
        Number of worker threads (auto-detected if None)
    queue_size : int, optional
        Maximum queue size (default: 1M)
    enable_gpu : bool, optional
        Enable GPU acceleration if available (default: auto-detect)
    
    Examples:
    ---------
    >>> processor = RealTimeProcessor()
    >>> processor.start()
    >>> 
    >>> # Process observations
    >>> for obs in observations:
    >>>     processor.submit(obs)
    >>>     if processor.has_result():
    >>>         result = processor.get_result()
    >>>         print(f"Processed: {result}")
    >>> 
    >>> processor.stop()
    """
    
    def __init__(self, 
                 num_workers: Optional[int] = None,
                 queue_size: Optional[int] = None,
                 enable_gpu: Optional[bool] = None):
        
        # Configure parameters
        self.num_workers = num_workers or performance_config.num_workers
        self.queue_size = queue_size or performance_config.queue_size
        self.enable_gpu = enable_gpu if enable_gpu is not None else CAPABILITIES['gpu']
        
        # Initialize underlying processor
        self._processor = _RealTimeStreamProcessor(
            self.num_workers, 
            self.queue_size
        )
        
        # Performance tracking
        self._start_time = None
        self._total_processed = 0
        
        logger.info(f"Initialized RealTimeProcessor with {self.num_workers} workers")
        if self.enable_gpu:
            logger.info("GPU acceleration enabled")
    
    def start(self) -> None:
        """Start the processing workers"""
        self._processor.start()
        self._start_time = time.time()
        logger.info("RealTimeProcessor started")
    
    def stop(self) -> None:
        """Stop the processing workers"""
        self._processor.stop()
        if self._start_time:
            elapsed = time.time() - self._start_time
            throughput = self._total_processed / elapsed if elapsed > 0 else 0
            logger.info(f"RealTimeProcessor stopped. Processed {self._total_processed} items "
                       f"in {elapsed:.2f}s ({throughput:.1f} items/sec)")
    
    def submit(self, observation: UncertainQuantity) -> bool:
        """
        Submit observation for processing
        
        Parameters:
        -----------
        observation : UncertainQuantity
            Observation to process
            
        Returns:
        --------
        bool
            True if successfully queued, False if queue full
        """
        success = self._processor.submit(observation)
        if success:
            self._total_processed += 1
        return success
    
    def get_result(self, timeout: float = 0.1) -> Optional[UncertainQuantity]:
        """
        Get processed result
        
        Parameters:
        -----------
        timeout : float
            Timeout in seconds
            
        Returns:
        --------
        UncertainQuantity or None
            Processed result or None if timeout
        """
        return self._processor.get_result(timeout)
    
    def has_result(self) -> bool:
        """Check if result is available"""
        return self._processor.has_result()
    
    def get_metrics(self) -> Dict[str, float]:
        """Get performance metrics"""
        metrics = self._processor.get_metrics()
        return {
            'operations_per_second': metrics.operations_per_second,
            'average_latency_us': metrics.average_latency_us,
            'error_rate': metrics.error_rate,
            'queue_utilization': metrics.queue_utilization,
            'total_processed': self._total_processed,
        }

# Convenience functions
def propagate(function, inputs, method='auto'):
    """
    Propagate uncertainty through function.
    
    This is the main entry point for uncertainty propagation in M17.
    
    Parameters:
    -----------
    function : callable
        Function to propagate uncertainty through
    inputs : list of UncertainQuantity
        Input uncertain quantities
    method : str
        Propagation method ('analytical', 'numerical', 'monte_carlo', 'auto')
        
    Returns:
    --------
    UncertainQuantity
        Result with propagated uncertainty
        
    Examples:
    --------
    >>> x = UncertainQuantity(2.0, 0.1, "measurement_x")
    >>> y = UncertainQuantity(3.0, 0.2, "measurement_y") 
    >>> result = propagate(lambda vals: vals[0] * vals[1], [x, y])
    >>> print(f"x * y = {result.value:.3f} ± {result.uncertainty:.3f}")
    """
    if method == 'auto':
        # Choose method based on function complexity and input size
        if len(inputs) <= 10:
            method = 'analytical'
        else:
            method = 'numerical'
    
    if method == 'analytical':
        return propagate_analytical(function, inputs)
    elif method == 'numerical':
        return propagate_numerical(function, inputs)
    else:
        raise ValueError(f"Unknown propagation method: {method}")

def system_info() -> Dict[str, Any]:
    """
    Get system information and capabilities.
    
    Returns:
    --------
    dict
        System capabilities and configuration
    """
    import time
    
    return {
        'version': __version__,
        'capabilities': CAPABILITIES.copy(),
        'performance_config': {
            'num_workers': performance_config.num_workers,
            'batch_size': performance_config.batch_size,
            'use_simd': performance_config.use_simd,
            'use_gpu': performance_config.use_gpu,
        },
        'physical_constants': dict(PhysicalConstants.__dict__.items()),
        'timestamp': time.time(),
    }

def validate_installation() -> bool:
    """
    Validate M17 installation and run basic tests.
    
    Returns:
    --------
    bool
        True if all tests pass
    """
    try:
        # Test uncertainty propagation
        x = UncertainQuantity(2.0, 0.1, "test")
        result = propagate(lambda vals: vals[0]**2, [x])
        
        expected_value = 4.0
        expected_uncertainty = 2 * 2.0 * 0.1  # d/dx(x²) = 2x, so σ = 2*x*σ_x
        
        if abs(result.value - expected_value) > 1e-10:
            logger.error(f"Uncertainty test failed: value {result.value} != {expected_value}")
            return False
            
        if abs(result.uncertainty - expected_uncertainty) > 1e-10:
            logger.error(f"Uncertainty test failed: uncertainty {result.uncertainty} != {expected_uncertainty}")
            return False
        
        # Test physics
        state = OrbitalState([7e6, 0, 0], [0, 7500, 0], 0.0)  # Circular orbit
        if abs(state.semimajor_axis() - 7e6) > 1e3:
            logger.error("Physics test failed: incorrect orbital elements")
            return False
        
        logger.info("All validation tests passed")
        return True
        
    except Exception as e:
        logger.error(f"Validation failed: {e}")
        return False

# System information logging
logger.info(f"ChiragRathi M17 v{__version__} initialized")
logger.info(f"System capabilities: {CAPABILITIES}")
logger.info(f"Performance config: {performance_config.num_workers} workers, "
           f"SIMD: {performance_config.use_simd}, GPU: {performance_config.use_gpu}")

# Warning for suboptimal performance
if not CAPABILITIES['simd']:
    warnings.warn(
        "SIMD instructions not available. Performance will be reduced. "
        "Consider using a CPU with AVX2 or AVX-512 support.",
        RuntimeWarning
    )

if not CAPABILITIES['openmp']:
    warnings.warn(
        "OpenMP not available. Parallel performance will be limited.",
        RuntimeWarning
    )

# Module exports
__all__ = [
    # Version info
    '__version__', '__author__', '__email__',
    
    # Core classes
    'UncertainQuantity', 'UncertaintyPropagator', 'RealTimeProcessor',
    'OrbitalState', 'PhysicsEngine', 'ScientificMemory',
    
    # Main functions
    'propagate', 'analyze_orbit', 'detect_anomalies',
    
    # Utilities
    'system_info', 'validate_installation', 'setup_logging',
    'performance_config', 'CAPABILITIES',
    
    # Configuration
    'Config', 'PhysicalConstants', 'PerformanceTargets',
    
    # Data handling
    'load_astronomical_catalog', 'load_satellite_data',
    
    # Visualization
    'plot_uncertainty', 'plot_orbit', 'plot_anomaly_timeline',
]
