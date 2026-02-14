# src/k19_uncertainty.py
"""
K19: Uncertainty Quantification Engine
Core propagation system - No domain-specific logic allowed
"""

import numpy as np
import datetime
from dataclasses import dataclass, field
from typing import Union, List, Dict, Any, Optional
import warnings
from copy import deepcopy


@dataclass
class UncertainQuantity:
    """Rule 1: No value exists without uncertainty"""
    
    value: Union[float, np.ndarray]
    uncertainty: Union[float, np.ndarray, Dict[str, Any]]
    source: str
    timestamp: datetime.datetime = field(default_factory=datetime.datetime.now)
    trust_state: Dict[str, float] = field(default_factory=dict)
    propagation_trace: List[str] = field(default_factory=list)

    def __post_init__(self):
        """Validate on creation"""
        self._validate_uncertainty()
        self.propagation_trace.append(f"Created from {self.source}")

    def _validate_uncertainty(self):
        """Rule 1 enforcement: Reject missing uncertainty"""
        if self.uncertainty is None:
            raise ValueError("Uncertainty is missing → reject input (Rule 1)")
        
        if isinstance(self.uncertainty, (int, float, np.number)):
            if self.uncertainty == 0:
                warnings.warn("WARNING: Zero uncertainty detected - auto-inflating")
                self.uncertainty = 1e-10  # Minimal inflation to avoid division by zero
        elif isinstance(self.uncertainty, dict):
            if 'variance' in self.uncertainty and self.uncertainty['variance'] == 0:
                warnings.warn("WARNING: Zero variance detected - auto-inflating")
                self.uncertainty['variance'] = 1e-10

    def to_dict(self):
        """Serializable representation"""
        return {
            'value': float(self.value) if np.isscalar(self.value) else self.value.tolist(),
            'uncertainty': float(self.uncertainty) if np.isscalar(self.uncertainty) else self.uncertainty,
            'source': self.source,
            'timestamp': self.timestamp.isoformat(),
            'trust_state': self.trust_state,
            'propagation_trace': self.propagation_trace
        }


