# src/observation_ingestion.py
"""
Observation Ingestion: Trust-aware satellite data processing
"""

import numpy as np
import datetime
from typing import Dict, List, Optional
from dataclasses import dataclass
from .k19_uncertainty import UncertainQuantity


@dataclass
class ObservationTrustModel:
    """Trust affects uncertainty inflation, not acceptance"""
    
    def __init__(self):
        self.instrument_profiles = {
            'HST': {'precision': 0.95, 'reliability': 0.98, 'age': 0.8},
            'ISS': {'precision': 0.85, 'reliability': 0.95, 'age': 0.7},
            'GPS': {'precision': 0.99, 'reliability': 0.99, 'age': 0.9},
            'GROUND_RADAR': {'precision': 0.75, 'reliability': 0.85, 'age': 0.6},
            'DEFAULT': {'precision': 0.8, 'reliability': 0.8, 'age': 0.5}
        }
    
    def assess_trust(self, 
                    instrument_id: str,
                    calibration_age_days: float = 30,
                    environment_quality: float = 0.9) -> Dict:
        """Assess trust level for an observation"""
        profile = self.instrument_profiles.get(
            instrument_id.upper(),
            self.instrument_profiles['DEFAULT']
        )
        
        # Calculate trust score (0-1, higher is better)
        base_trust = (profile['precision'] * 0.4 +
                     profile['reliability'] * 0.4 +
                     profile['age'] * 0.2)
        
        # Adjust for calibration
        calibration_factor = max(0.5, 1.0 - (calibration_age_days / 365))
        
        # Final trust score
        trust_score = base_trust * calibration_factor * environment_quality
        
        # Inflation factor: lower trust = higher uncertainty inflation
        inflation_factor = 1.0 + (1.0 - trust_score) * 2.0
        
        return {
            'trust_score': trust_score,
            'inflation_factor': inflation_factor,
            'instrument_profile': profile,
            'calibration_factor': calibration_factor
        }
    
    def apply_trust_inflation(self, 
                             quantity: UncertainQuantity,
                             trust_assessment: Dict) -> UncertainQuantity:
        """Inflate uncertainty based on trust assessment"""
        inflated_uncertainty = quantity.uncertainty * trust_assessment['inflation_factor']
        
        # Create new quantity with inflated uncertainty
        inflated_quantity = UncertainQuantity(
            value=quantity.value,
            uncertainty=inflated_uncertainty,
            source=f"trust_inflated_{quantity.source}",
            trust_state={
                **quantity.trust_state,
                'original_uncertainty': quantity.uncertainty,
                'trust_score': trust_assessment['trust_score'],
                'inflation_factor': trust_assessment['inflation_factor']
            }
        )
        
        # Copy propagation trace
        if hasattr(quantity, 'propagation_trace'):
            inflated_quantity.propagation_trace = quantity.propagation_trace.copy()
        
        return inflated_quantity


class SatelliteDataProcessor:
    """Process satellite data with K19 compliance"""
    
    def __init__(self, trust_model=None, constraints=None):
        self.trust_model = trust_model or ObservationTrustModel()
        self.constraints = constraints
        self.processed_count = 0
        self.anomaly_count = 0
        self.processing_log = []
    
    def process_observation(self, raw_data: Dict) -> Dict:
        """Process a single observation through K19 pipeline"""
        self.processed_count += 1
        
        try:
            # Step 1: Convert to UncertainQuantity
            observation = self._create_uncertain_quantity(raw_data)
            
            # Step 2: Apply trust model
            trust_assessment = self.trust_model.assess_trust(
                instrument_id=raw_data.get('instrument', 'UNKNOWN'),
                calibration_age_days=raw_data.get('calibration_age_days', 30),
                environment_quality=raw_data.get('environment_quality', 0.9)
            )
            
            trusted_observation = self.trust_model.apply_trust_inflation(
                observation, trust_assessment
            )
            
            # Step 3: Check constraints if available
            violations = []
            if self.constraints:
                # Apply physics constraints
                constrained_obs, violations = self.constraints.apply_constraints(
                    trusted_observation
                )
            else:
                constrained_obs = trusted_observation
            
            # Create result
            result = {
                'status': 'processed',
                'observation_id': raw_data.get('id', f'OBS-{self.processed_count:06d}'),
                'original_value': raw_data['value'],
                'original_uncertainty': raw_data['uncertainty'],
                'final_value': constrained_obs.value,
                'final_uncertainty': constrained_obs.uncertainty,
                'trust_score': trust_assessment['trust_score'],
                'inflation_factor': trust_assessment['inflation_factor'],
                'constraint_violations': len(violations),
                'survives_constraints': len(violations) == 0,
                'processing_timestamp': datetime.datetime.now().isoformat(),
                'instrument': raw_data.get('instrument', 'UNKNOWN')
            }
            
            if violations:
                self.anomaly_count += 1
                result['anomalies'] = violations
            
            # Log processing
            self.processing_log.append(result)
            
            return result
            
        except Exception as e:
            error_result = {
                'status': 'error',
                'observation_id': raw_data.get('id', f'ERR-{self.processed_count:06d}'),
                'error': str(e),
                'raw_data': raw_data,
                'processing_timestamp': datetime.datetime.now().isoformat()
            }
            self.processing_log.append(error_result)
            return error_result
    
    def _create_uncertain_quantity(self, raw_data: Dict) -> UncertainQuantity:
        """Convert raw data to UncertainQuantity"""
        required_fields = ['value', 'uncertainty']
        for field in required_fields:
            if field not in raw_data:
                raise ValueError(f"Missing required field: {field}")
        
        return UncertainQuantity(
            value=float(raw_data['value']),
            uncertainty=float(raw_data['uncertainty']),
            source=raw_data.get('instrument', 'unknown_instrument'),
            trust_state={
                'raw_trust': 0.5,
                'calibration_age': raw_data.get('calibration_age_days', 30),
                'environment': raw_data.get('environment_quality', 0.9)
            }
        )
    
    def process_batch(self, observations: List[Dict]) -> List[Dict]:
        """Process multiple observations"""
        results = []
        for obs in observations:
            result = self.process_observation(obs)
            results.append(result)
        return results
    
    def get_statistics(self) -> Dict:
        """Get processing statistics"""
        if not self.processing_log:
            return {}
        
        successful = [r for r in self.processing_log if r['status'] == 'processed']
        
        if successful:
            avg_trust = np.mean([r.get('trust_score', 0) for r in successful])
            survival_rate = np.mean([r.get('survives_constraints', False) for r in successful])
        else:
            avg_trust = 0
            survival_rate = 0
        
        return {
            'total_processed': len(self.processing_log),
            'successful': len(successful),
            'errors': len([r for r in self.processing_log if r['status'] == 'error']),
            'anomalies': self.anomaly_count,
            'avg_trust_score': avg_trust,
            'survival_rate': survival_rate
        }