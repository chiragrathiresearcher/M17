# src/k20_physics.py
"""
K20: Physics Core - Multi-body simulation engine
Purpose: Prepare scientifically validated orbital and galactic states for K19
"""

import numpy as np
import datetime
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Tuple, Any
import pickle
import json
from sgp4.api import Satrec, jday
from sgp4 import exporter
from config.settings import EARTH_RADIUS, EARTH_MASS, GRAVITATIONAL_CONSTANT


@dataclass
class OrbitalState:
    """Primary data structure for K20 → K19 compatibility"""
    
    position: np.ndarray  # [x, y, z] in meters (ECI frame)
    velocity: np.ndarray  # [vx, vy, vz] in m/s
    mass: float           # kg
    name: str
    timestamp: datetime.datetime = field(default_factory=datetime.datetime.now)
    source: str = "physics_simulation"  # "tle", "manual", "simulation"
    uncertainty: Optional[Dict[str, float]] = field(default_factory=dict)
    metadata: Dict[str, Any] = field(default_factory=dict)

    def __post_init__(self):
        # Ensure numpy arrays
        self.position = np.array(self.position, dtype=np.float64)
        self.velocity = np.array(self.velocity, dtype=np.float64)

        # Initialize default uncertainty if not provided
        if not self.uncertainty:
            self.uncertainty = {
                'position': 1000.0,  # 1km default
                'velocity': 1.0,     # 1 m/s default
                'mass': self.mass * 0.01  # 1% default
            }

    @property
    def velocity_magnitude(self) -> float:
        """Get velocity magnitude in m/s"""
        return float(np.linalg.norm(self.velocity))

    @property
    def position_magnitude(self) -> float:
        """Get position magnitude in m"""
        return float(np.linalg.norm(self.position))

    @property
    def orbital_elements(self) -> Dict:
        """Calculate Keplerian orbital elements (Earth-centered)"""
        mu = 3.986004418e14  # Earth's gravitational parameter (m^3/s^2)
        r = self.position
        v = self.velocity

        # Specific angular momentum
        h = np.cross(r, v)
        h_norm = np.linalg.norm(h)

        # Eccentricity vector
        e_vec = np.cross(v, h) / mu - r / np.linalg.norm(r)
        e = np.linalg.norm(e_vec)

        # Inclination (radians)
        i = np.arccos(h[2] / h_norm) if h_norm > 0 else 0

        # Longitude of ascending node
        n = np.cross([0, 0, 1], h)
        n_norm = np.linalg.norm(n)
        raan = np.arctan2(n[1], n[0]) if n_norm > 0 else 0

        # Energy and semi-major axis
        energy = np.dot(v, v) / 2 - mu / np.linalg.norm(r)
        a = -mu / (2 * energy) if abs(energy) > 1e-10 else float('inf')

        # Altitude
        altitude = np.linalg.norm(r) - EARTH_RADIUS

        return {
            'semi_major_axis': a,
            'eccentricity': e,
            'inclination': np.degrees(i),
            'raan': np.degrees(raan),
            'altitude': altitude,
            'energy': energy
        }

    def to_dict(self) -> Dict:
        """Convert to dictionary for serialization"""
        return {
            'name': self.name,
            'position': self.position.tolist(),
            'velocity': self.velocity.tolist(),
            'mass': self.mass,
            'timestamp': self.timestamp.isoformat(),
            'source': self.source,
            'uncertainty': self.uncertainty,
            'metadata': self.metadata,
            'orbital_elements': self.orbital_elements
        }

    def to_k19_format(self) -> Dict:
        """Convert to K19-compatible format for uncertainty processing"""
        return {
            'value': self.velocity_magnitude,
            'uncertainty': self.uncertainty.get('velocity', 1.0),
            'source': f"K20_{self.name}",
            'metadata': {
                'name': self.name,
                'timestamp': self.timestamp.isoformat(),
                'source': self.source,
                'orbital_elements': self.orbital_elements,
                'position': self.position.tolist(),
                'velocity': self.velocity.tolist(),
                'mass': self.mass
            }
        }


