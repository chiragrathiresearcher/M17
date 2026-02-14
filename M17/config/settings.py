# config/settings.py
import os
from pathlib import Path

# Project paths
BASE_DIR = Path(__file__).resolve().parent.parent
DATA_DIR = BASE_DIR / "data"
RAW_DATA_DIR = DATA_DIR / "raw"
PROCESSED_DATA_DIR = DATA_DIR / "processed"
OUTPUTS_DIR = DATA_DIR / "outputs"

# Create directories
for directory in [DATA_DIR, RAW_DATA_DIR, PROCESSED_DATA_DIR, OUTPUTS_DIR]:
    directory.mkdir(parents=True, exist_ok=True)

# API Configuration
CELESTRAK_URL = "https://celestrak.org/NORAD/elements/gp.php"
SPACETRACK_URL = "https://www.space-track.org/ajaxauth/login"

# Physics Constants
SPEED_OF_LIGHT = 299792458.0  # m/s
EARTH_RADIUS = 6378137.0  # WGS84 equatorial radius (m)
EARTH_MASS = 5.972e24  # kg
GRAVITATIONAL_CONSTANT = 6.67430e-11  # m^3 kg^-1 s^-2

# Research Parameters
DEFAULT_SAMPLES = 10000  # Monte Carlo samples
TRUST_THRESHOLD = 0.5  # Default trust threshold
CACHE_DURATION = 3600  # 1 hour cache duration
