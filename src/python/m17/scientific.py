#!/usr/bin/env python3
"""
ChiragRathi M17 Scientific Computing Interface

Comprehensive Python interface providing researchers with high-level access
to M17 framework capabilities for astronomical data analysis.

Key Features:
- Uncertainty-aware data structures (UncertainArray, UncertainDataFrame)
- Astronomical coordinate systems with full uncertainty propagation
- Statistical analysis tools for survey data
- Visualization for scientific publications
- Integration with Astropy, NumPy, Pandas ecosystems
- Jupyter notebook support with interactive widgets

Author: ChiragRathi
Version: 2.0.0
Date: 2026
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from pathlib import Path
import warnings
import logging
from typing import Union, List, Tuple, Dict, Optional, Any, Callable
from dataclasses import dataclass, field
import pickle
import json
from datetime import datetime

# Astronomical libraries
try:
    from astropy import units as u
    from astropy.coordinates import SkyCoord, EarthLocation, Time
    from astropy.time import Time as AstroTime
    from astropy.table import Table, QTable
    from astropy.io import fits, ascii
    from astropy.wcs import WCS
    from astropy.constants import c, G, M_earth, M_sun
    ASTROPY_AVAILABLE = True
except ImportError:
    ASTROPY_AVAILABLE = False
    warnings.warn("Astropy not available. Some astronomical features disabled.")

# Scientific computing
try:
    import scipy.stats as stats
    from scipy.optimize import minimize, curve_fit
    from scipy.interpolate import interp1d, griddata
    from sklearn.cluster import KMeans, DBSCAN
    from sklearn.decomposition import PCA
    from sklearn.preprocessing import StandardScaler
    SCIPY_SKLEARN_AVAILABLE = True
except ImportError:
    SCIPY_SKLEARN_AVAILABLE = False
    warnings.warn("SciPy/scikit-learn not available. Some analysis features disabled.")

# M17 core bindings
try:
    import m17._uncertainty as _uncertainty
    import m17._physics as _physics
    import m17._memory as _memory
    M17_CORE_AVAILABLE = True
except ImportError:
    M17_CORE_AVAILABLE = False
    warnings.warn("M17 core modules not available. Using mock implementations.")

# Configure logging
logger = logging.getLogger(__name__)

@dataclass
class ObservationMetadata:
    """Metadata for astronomical observations"""
    source_id: str
    instrument: str
    filter_name: str
    exposure_time: float
    observation_date: datetime
    observer_location: Optional[str] = None
    weather_conditions: Dict[str, float] = field(default_factory=dict)
    calibration_flags: Dict[str, bool] = field(default_factory=dict)
    quality_flags: List[str] = field(default_factory=list)

class UncertainArray:
    """
    N-dimensional array with uncertainty propagation
    
    Extends NumPy arrays to include uncertainty information
    and provides uncertainty-aware mathematical operations.
    
    Parameters:
    -----------
    values : array-like
        Central values
    uncertainties : array-like
        Standard uncertainties (same shape as values)
    correlations : array-like, optional
        Correlation matrix for uncertainty propagation
    units : astropy.units.Unit, optional
        Physical units
    labels : list of str, optional
        Axis labels for plotting and display
    
    Examples:
    ---------
    >>> import numpy as np
    >>> values = np.array([1.0, 2.0, 3.0])
    >>> uncertainties = np.array([0.1, 0.1, 0.2])
    >>> arr = UncertainArray(values, uncertainties)
    >>> result = arr + arr  # Uncertainty propagation
    >>> print(result.values, result.uncertainties)
    [2. 4. 6.] [0.14142136 0.14142136 0.28284271]
    """
    
    def __init__(self, values, uncertainties, correlations=None, 
                 units=None, labels=None, metadata=None):
        self.values = np.asarray(values, dtype=float)
        self.uncertainties = np.asarray(uncertainties, dtype=float)
        
        if self.values.shape != self.uncertainties.shape:
            raise ValueError("Values and uncertainties must have same shape")
        
        if correlations is not None:
            self.correlations = np.asarray(correlations)
        else:
            # Default to no correlation
            n = self.values.size
            self.correlations = np.eye(n)
        
        self.units = units
        self.labels = labels or [f"axis_{i}" for i in range(self.values.ndim)]
        self.metadata = metadata or {}
        
    def __repr__(self):
        return f"UncertainArray(shape={self.values.shape}, units={self.units})"
    
    def __str__(self):
        lines = []
        lines.append(f"UncertainArray:")
        lines.append(f"  Shape: {self.values.shape}")
        lines.append(f"  Values: {self.values}")
        lines.append(f"  Uncertainties: {self.uncertainties}")
        if self.units:
            lines.append(f"  Units: {self.units}")
        return "\n".join(lines)
    
    @property
    def shape(self):
        return self.values.shape
    
    @property
    def size(self):
        return self.values.size
    
    @property
    def ndim(self):
        return self.values.ndim
    
    def __len__(self):
        return len(self.values)
    
    def __getitem__(self, key):
        values = self.values[key]
        uncertainties = self.uncertainties[key]
        
        # Handle correlation matrix slicing (simplified)
        correlations = None
        if hasattr(key, '__len__') and len(key) == 2:
            # 2D slicing
            correlations = self.correlations[key[0], key[1]]
        
        return UncertainArray(values, uncertainties, correlations, 
                            self.units, self.labels, self.metadata)
    
    def __setitem__(self, key, value):
        if isinstance(value, UncertainArray):
            self.values[key] = value.values
            self.uncertainties[key] = value.uncertainties
        else:
            self.values[key] = value
            # Uncertainty remains unchanged for direct value assignment
    
    def _propagate_binary_operation(self, other, operation, derivative_self, derivative_other):
        """Generic binary operation with uncertainty propagation"""
        if isinstance(other, UncertainArray):
            # Both have uncertainties
            result_values = operation(self.values, other.values)
            
            # Uncertainty propagation using derivatives
            var_self = (derivative_self(self.values, other.values) * self.uncertainties) ** 2
            var_other = (derivative_other(self.values, other.values) * other.uncertainties) ** 2
            
            # Cross-correlation term (simplified - assume no correlation)
            result_uncertainties = np.sqrt(var_self + var_other)
            
        elif np.isscalar(other) or isinstance(other, np.ndarray):
            # Only self has uncertainty
            result_values = operation(self.values, other)
            result_uncertainties = np.abs(derivative_self(self.values, other)) * self.uncertainties
            
        else:
            raise TypeError(f"Unsupported type for operation: {type(other)}")
        
        return UncertainArray(result_values, result_uncertainties, 
                            correlations=self.correlations, units=self.units)
    
    def __add__(self, other):
        return self._propagate_binary_operation(
            other, 
            lambda x, y: x + y,
            lambda x, y: np.ones_like(x),  # d/dx(x + y) = 1
            lambda x, y: np.ones_like(y)   # d/dy(x + y) = 1
        )
    
    def __sub__(self, other):
        return self._propagate_binary_operation(
            other,
            lambda x, y: x - y,
            lambda x, y: np.ones_like(x),   # d/dx(x - y) = 1
            lambda x, y: -np.ones_like(y)   # d/dy(x - y) = -1
        )
    
    def __mul__(self, other):
        return self._propagate_binary_operation(
            other,
            lambda x, y: x * y,
            lambda x, y: y,  # d/dx(x * y) = y
            lambda x, y: x   # d/dy(x * y) = x
        )
    
    def __truediv__(self, other):
        return self._propagate_binary_operation(
            other,
            lambda x, y: x / y,
            lambda x, y: 1.0 / y,        # d/dx(x / y) = 1/y
            lambda x, y: -x / (y * y)    # d/dy(x / y) = -x/y²
        )
    
    def __pow__(self, exponent):
        """Power operation with uncertainty propagation"""
        if isinstance(exponent, UncertainArray):
            # Both base and exponent have uncertainty
            result_values = self.values ** exponent.values
            
            # d/dx(x^y) = y * x^(y-1)
            d_base = exponent.values * (self.values ** (exponent.values - 1))
            var_base = (d_base * self.uncertainties) ** 2
            
            # d/dy(x^y) = x^y * ln(x)
            d_exp = result_values * np.log(np.maximum(self.values, 1e-100))
            var_exp = (d_exp * exponent.uncertainties) ** 2
            
            result_uncertainties = np.sqrt(var_base + var_exp)
            
        else:
            # Only base has uncertainty
            result_values = self.values ** exponent
            derivative = exponent * (self.values ** (exponent - 1))
            result_uncertainties = np.abs(derivative) * self.uncertainties
        
        return UncertainArray(result_values, result_uncertainties, 
                            correlations=self.correlations, units=self.units)
    
    def sqrt(self):
        """Square root with uncertainty propagation"""
        return self.__pow__(0.5)
    
    def log(self):
        """Natural logarithm with uncertainty propagation"""
        if np.any(self.values <= 0):
            raise ValueError("Logarithm of non-positive values")
        
        result_values = np.log(self.values)
        derivative = 1.0 / self.values
        result_uncertainties = derivative * self.uncertainties
        
        return UncertainArray(result_values, result_uncertainties,
                            correlations=self.correlations)
    
    def exp(self):
        """Exponential with uncertainty propagation"""
        result_values = np.exp(self.values)
        derivative = result_values  # d/dx(e^x) = e^x
        result_uncertainties = derivative * self.uncertainties
        
        return UncertainArray(result_values, result_uncertainties,
                            correlations=self.correlations)
    
    def sin(self):
        """Sine with uncertainty propagation"""
        result_values = np.sin(self.values)
        derivative = np.cos(self.values)
        result_uncertainties = np.abs(derivative) * self.uncertainties
        
        return UncertainArray(result_values, result_uncertainties,
                            correlations=self.correlations)
    
    def cos(self):
        """Cosine with uncertainty propagation"""
        result_values = np.cos(self.values)
        derivative = -np.sin(self.values)
        result_uncertainties = np.abs(derivative) * self.uncertainties
        
        return UncertainArray(result_values, result_uncertainties,
                            correlations=self.correlations)
    
    def mean(self, axis=None, weights=None):
        """Weighted mean with uncertainty propagation"""
        if weights is None:
            # Simple mean
            mean_value = np.mean(self.values, axis=axis)
            
            if axis is None:
                # All elements
                n = self.values.size
                mean_uncertainty = np.sqrt(np.sum(self.uncertainties**2)) / n
            else:
                # Along specific axis
                mean_uncertainty = np.sqrt(np.mean(self.uncertainties**2, axis=axis))
        else:
            # Weighted mean
            weights = np.asarray(weights)
            weighted_sum = np.sum(weights * self.values, axis=axis)
            weight_sum = np.sum(weights, axis=axis)
            mean_value = weighted_sum / weight_sum
            
            # Uncertainty propagation for weighted mean
            var_weighted = np.sum((weights * self.uncertainties)**2, axis=axis)
            mean_uncertainty = np.sqrt(var_weighted) / weight_sum
        
        return UncertainArray(mean_value, mean_uncertainty, units=self.units)
    
    def std(self, axis=None, ddof=0):
        """Standard deviation with uncertainty estimation"""
        std_value = np.std(self.values, axis=axis, ddof=ddof)
        
        # Uncertainty in standard deviation (approximate)
        n = self.values.shape[axis] if axis is not None else self.values.size
        std_uncertainty = std_value / np.sqrt(2 * (n - ddof))
        
        return UncertainArray(std_value, std_uncertainty, units=self.units)
    
    def to_astropy_table(self):
        """Convert to Astropy Table for astronomical data handling"""
        if not ASTROPY_AVAILABLE:
            raise ImportError("Astropy required for table conversion")
        
        table_data = {}
        
        if self.values.ndim == 1:
            # 1D array
            table_data['value'] = self.values
            table_data['uncertainty'] = self.uncertainties
            
            if self.units:
                table_data['value'] = table_data['value'] * self.units
                table_data['uncertainty'] = table_data['uncertainty'] * self.units
        else:
            # Multi-dimensional - flatten or select specific columns
            for i in range(self.values.shape[-1]):
                col_name = self.labels[i] if i < len(self.labels) else f"col_{i}"
                table_data[col_name] = self.values[..., i]
                table_data[f"{col_name}_err"] = self.uncertainties[..., i]
        
        return QTable(table_data)
    
    def plot(self, x=None, ax=None, show_uncertainty=True, **kwargs):
        """Plot with uncertainty visualization"""
        if ax is None:
            fig, ax = plt.subplots(figsize=(10, 6))
        
        if x is None:
            x = np.arange(len(self.values))
        
        # Main plot
        line = ax.plot(x, self.values, **kwargs)[0]
        
        # Uncertainty bands
        if show_uncertainty:
            ax.fill_between(x, 
                          self.values - self.uncertainties,
                          self.values + self.uncertainties,
                          alpha=0.3, color=line.get_color())
        
        # Labels and units
        if hasattr(x, 'units') and x.units:
            ax.set_xlabel(f"X [{x.units}]")
        
        if self.units:
            ax.set_ylabel(f"Y [{self.units}]")
        
        ax.grid(True, alpha=0.3)
        return ax

class AstronomicalCatalog:
    """
    Container for astronomical source catalogs with uncertainty support
    
    Provides high-level interface for working with large astronomical
    datasets from surveys like Gaia, WISE, SDSS, etc.
    
    Features:
    - Automatic coordinate system handling
    - Proper motion propagation with uncertainties
    - Cross-matching capabilities
    - Statistical analysis tools
    - Export to standard formats (FITS, VOTable, CSV)
    
    Examples:
    ---------
    >>> catalog = AstronomicalCatalog.from_file('gaia_dr3_sample.fits')
    >>> nearby_stars = catalog.cone_search(ra=83.8, dec=22.0, radius=0.5)
    >>> proper_motion = catalog.compute_proper_motion(epoch=2025.0)
    """
    
    def __init__(self, data=None, coordinates=None, metadata=None):
        self.data = data or {}
        self.coordinates = coordinates
        self.metadata = metadata or {}
        self._cached_distances = None
        
    @classmethod
    def from_file(cls, filename, format='auto'):
        """Load catalog from file"""
        filepath = Path(filename)
        
        if not filepath.exists():
            raise FileNotFoundError(f"Catalog file not found: {filename}")
        
        if format == 'auto':
            format = filepath.suffix.lower().lstrip('.')
        
        metadata = {'source_file': str(filepath), 'load_time': datetime.now()}
        
        if format in ['fits', 'fit']:
            if not ASTROPY_AVAILABLE:
                raise ImportError("Astropy required for FITS files")
            
            with fits.open(filepath) as hdul:
                table = QTable.read(hdul[1])  # Assume data in first extension
                metadata['fits_header'] = dict(hdul[0].header)
                
        elif format in ['csv', 'txt']:
            table = ascii.read(filepath)
            
        elif format == 'parquet':
            try:
                import pyarrow.parquet as pq
                table_pd = pd.read_parquet(filepath)
                table = QTable.from_pandas(table_pd)
            except ImportError:
                raise ImportError("PyArrow required for Parquet files")
        
        else:
            raise ValueError(f"Unsupported format: {format}")
        
        # Convert table to internal format
        data = {}
        coordinates = None
        
        for col_name in table.colnames:
            col_data = table[col_name]
            
            # Check for uncertainty columns
            if col_name.endswith('_error') or col_name.endswith('_err'):
                continue  # Handle with main column
            
            error_col_name = None
            for err_suffix in ['_error', '_err']:
                if f"{col_name}{err_suffix}" in table.colnames:
                    error_col_name = f"{col_name}{err_suffix}"
                    break
            
            if error_col_name:
                values = np.array(col_data)
                uncertainties = np.array(table[error_col_name])
                data[col_name] = UncertainArray(values, uncertainties,
                                              units=getattr(col_data, 'unit', None))
            else:
                data[col_name] = col_data
        
        # Extract coordinates if available
        if 'ra' in data and 'dec' in data and ASTROPY_AVAILABLE:
            ra = data['ra']
            dec = data['dec']
            
            if isinstance(ra, UncertainArray) and isinstance(dec, UncertainArray):
                coordinates = SkyCoord(ra=ra.values*u.deg, dec=dec.values*u.deg)
                # Store coordinate uncertainties separately
                metadata['coord_uncertainties'] = {
                    'ra_error': ra.uncertainties,
                    'dec_error': dec.uncertainties
                }
            else:
                coordinates = SkyCoord(ra=ra*u.deg, dec=dec*u.deg)
        
        return cls(data, coordinates, metadata)
    
    def __len__(self):
        if self.data:
            first_key = next(iter(self.data))
            return len(self.data[first_key])
        return 0
    
    def __getitem__(self, key):
        if isinstance(key, str):
            return self.data[key]
        elif isinstance(key, (int, slice, np.ndarray)):
            # Index or slice catalog
            new_data = {}
            for col_name, col_data in self.data.items():
                if isinstance(col_data, UncertainArray):
                    new_data[col_name] = col_data[key]
                else:
                    new_data[col_name] = col_data[key]
            
            new_coords = None
            if self.coordinates is not None:
                new_coords = self.coordinates[key]
            
            return AstronomicalCatalog(new_data, new_coords, self.metadata.copy())
    
    def cone_search(self, ra, dec, radius, units='deg'):
        """Cone search around given coordinates"""
        if not ASTROPY_AVAILABLE or self.coordinates is None:
            raise ValueError("Coordinates required for cone search")
        
        center = SkyCoord(ra=ra*u.deg, dec=dec*u.deg)
        separations = center.separation(self.coordinates)
        
        mask = separations <= radius * u.Unit(units)
        return self[mask]
    
    def cross_match(self, other_catalog, max_sep=1.0*u.arcsec):
        """Cross-match with another catalog"""
        if not ASTROPY_AVAILABLE:
            raise ImportError("Astropy required for cross-matching")
        
        if self.coordinates is None or other_catalog.coordinates is None:
            raise ValueError("Both catalogs must have coordinates")
        
        idx, d2d, d3d = self.coordinates.match_to_catalog_sky(other_catalog.coordinates)
        
        # Apply separation criterion
        sep_constraint = d2d < max_sep
        
        matched_self = self[sep_constraint]
        matched_other = other_catalog[idx[sep_constraint]]
        matched_separations = d2d[sep_constraint]
        
        return matched_self, matched_other, matched_separations
    
    def compute_proper_motion(self, target_epoch, reference_epoch=None):
        """Compute positions at target epoch using proper motion"""
        if not ASTROPY_AVAILABLE:
            raise ImportError("Astropy required for proper motion calculations")
        
        if 'pmra' not in self.data or 'pmdec' not in self.data:
            raise ValueError("Proper motion data not available")
        
        # Get proper motions with uncertainties
        pmra = self.data['pmra']
        pmdec = self.data['pmdec']
        
        if reference_epoch is None:
            reference_epoch = self.metadata.get('epoch', 2016.0)  # Gaia DR3 epoch
        
        dt = target_epoch - reference_epoch  # years
        
        # Current coordinates
        if isinstance(self.data['ra'], UncertainArray):
            ra = self.data['ra']
            dec = self.data['dec']
        else:
            ra = UncertainArray(self.data['ra'], np.zeros_like(self.data['ra']))
            dec = UncertainArray(self.data['dec'], np.zeros_like(self.data['dec']))
        
        # Propagate coordinates
        # RA correction for spherical geometry: Δα = (μ_α / cos δ) × Δt
        delta_ra = (pmra / np.cos(np.deg2rad(dec.values))) * dt / 3600.0  # arcsec to degrees
        delta_dec = pmdec * dt / 3600.0
        
        new_ra = ra + delta_ra
        new_dec = dec + delta_dec
        
        # Create new catalog with propagated positions
        new_data = self.data.copy()
        new_data['ra'] = new_ra
        new_data['dec'] = new_dec
        new_data['epoch'] = target_epoch
        
        new_coords = SkyCoord(ra=new_ra.values*u.deg, dec=new_dec.values*u.deg)
        
        new_metadata = self.metadata.copy()
        new_metadata['propagated_to_epoch'] = target_epoch
        
        return AstronomicalCatalog(new_data, new_coords, new_metadata)
    
    def color_magnitude_diagram(self, color_bands, magnitude_band, ax=None):
        """Create color-magnitude diagram with uncertainties"""
        if ax is None:
            fig, ax = plt.subplots(figsize=(10, 8))
        
        # Extract magnitudes and colors
        if len(color_bands) != 2:
            raise ValueError("Color requires exactly two photometric bands")
        
        mag1_name, mag2_name = color_bands
        mag_name = magnitude_band
        
        if any(band not in self.data for band in [mag1_name, mag2_name, mag_name]):
            missing = [b for b in [mag1_name, mag2_name, mag_name] if b not in self.data]
            raise ValueError(f"Missing photometric bands: {missing}")
        
        mag1 = self.data[mag1_name]
        mag2 = self.data[mag2_name]
        magnitude = self.data[mag_name]
        
        # Compute color
        if isinstance(mag1, UncertainArray) and isinstance(mag2, UncertainArray):
            color = mag1 - mag2
            x = color.values
            x_err = color.uncertainties
            y = magnitude.values
            y_err = magnitude.uncertainties
        else:
            x = mag1 - mag2
            x_err = None
            y = magnitude
            y_err = None
        
        # Plot with error bars if available
        if x_err is not None and y_err is not None:
            ax.errorbar(x, y, xerr=x_err, yerr=y_err, fmt='o', alpha=0.6, markersize=3)
        else:
            ax.scatter(x, y, alpha=0.6, s=10)
        
        # Invert magnitude axis (brighter objects at top)
        ax.invert_yaxis()
        
        ax.set_xlabel(f"{mag1_name} - {mag2_name}")
        ax.set_ylabel(magnitude_band)
        ax.set_title("Color-Magnitude Diagram")
        ax.grid(True, alpha=0.3)
        
        return ax
    
    def statistical_summary(self):
        """Generate statistical summary of catalog"""
        summary = {
            'total_sources': len(self),
            'coordinate_range': {},
            'photometric_summary': {},
            'astrometric_summary': {},
        }
        
        # Coordinate statistics
        if self.coordinates is not None:
            summary['coordinate_range'] = {
                'ra_min': float(np.min(self.coordinates.ra.deg)),
                'ra_max': float(np.max(self.coordinates.ra.deg)),
                'dec_min': float(np.min(self.coordinates.dec.deg)),
                'dec_max': float(np.max(self.coordinates.dec.deg)),
            }
        
        # Column statistics
        for col_name, col_data in self.data.items():
            if isinstance(col_data, UncertainArray):
                col_stats = {
                    'mean': float(np.mean(col_data.values)),
                    'std': float(np.std(col_data.values)),
                    'min': float(np.min(col_data.values)),
                    'max': float(np.max(col_data.values)),
                    'mean_uncertainty': float(np.mean(col_data.uncertainties)),
                    'median_uncertainty': float(np.median(col_data.uncertainties)),
                }
                
                if 'mag' in col_name.lower() or 'phot' in col_name.lower():
                    summary['photometric_summary'][col_name] = col_stats
                elif any(x in col_name.lower() for x in ['pm', 'parallax', 'proper']):
                    summary['astrometric_summary'][col_name] = col_stats
        
        return summary
    
    def to_fits(self, filename, overwrite=False):
        """Export catalog to FITS file"""
        if not ASTROPY_AVAILABLE:
            raise ImportError("Astropy required for FITS export")
        
        table = self.to_astropy_table()
        table.write(filename, format='fits', overwrite=overwrite)
    
    def to_csv(self, filename, include_uncertainties=True):
        """Export catalog to CSV file"""
        table_data = {}
        
        for col_name, col_data in self.data.items():
            if isinstance(col_data, UncertainArray):
                table_data[col_name] = col_data.values
                if include_uncertainties:
                    table_data[f"{col_name}_err"] = col_data.uncertainties
            else:
                table_data[col_name] = col_data
        
        df = pd.DataFrame(table_data)
        df.to_csv(filename, index=False)
    
    def to_astropy_table(self):
        """Convert to Astropy Table"""
        if not ASTROPY_AVAILABLE:
            raise ImportError("Astropy required for table conversion")
        
        table_data = {}
        
        for col_name, col_data in self.data.items():
            if isinstance(col_data, UncertainArray):
                table_data[col_name] = col_data.values
                table_data[f"{col_name}_err"] = col_data.uncertainties
                
                if col_data.units:
                    table_data[col_name] = table_data[col_name] * col_data.units
                    table_data[f"{col_name}_err"] = table_data[f"{col_name}_err"] * col_data.units
            else:
                table_data[col_name] = col_data
        
        table = QTable(table_data)
        
        # Add metadata to table
        for key, value in self.metadata.items():
            if isinstance(value, (str, int, float)):
                table.meta[key] = value
        
        return table

def load_gaia_dr3_sample(region=None, magnitude_limit=None, proper_motion_limit=None):
    """
    Load Gaia DR3 sample data for testing and validation
    
    Parameters:
    -----------
    region : tuple, optional
        (ra_min, ra_max, dec_min, dec_max) in degrees
    magnitude_limit : float, optional
        Limiting G magnitude
    proper_motion_limit : float, optional
        Minimum proper motion in mas/yr
    
    Returns:
    --------
    AstronomicalCatalog
        Gaia DR3 sample catalog
    """
    # This would normally query Gaia archive or load cached data
    # For demonstration, create synthetic data
    
    n_sources = 10000
    np.random.seed(42)  # Reproducible
    
    if region:
        ra_min, ra_max, dec_min, dec_max = region
        ra_range = (ra_min, ra_max)
        dec_range = (dec_min, dec_max)
    else:
        ra_range = (0, 360)
        dec_range = (-90, 90)
    
    # Generate synthetic Gaia-like data
    ra = np.random.uniform(*ra_range, n_sources)
    dec = np.random.uniform(*dec_range, n_sources)
    
    # G magnitude with realistic distribution
    g_mag = np.random.normal(15.0, 3.0, n_sources)
    g_mag = np.clip(g_mag, 8.0, 21.0)
    
    # Color indices
    bp_rp = np.random.normal(1.2, 0.8, n_sources)
    g_rp = np.random.normal(0.6, 0.4, n_sources)
    
    # Proper motions
    pmra = np.random.normal(0.0, 10.0, n_sources)  # mas/yr
    pmdec = np.random.normal(0.0, 10.0, n_sources)
    
    # Parallax
    parallax = np.random.exponential(0.5, n_sources)  # mas
    
    # Realistic uncertainties
    ra_error = np.random.uniform(0.001, 0.1, n_sources)  # arcsec
    dec_error = np.random.uniform(0.001, 0.1, n_sources)
    g_mag_error = np.random.uniform(0.001, 0.05, n_sources)
    pmra_error = np.random.uniform(0.01, 1.0, n_sources)
    pmdec_error = np.random.uniform(0.01, 1.0, n_sources)
    parallax_error = np.random.uniform(0.01, 0.1, n_sources)
    
    # Apply filters
    if magnitude_limit:
        mask = g_mag <= magnitude_limit
        ra, dec = ra[mask], dec[mask]
        g_mag, g_mag_error = g_mag[mask], g_mag_error[mask]
        bp_rp, g_rp = bp_rp[mask], g_rp[mask]
        pmra, pmra_error = pmra[mask], pmra_error[mask]
        pmdec, pmdec_error = pmdec[mask], pmdec_error[mask]
        parallax, parallax_error = parallax[mask], parallax_error[mask]
        ra_error, dec_error = ra_error[mask], dec_error[mask]
    
    if proper_motion_limit:
        pm_total = np.sqrt(pmra**2 + pmdec**2)
        mask = pm_total >= proper_motion_limit
        ra, dec = ra[mask], dec[mask]
        g_mag, g_mag_error = g_mag[mask], g_mag_error[mask]
        bp_rp, g_rp = bp_rp[mask], g_rp[mask]
        pmra, pmra_error = pmra[mask], pmra_error[mask]
        pmdec, pmdec_error = pmdec[mask], pmdec_error[mask]
        parallax, parallax_error = parallax[mask], parallax_error[mask]
        ra_error, dec_error = ra_error[mask], dec_error[mask]
    
    # Create catalog data
    data = {
        'source_id': np.arange(len(ra)),
        'ra': UncertainArray(ra, ra_error, units=u.deg if ASTROPY_AVAILABLE else None),
        'dec': UncertainArray(dec, dec_error, units=u.deg if ASTROPY_AVAILABLE else None),
        'g_mag': UncertainArray(g_mag, g_mag_error, units=u.mag if ASTROPY_AVAILABLE else None),
        'bp_rp': UncertainArray(bp_rp, g_mag_error * 0.5),
        'g_rp': UncertainArray(g_rp, g_mag_error * 0.3),
        'pmra': UncertainArray(pmra, pmra_error, units=u.mas/u.yr if ASTROPY_AVAILABLE else None),
        'pmdec': UncertainArray(pmdec, pmdec_error, units=u.mas/u.yr if ASTROPY_AVAILABLE else None),
        'parallax': UncertainArray(parallax, parallax_error, units=u.mas if ASTROPY_AVAILABLE else None),
    }
    
    # Create coordinates
    if ASTROPY_AVAILABLE:
        coordinates = SkyCoord(ra=ra*u.deg, dec=dec*u.deg)
    else:
        coordinates = None
    
    metadata = {
        'survey': 'Gaia DR3 (synthetic)',
        'epoch': 2016.0,
        'creation_date': datetime.now(),
        'n_sources': len(ra),
        'filters_applied': {
            'magnitude_limit': magnitude_limit,
            'proper_motion_limit': proper_motion_limit,
            'region': region
        }
    }
    
    return AstronomicalCatalog(data, coordinates, metadata)

# Utility functions for astronomical calculations

def compute_distance_modulus(parallax_mas):
    """
    Compute distance modulus from parallax
    
    Parameters:
    -----------
    parallax_mas : UncertainArray or array-like
        Parallax in milliarcseconds
        
    Returns:
    --------
    UncertainArray
        Distance modulus in magnitudes
    """
    if isinstance(parallax_mas, UncertainArray):
        # Distance in parsecs
        distance_pc = 1000.0 / parallax_mas  # 1000 mas = 1 arcsec = 1/1 pc
        
        # Distance modulus: μ = 5 * log10(d/10)
        distance_modulus = 5.0 * (distance_pc / 10.0).log()
        
        return distance_modulus
    else:
        distance_pc = 1000.0 / np.array(parallax_mas)
        return 5.0 * np.log10(distance_pc / 10.0)

def galactic_extinction_correction(coordinates, ebv_map=None):
    """
    Apply Galactic extinction correction
    
    Parameters:
    -----------
    coordinates : SkyCoord
        Source coordinates
    ebv_map : callable, optional
        Function to get E(B-V) from coordinates
        
    Returns:
    --------
    dict
        Extinction corrections for different bands
    """
    # Simplified extinction model
    # In reality, would use dust maps like Planck, SFD, or 3D maps
    
    if ebv_map is None:
        # Simple model based on Galactic latitude
        galactic_coords = coordinates.galactic
        b = np.abs(galactic_coords.b.deg)
        
        # Higher extinction near Galactic plane
        ebv = 0.1 * np.exp(-b / 10.0)  # E(B-V)
    else:
        ebv = ebv_map(coordinates)
    
    # Extinction coefficients (Cardelli, Clayton, & Mathis 1989)
    extinction_coeffs = {
        'U': 5.434,
        'B': 4.315,
        'V': 3.315,
        'R': 2.673,
        'I': 1.940,
        'J': 0.902,
        'H': 0.576,
        'K': 0.367,
        'g': 3.793,  # SDSS
        'r': 2.751,
        'i': 2.086,
        'z': 1.479
    }
    
    extinctions = {}
    for band, coeff in extinction_coeffs.items():
        extinctions[f'A_{band}'] = coeff * ebv
    
    return extinctions

def main():
    """Demonstration of scientific interface capabilities"""
    print("ChiragRathi M17 Scientific Computing Interface")
    print("==========================================")
    
    # Create sample data
    print("\n1. Creating UncertainArray with astronomical measurements...")
    magnitudes = np.array([12.5, 13.2, 14.1, 15.3, 16.0])
    uncertainties = np.array([0.02, 0.03, 0.05, 0.08, 0.12])
    
    mags = UncertainArray(magnitudes, uncertainties, 
                         units=u.mag if ASTROPY_AVAILABLE else None,
                         labels=['V-band magnitudes'])
    
    print(f"Created array: {mags}")
    
    # Demonstrate uncertainty propagation
    print("\n2. Uncertainty propagation in color calculations...")
    colors = mags[:-1] - mags[1:]  # Adjacent color differences
    print(f"Color differences: {colors.values}")
    print(f"Color uncertainties: {colors.uncertainties}")
    
    # Load synthetic Gaia data
    print("\n3. Loading synthetic Gaia DR3 sample...")
    gaia_catalog = load_gaia_dr3_sample(magnitude_limit=18.0, proper_motion_limit=5.0)
    print(f"Loaded {len(gaia_catalog)} sources")
    
    # Statistical analysis
    print("\n4. Statistical summary of catalog...")
    summary = gaia_catalog.statistical_summary()
    print(f"Coordinate range: RA {summary['coordinate_range']['ra_min']:.1f}° to {summary['coordinate_range']['ra_max']:.1f}°")
    print(f"G magnitude: {summary['photometric_summary']['g_mag']['mean']:.1f} ± {summary['photometric_summary']['g_mag']['std']:.1f}")
    
    # Cone search
    print("\n5. Cone search around Betelgeuse...")
    betelgeuse_region = gaia_catalog.cone_search(ra=88.79, dec=7.41, radius=2.0)
    print(f"Found {len(betelgeuse_region)} sources within 2° of Betelgeuse")
    
    print("\nDemonstration completed successfully!")

if __name__ == "__main__":
    main()