class TLEProcessor:
    """Process Two-Line Element data"""
    
    @staticmethod
    def tle_to_orbital_state(tle_lines: List[str], name: str = "Unknown") -> OrbitalState:
        """Convert TLE to OrbitalState"""
        if len(tle_lines) < 3:
            raise ValueError("TLE must have at least 3 lines")
        
        # Create satellite object from TLE
        satellite = Satrec.twoline2rv(tle_lines[1], tle_lines[2])
        
        # Get current time
        now = datetime.datetime.now()
        jd, fr = jday(now.year, now.month, now.day, 
                      now.hour, now.minute, now.second)
        
        # Calculate position and velocity
        error, position, velocity = satellite.sgp4(jd, fr)
        
        if error != 0:
            raise ValueError(f"TLE propagation error: {error}")
        
        # Convert from km to meters
        position = np.array(position) * 1000  # km to m
        velocity = np.array(velocity) * 1000  # km/s to m/s
        
        # Estimate mass from object type
        mass = TLEProcessor._estimate_mass_from_tle(tle_lines[0])
        
        return OrbitalState(
            position=position,
            velocity=velocity,
            mass=mass,
            name=name,
            timestamp=now,
            source="tle",
            metadata={
                'tle_line0': tle_lines[0],
                'tle_line1': tle_lines[1],
                'tle_line2': tle_lines[2],
                'norad_id': TLEProcessor._extract_norad_id(tle_lines[1])
            }
        )
    
    @staticmethod
    def _estimate_mass_from_tle(name_line: str) -> float:
        """Estimate satellite mass from name/type"""
        name_lower = name_line.lower()
        
        if 'iss' in name_lower or 'zarya' in name_lower:
            return 419725  # ISS mass in kg
        elif 'hst' in name_lower or 'hubble' in name_lower:
            return 11110  # Hubble mass
        elif 'gps' in name_lower:
            return 2000  # GPS satellite
        elif 'starlink' in name_lower:
            return 260  # Starlink v1.0
        elif 'cube' in name_lower:
            return 1.33  # 1U CubeSat
        else:
            return 1000  # Default small satellite
    
    @staticmethod
    def _extract_norad_id(tle_line1: str) -> str:
        """Extract NORAD ID from TLE line 1"""
        # NORAD ID is positions 3-7 in line 1
        return tle_line1[2:7].strip()


class MultiBodySimulator:
    """Newtonian multi-body gravity simulator"""
    
    def __init__(self, G: float = GRAVITATIONAL_CONSTANT):
        self.G = G
        self.bodies: List[OrbitalState] = []
        self.time = 0.0  # Simulation time in seconds
        self.step_count = 0
    
    def add_body(self, body: OrbitalState) -> None:
        """Add a body to the simulation"""
        self.bodies.append(body)
        print(f"➕ Added {body.name} (mass: {body.mass:.2e} kg)")
    
    def step(self, dt: float, method: str = "euler") -> None:
        """Advance simulation by dt seconds"""
        if len(self.bodies) == 0:
            return
        
        accelerations = self._calculate_accelerations()
        
        for i, body in enumerate(self.bodies):
            if method == "euler":
                # Euler integration
                body.velocity += accelerations[i] * dt
                body.position += body.velocity * dt
            elif method == "verlet":
                # Simple Verlet (needs previous accelerations)
                if not hasattr(self, '_prev_accelerations'):
                    self._prev_accelerations = accelerations
                body.position += body.velocity * dt + 0.5 * self._prev_accelerations[i] * dt**2
                new_accelerations = self._calculate_accelerations()
                body.velocity += 0.5 * (self._prev_accelerations[i] + new_accelerations[i]) * dt
                self._prev_accelerations = new_accelerations
            
            body.timestamp += datetime.timedelta(seconds=dt)
        
        self.time += dt
        self.step_count += 1
    
    def _calculate_accelerations(self) -> np.ndarray:
        """Calculate gravitational accelerations for all bodies"""
        n = len(self.bodies)
        accelerations = np.zeros((n, 3))
        
        for i in range(n):
            for j in range(n):
                if i != j:
                    r_vec = self.bodies[j].position - self.bodies[i].position
                    r = np.linalg.norm(r_vec)
                    
                    if r < 1e-10:
                        continue
                    
                    accelerations[i] += self.G * self.bodies[j].mass * r_vec / r**3
        
        return accelerations
    
    def simulate(self, duration: float, dt: float, method: str = "euler") -> None:
        """Run simulation for specified duration"""
        if len(self.bodies) == 0:
            print("⚠ No bodies in simulation")
            return
        
        steps = int(duration / dt)
        print(f"🚀 Simulating {duration:.0f} seconds in {steps} steps ({method})...")
        
        for step in range(steps):
            self.step(dt, method)
            
            if steps >= 10 and step % (steps // 10) == 0 and step > 0:
                progress = (step + 1) / steps * 100
                print(f"  Progress: {progress:.0f}% (t = {self.time:.0f} s)")
        
        print(f"✅ Simulation complete")
        print(f"   Final time: {self.time:.0f} s, Steps: {self.step_count}")


# Utility functions for research
def create_circular_orbit(altitude: float, name: str = "Satellite") -> OrbitalState:
    """Create a satellite in circular orbit around Earth"""
    r = EARTH_RADIUS + altitude
    v = np.sqrt(GRAVITATIONAL_CONSTANT * EARTH_MASS / r)
    
    position = np.array([r, 0.0, 0.0])
    velocity = np.array([0.0, v, 0.0])
    
    return OrbitalState(
        position=position,
        velocity=velocity,
        mass=1000.0,
        name=name,
        source="circular_orbit",
        metadata={'altitude': altitude, 'orbital_type': 'circular'}
    )