class UncertaintyPropagator:
    """Rule 2: Uncertainty propagates before conclusions"""
    
    def __init__(self, monte_carlo_samples: int = 10000):
        self.monte_carlo_samples = monte_carlo_samples
        self.operation_log = []

    def propagate(self,
                  a: UncertainQuantity,
                  b: UncertainQuantity,
                  operation: str = "+",
                  correlation: Optional[float] = None) -> UncertainQuantity:
        """
        Rule 3.2: No direct arithmetic - must use propagate()
        Hierarchy: Analytic → Jacobian → Monte Carlo
        """
        trace_entry = f"{operation} between {a.source} and {b.source}"

        try:
            # Attempt analytic propagation first (preferred)
            result = self._analytic_propagation(a, b, operation, correlation)
            result.propagation_trace.append(f"Analytic: {trace_entry}")
        except (ValueError, NotImplementedError):
            # Fallback to Jacobian-based
            try:
                result = self._jacobian_propagation(a, b, operation)
                result.propagation_trace.append(f"Jacobian: {trace_entry}")
            except Exception:
                # Mandatory Monte Carlo fallback
                result = self._monte_carlo_propagation(a, b, operation)
                result.propagation_trace.append(f"MonteCarlo: {trace_entry}")

        self.operation_log.append({
            'operation': operation,
            'sources': [a.source, b.source],
            'timestamp': datetime.datetime.now(),
            'method': result.propagation_trace[-1].split(":")[0]
        })

        return result

    def _analytic_propagation(self, a, b, operation, correlation=None):
        """Level 1: Analytic uncertainty propagation"""
        if operation == "+":
            value = a.value + b.value
            if correlation is None:
                # Degrade confidence when correlation unknown
                uncertainty = np.sqrt(a.uncertainty**2 + b.uncertainty**2) * 1.2
            else:
                uncertainty = np.sqrt(a.uncertainty**2 + b.uncertainty**2 +
                                    2*correlation*a.uncertainty*b.uncertainty)

        elif operation == "-":
            value = a.value - b.value
            if correlation is None:
                uncertainty = np.sqrt(a.uncertainty**2 + b.uncertainty**2) * 1.2
            else:
                uncertainty = np.sqrt(a.uncertainty**2 + b.uncertainty**2 -
                                    2*correlation*a.uncertainty*b.uncertainty)

        elif operation == "*":
            value = a.value * b.value
            # Relative uncertainty propagation
            rel_a = a.uncertainty / abs(a.value) if a.value != 0 else np.inf
            rel_b = b.uncertainty / abs(b.value) if b.value != 0 else np.inf
            rel_uncertainty = np.sqrt(rel_a**2 + rel_b**2)
            uncertainty = abs(value) * rel_uncertainty

        elif operation == "/":
            if b.value == 0:
                raise ValueError("Division by zero")
            value = a.value / b.value
            rel_a = a.uncertainty / abs(a.value) if a.value != 0 else np.inf
            rel_b = b.uncertainty / abs(b.value) if b.value != 0 else np.inf
            rel_uncertainty = np.sqrt(rel_a**2 + rel_b**2)
            uncertainty = abs(value) * rel_uncertainty
        else:
            raise NotImplementedError(f"Analytic propagation for {operation} not implemented")

        return UncertainQuantity(
            value=value,
            uncertainty=uncertainty,
            source="analytic_propagation",
            trust_state=self._merge_trust(a.trust_state, b.trust_state)
        )

    def _jacobian_propagation(self, a, b, operation):
        """Level 2: Jacobian-based propagation for non-linear operations"""
        if operation == "**":
            # f(a,b) = a^b
            value = a.value ** b.value

            # Jacobian elements
            df_da = b.value * (a.value ** (b.value - 1))
            df_db = (a.value ** b.value) * np.log(a.value) if a.value > 0 else 0

            uncertainty = np.sqrt(
                (df_da * a.uncertainty)**2 +
                (df_db * b.uncertainty)**2
            )

            return UncertainQuantity(
                value=value,
                uncertainty=uncertainty,
                source="jacobian_propagation",
                trust_state=self._merge_trust(a.trust_state, b.trust_state)
            )
        raise NotImplementedError(f"Jacobian for {operation} not available")

    def _monte_carlo_propagation(self, a, b, operation):
        """Level 3: Mandatory Monte Carlo fallback"""
        # Generate samples
        samples_a = np.random.normal(a.value, a.uncertainty, self.monte_carlo_samples)
        samples_b = np.random.normal(b.value, b.uncertainty, self.monte_carlo_samples)

        # Apply operation
        if operation == "+":
            samples_result = samples_a + samples_b
        elif operation == "-":
            samples_result = samples_a - samples_b
        elif operation == "*":
            samples_result = samples_a * samples_b
        elif operation == "/":
            samples_result = samples_a / (samples_b + 1e-10)  # Avoid division by zero
        elif operation == "**":
            samples_result = samples_a ** samples_b
        else:
            raise ValueError(f"Cannot perform {operation} in Monte Carlo")

        # Compute statistics
        value = np.mean(samples_result)
        uncertainty = np.std(samples_result)

        # Apply constraint-aware clipping
        value, uncertainty = self._constraint_clipping(value, uncertainty, samples_result)

        return UncertainQuantity(
            value=value,
            uncertainty=uncertainty,
            source="monte_carlo_propagation",
            trust_state=self._merge_trust(a.trust_state, b.trust_state)
        )

    def _constraint_clipping(self, value, uncertainty, samples):
        """Constraint-aware clipping"""
        sample_range = np.percentile(samples, 99.9) - np.percentile(samples, 0.1)
        uncertainty = min(uncertainty, sample_range / 2)
        return value, uncertainty

    def _merge_trust(self, trust_a, trust_b):
        """Merge trust states from two quantities"""
        merged = trust_a.copy()
        for key, value in trust_b.items():
            if key in merged:
                merged[key] = (merged[key] + value) / 2  # Average trust
            else:
                merged[key] = value
        return merged


# Quick validation function
def validate_uncertainty_engine():
    """Test the uncertainty engine"""
    print("Validating K19 Uncertainty Engine...")
    
    # Test 1: Basic creation
    q1 = UncertainQuantity(
        value=10.0,
        uncertainty=0.5,
        source="test_measurement"
    )
    
    # Test 2: Propagation
    propagator = UncertaintyPropagator(monte_carlo_samples=1000)
    q2 = UncertainQuantity(
        value=5.0,
        uncertainty=0.2,
        source="second_measurement"
    )
    
    result = propagator.propagate(q1, q2, operation="+")
    print(f"✓ Validation complete: {result.value:.2f} ± {result.uncertainty:.2f}")
    
    return propagator