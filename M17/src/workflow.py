# src/workflow.py
"""
Main Workflow: Orchestrates the complete K-Framework pipeline
"""

import datetime
import json
from typing import List, Dict, Optional
import pandas as pd
from pathlib import Path

from .k19_uncertainty import UncertainQuantity, UncertaintyPropagator, validate_uncertainty_engine
from .k20_physics import OrbitalState, TLEProcessor, create_circular_orbit
from .k21_memory import Case, CaseRegistry, PhysicalStateRecord, EpistemicJudgment, MetaContext
from .satellite_bridge import SatelliteDataFetcher, DeadSatelliteAuditor
from .observation_ingestion import SatelliteDataProcessor, ObservationTrustModel
from config.settings import OUTPUTS_DIR


class KFrameworkWorkflow:
    """Main orchestration class for the K-Framework"""
    
    def __init__(self, 
                 enable_uncertainty: bool = True,
                 enable_memory: bool = True,
                 cache_data: bool = True):
        
        print("\n" + "=" * 70)
        print("K-FRAMEWORK: RESEARCH-GRADE SATELLITE AUDIT SYSTEM")
        print("=" * 70)
        
        # Initialize components
        self.enable_uncertainty = enable_uncertainty
        self.enable_memory = enable_memory
        self.cache_data = cache_data
        
        # Core modules
        if enable_uncertainty:
            print("🔧 Initializing K19 Uncertainty Engine...")
            self.uncertainty_propagator = validate_uncertainty_engine()
        else:
            self.uncertainty_propagator = None
        
        if enable_memory:
            print("🧠 Initializing K21 Scientific Memory...")
            self.case_registry = CaseRegistry()
        else:
            self.case_registry = None
        
        # Data modules
        print("📡 Initializing Satellite Data Bridge...")
        self.data_fetcher = SatelliteDataFetcher()
        self.auditor = DeadSatelliteAuditor(self.data_fetcher)
        
        # Processing modules
        print("⚡ Initializing Observation Processor...")
        self.processor = SatelliteDataProcessor()
        
        print("✅ K-Framework initialized successfully")
    
    def run_dead_satellite_audit(self, 
                                 max_satellites: int = 10,
                                 save_results: bool = True) -> Dict:
        """
        Main audit workflow for decommissioned satellites
        
        Args:
            max_satellites: Maximum number of satellites to audit
            save_results: Whether to save results to file
            
        Returns:
            Audit results dictionary
        """
        print("\n" + "=" * 70)
        print("STARTING DEAD SATELLITE AUDIT")
        print("=" * 70)
        
        # Step 1: Audit satellites
        print("\n1. Auditing decommissioned NASA satellites...")
        audit_results = self.auditor.audit_all(max_satellites=max_satellites)
        
        # Step 2: Generate comprehensive report
        print("\n2. Generating audit report...")
        report = self.auditor.generate_report(audit_results)
        
        # Step 3: Process through uncertainty framework
        if self.enable_uncertainty:
            print("\n3. Applying uncertainty quantification...")
            processed_results = self._apply_uncertainty_to_audit(audit_results)
            report['uncertainty_analysis'] = processed_results
        
        # Step 4: Create scientific cases
        if self.enable_memory and self.case_registry:
            print("\n4. Creating scientific cases...")
            cases_created = self._create_cases_from_audit(audit_results)
            report['cases_created'] = cases_created
            
            # Save registry
            self.case_registry.save_registry()
        
        # Step 5: Save final report
        if save_results:
            timestamp = datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
            report_file = OUTPUTS_DIR / f"dead_satellite_audit_{timestamp}.json"
            
            with open(report_file, 'w') as f:
                json.dump(report, f, indent=2, default=str)
            
            print(f"\n💾 Full report saved to: {report_file}")
            report['saved_to'] = str(report_file)
        
        print("\n" + "=" * 70)
        print("AUDIT COMPLETE")
        print("=" * 70)
        
        return report
    
    def _apply_uncertainty_to_audit(self, audit_results: List[Dict]) -> List[Dict]:
        """Apply uncertainty quantification to audit results"""
        processed = []
        
        for result in audit_results:
            if 'zombie_score' in result:
                # Create uncertain quantity for zombie score
                uncertainty = result['zombie_score'] * 0.2  # 20% relative uncertainty
                
                uncertain_score = UncertainQuantity(
                    value=result['zombie_score'],
                    uncertainty=uncertainty,
                    source=f"audit_{result['satellite']}",
                    trust_state={'audit_confidence': 0.7}
                )
                
                processed_result = result.copy()
                processed_result['zombie_score_uncertain'] = uncertain_score.to_dict()
                processed.append(processed_result)
        
        return processed
    
    def _create_cases_from_audit(self, audit_results: List[Dict]) -> List[str]:
        """Create K21 scientific cases from audit results"""
        case_ids = []
        
        for result in audit_results:
            if result.get('zombie_score', 0) > 0.3:  # Only create cases for interesting results
                case = self.case_registry.create_case({
                    'audit_result': result,
                    'satellite': result['satellite'],
                    'norad_id': result['norad_id']
                })
                
                # Add observation to case
                if 'anomalies' in result:
                    # Create mock observation for demonstration
                    physical_state = PhysicalStateRecord(
                        position=[0, 0, 0],  # Placeholder
                        velocity=[0, 0, 0],
                        time=datetime.datetime.now(),
                        source=f"audit_{result['satellite']}",
                        constraints_applied=['zombie_detection'],
                        constraint_results={'zombie_detection': result['zombie_score'] > 0.5}
                    )
                    
                    epistemic_judgment = EpistemicJudgment(
                        uncertainty_distribution={'zombie_score': result.get('zombie_score', 0) * 0.2},
                        confidence_trajectory=[0.7, 0.7, 0.7],
                        inflation_events=[],
                        pass_fail_flags={'audit_complete': True},
                        explanation=f"Satellite audit: {result['satellite']}"
                    )
                    
                    meta_context = MetaContext(
                        timestamp=datetime.datetime.now(),
                        data_source='K-Framework_Audit',
                        model_version='1.0',
                        observer_tag='RESEARCHER'
                    )
                    
                    case.add_observation(
                        physical_state=physical_state,
                        epistemic_judgment=epistemic_judgment,
                        meta_context=meta_context,
                        explanation=f"Dead satellite audit for {result['satellite']}"
                    )
                
                case_ids.append(case.id)
                
                # Save individual case
                case.save()
        
        return case_ids
    
    def analyze_single_satellite(self, norad_id: str) -> Dict:
        """Detailed analysis of a single satellite"""
        print(f"\n🔬 Detailed analysis of NORAD {norad_id}")
        
        # Fetch TLE data
        tle_data = self.data_fetcher.fetch_tle_from_celestrak(catalog=norad_id)
        
        if not tle_data:
            return {'error': 'No TLE data found'}
        
        analysis = {
            'norad_id': norad_id,
            'analysis_date': datetime.datetime.now().isoformat(),
            'tle_sources': list(tle_data.keys())
        }
        
        # Process each TLE
        for name, tle_lines in tle_data.items():
            try:
                from .k20_physics import TLEProcessor
                orbital_state = TLEProcessor.tle_to_orbital_state(tle_lines, name)
                
                # Store orbital analysis
                analysis[name] = {
                    'orbital_state': orbital_state.to_dict(),
                    'velocity_magnitude': orbital_state.velocity_magnitude,
                    'position_magnitude': orbital_state.position_magnitude,
                    'orbital_elements': orbital_state.orbital_elements
                }
                
                # Apply uncertainty if enabled
                if self.uncertainty_propagator:
                    # Create uncertain velocity
                    uncertain_velocity = UncertainQuantity(
                        value=orbital_state.velocity_magnitude,
                        uncertainty=orbital_state.uncertainty.get('velocity', 1.0),
                        source=f"TLE_{norad_id}",
                        trust_state={'tle_quality': 0.8}
                    )
                    
                    analysis[name]['uncertain_velocity'] = uncertain_velocity.to_dict()
                    
            except Exception as e:
                analysis[name] = {'error': str(e)}
        
        # Save analysis
        output_file = OUTPUTS_DIR / f"satellite_analysis_{norad_id}_{datetime.datetime.now().strftime('%Y%m%d')}.json"
        with open(output_file, 'w') as f:
            json.dump(analysis, f, indent=2)
        
        print(f"💾 Analysis saved to {output_file}")
        return analysis
    
    def generate_research_summary(self) -> Dict:
        """Generate comprehensive research summary"""
        summary = {
            'framework_version': '1.0',
            'generated_at': datetime.datetime.now().isoformat(),
            'components': {
                'k19_uncertainty': self.enable_uncertainty,
                'k21_memory': self.enable_memory,
                'data_fetcher': True,
                'auditor': True,
                'processor': True
            },
            'outputs_location': str(OUTPUTS_DIR),
            'research_aim': 'Audit decommissioned NASA satellites for potential residual functionality',
            'methodology': [
                '1. Fetch current TLE data from Celestrak',
                '2. Analyze orbital characteristics',
                '3. Apply uncertainty quantification',
                '4. Create scientific cases for anomalies',
                '5. Generate research reports'
            ]
        }
        
        if self.case_registry:
            summary['case_registry_stats'] = self.case_registry.get_registry_stats()
        
        # Save summary
        summary_file = OUTPUTS_DIR / f"research_summary_{datetime.datetime.now().strftime('%Y%m%d')}.json"
        with open(summary_file, 'w') as f:
            json.dump(summary, f, indent=2)
        
        return summary