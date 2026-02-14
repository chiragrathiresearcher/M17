# src/k21_memory.py
"""
K21: Scientific Memory & Case Evolution Engine
Purpose: Records scientific cases over time, preserves anomalies and failures
IMPORTANT: K21 does NOT compute, judge, or decide. It ONLY stores, links, evolves, and explains.
"""

import datetime
import uuid
import json
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Any, Tuple
from enum import Enum, auto
import pandas as pd
import numpy as np
from pathlib import Path
from config.settings import OUTPUTS_DIR


class CaseState(Enum):
    """Allowed Case States - Descriptive, not verdicts"""
    CONSISTENT = auto()    # All evidence agrees
    TENSION = auto()       # Some contradictory evidence
    ANOMALOUS = auto()     # Violates current constraints
    DORMANT = auto()       # No recent evidence
    RESOLVED = auto()      # Case explained within current framework
    FALSIFIED = auto()     # Core assumption proven false


class InvariantViolationError(Exception):
    """Raised when K21 invariants are violated"""
    pass


@dataclass
class PhysicalStateRecord:
    """Input from K20 - Reality states"""
    position: List[float]
    velocity: List[float]
    time: datetime.datetime
    source: str
    constraints_applied: List[str]
    constraint_results: Dict[str, bool]
    
    def to_dict(self) -> Dict:
        return {
            'type': 'PhysicalStateRecord',
            'position': self.position,
            'velocity': self.velocity,
            'time': self.time.isoformat(),
            'source': self.source,
            'constraints_applied': self.constraints_applied,
            'constraint_results': self.constraint_results
        }


@dataclass
class EpistemicJudgment:
    """Input from K19 - Epistemic reliability judgments"""
    uncertainty_distribution: Dict[str, float]
    confidence_trajectory: List[float]
    inflation_events: List[Dict]
    pass_fail_flags: Dict[str, bool]
    explanation: str
    
    def to_dict(self) -> Dict:
        return {
            'type': 'EpistemicJudgment',
            'uncertainty_distribution': self.uncertainty_distribution,
            'confidence_trajectory': self.confidence_trajectory,
            'inflation_events': self.inflation_events,
            'pass_fail_flags': self.pass_fail_flags,
            'explanation': self.explanation
        }


@dataclass
class MetaContext:
    """Mandatory metadata for all inputs"""
    timestamp: datetime.datetime
    data_source: str
    model_version: str
    observer_tag: str
    
    def to_dict(self) -> Dict:
        return {
            'type': 'MetaContext',
            'timestamp': self.timestamp.isoformat(),
            'data_source': self.data_source,
            'model_version': self.model_version,
            'observer_tag': self.observer_tag
        }


class Case:
    """Scientific Case with immutable ID - Never deletes history"""
    
    def __init__(self, case_id: str = None, origin_event: Dict = None):
        self.id = case_id if case_id else f"CASE-{uuid.uuid4().hex[:8].upper()}"
        self.origin_event = origin_event or {}
        self.linked_observations: List[Dict] = []
        self.linked_constraints: List[Dict] = []
        self.state: CaseState = CaseState.CONSISTENT
        self.state_transition_log: List[Dict] = []
        self.confidence_over_time: List[float] = []
        self.explanation_log: List[str] = []
        self.created_at: datetime.datetime = datetime.datetime.now()
        self.last_updated: datetime.datetime = self.created_at
        self._last_timestamp: Optional[datetime.datetime] = None
        
        # Log creation
        self._add_state_transition(
            from_state=None,
            to_state=CaseState.CONSISTENT,
            evidence={"type": "creation"},
            explanation=f"Case {self.id} created",
            timestamp=self.created_at
        )
        
        print(f"📂 Case {self.id} created at {self.created_at.isoformat()}")

    def _add_state_transition(self,
                             from_state: Optional[CaseState],
                             to_state: CaseState,
                             evidence: Dict,
                             explanation: str,
                             timestamp: datetime.datetime) -> None:
        """Record a state transition with full metadata"""
        
        # ENFORCE: No backward time travel
        if self._last_timestamp and timestamp < self._last_timestamp:
            raise InvariantViolationError(
                f"Time monotonicity violation: "
                f"New timestamp {timestamp} < Last timestamp {self._last_timestamp}"
            )
        
        transition = {
            'timestamp': timestamp.isoformat(),
            'from_state': from_state.name if from_state else "NONE",
            'to_state': to_state.name,
            'evidence': evidence,
            'explanation': explanation,
            'transition_id': f"TR-{len(self.state_transition_log):06d}"
        }
        
        self.state_transition_log.append(transition)
        self._last_timestamp = timestamp
        self.last_updated = datetime.datetime.now()
        
        # Add to explanation log
        self.explanation_log.append(
            f"[{timestamp.isoformat()}] {explanation} "
            f"({from_state.name if from_state else 'NONE'} → {to_state.name})"
        )

    def add_observation(self,
                       physical_state: PhysicalStateRecord,
                       epistemic_judgment: EpistemicJudgment,
                       meta_context: MetaContext,
                       explanation: str = "") -> None:
        """Add an observation to the case"""
        
        # ENFORCE: MetaContext required
        if not isinstance(meta_context, MetaContext):
            raise InvariantViolationError("MetaContext is mandatory for all observations")
        
        # ENFORCE: Time monotonicity
        if self._last_timestamp and meta_context.timestamp < self._last_timestamp:
            raise InvariantViolationError(
                f"Time travel violation: "
                f"New {meta_context.timestamp} < Previous {self._last_timestamp}"
            )
        
        # Store observation (append-only)
        observation_record = {
            'meta': meta_context.to_dict(),
            'physical': physical_state.to_dict(),
            'epistemic': epistemic_judgment.to_dict(),
            'explanation': explanation,
            'added_at': datetime.datetime.now().isoformat()
        }
        
        self.linked_observations.append(observation_record)
        
        # Update confidence trajectory (from K19 judgment)
        if epistemic_judgment.confidence_trajectory:
            latest_confidence = epistemic_judgment.confidence_trajectory[-1]
            self.confidence_over_time.append(latest_confidence)
        
        # Determine new state based on evidence
        new_state = self._determine_state(
            physical_state,
            epistemic_judgment,
            meta_context
        )
        
        # Record state transition
        self._add_state_transition(
            from_state=self.state,
            to_state=new_state,
            evidence={
                'physical_source': physical_state.source,
                'epistemic_explanation': epistemic_judgment.explanation,
                'constraint_violations': [
                    k for k, v in physical_state.constraint_results.items() if not v
                ]
            },
            explanation=explanation or "Observation added",
            timestamp=meta_context.timestamp
        )
        
        # Update current state
        self.state = new_state
        
        print(f"  📝 Added observation to {self.id} at {meta_context.timestamp.isoformat()}")
        print(f"    State: {self.state.name}")

    def _determine_state(self,
                        physical_state: PhysicalStateRecord,
                        epistemic_judgment: EpistemicJudgment,
                        meta_context: MetaContext) -> CaseState:
        """Determine case state based on evidence"""
        
        # Check constraint violations
        constraint_violations = [
            k for k, v in physical_state.constraint_results.items()
            if not v
        ]
        
        # Check epistemic flags
        epistemic_failures = [
            k for k, v in epistemic_judgment.pass_fail_flags.items()
            if not v
        ]
        
        # State determination logic (descriptive only)
        if constraint_violations:
            return CaseState.ANOMALOUS
        elif epistemic_failures:
            return CaseState.TENSION
        elif not constraint_violations and not epistemic_failures:
            return CaseState.CONSISTENT
        else:
            return self.state  # Default to current state

    def get_summary(self) -> Dict:
        """Get case summary (READ-ONLY output)"""
        return {
            'case_id': self.id,
            'state': self.state.name,
            'created_at': self.created_at.isoformat(),
            'last_updated': self.last_updated.isoformat(),
            'observation_count': len(self.linked_observations),
            'constraint_count': len(self.linked_constraints),
            'transition_count': len(self.state_transition_log),
            'current_confidence': (
                self.confidence_over_time[-1]
                if self.confidence_over_time
                else None
            ),
            'has_anomalies': any(
                t['to_state'] == 'ANOMALOUS'
                for t in self.state_transition_log
            )
        }

    def to_dict(self) -> Dict:
        """Full case serialization (preserves everything)"""
        return {
            'case_id': self.id,
            'origin_event': self.origin_event,
            'state': self.state.name,
            'created_at': self.created_at.isoformat(),
            'last_updated': self.last_updated.isoformat(),
            'linked_observations': self.linked_observations,
            'linked_constraints': self.linked_constraints,
            'state_transition_log': self.state_transition_log,
            'confidence_over_time': self.confidence_over_time,
            'explanation_log': self.explanation_log,
            'summary': self.get_summary()
        }

    def save(self, filename: str = None) -> str:
        """Save case to file (preserves all history)"""
        if not filename:
            filename = OUTPUTS_DIR / f"k21_case_{self.id}_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        else:
            filename = OUTPUTS_DIR / filename
        
        with open(filename, 'w') as f:
            json.dump(self.to_dict(), f, indent=2)
        
        print(f"💾 Case {self.id} saved to {filename}")
        return str(filename)

    @classmethod
    def load(cls, filename: str) -> 'Case':
        """Load case from file"""
        with open(filename, 'r') as f:
            data = json.load(f)
        
        # Recreate case
        case = cls(case_id=data['case_id'], origin_event=data['origin_event'])
        
        # Restore all fields
        case.linked_observations = data['linked_observations']
        case.linked_constraints = data['linked_constraints']
        case.state = CaseState[data['state']]
        case.state_transition_log = data['state_transition_log']
        case.confidence_over_time = data['confidence_over_time']
        case.explanation_log = data['explanation_log']
        case.created_at = datetime.datetime.fromisoformat(data['created_at'])
        case.last_updated = datetime.datetime.fromisoformat(data['last_updated'])
        
        # Restore timestamp tracking
        if case.state_transition_log:
            last_transition = case.state_transition_log[-1]
            case._last_timestamp = datetime.datetime.fromisoformat(
                last_transition['timestamp']
            )
        
        print(f"📂 Case {case.id} loaded from {filename}")
        return case


class CaseRegistry:
    """Manages multiple scientific cases"""
    
    def __init__(self):
        self.cases: Dict[str, Case] = {}
        self.registry_log: List[Dict] = []
    
    def create_case(self, origin_event: Dict = None) -> Case:
        """Create a new case"""
        case = Case(origin_event=origin_event)
        self.cases[case.id] = case
        
        # Log creation
        self.registry_log.append({
            'timestamp': datetime.datetime.now().isoformat(),
            'event': 'case_created',
            'case_id': case.id,
            'origin_event': origin_event
        })
        
        return case
    
    def get_case(self, case_id: str) -> Optional[Case]:
        """Get case by ID"""
        return self.cases.get(case_id)
    
    def list_cases(self) -> List[Dict]:
        """List all cases with summaries"""
        return [
            {
                'case_id': case_id,
                **case.get_summary()
            }
            for case_id, case in self.cases.items()
        ]
    
    def save_registry(self, filename: str = "k21_registry.json") -> None:
        """Save entire registry"""
        registry_data = {
            'cases': {case_id: case.to_dict() for case_id, case in self.cases.items()},
            'registry_log': self.registry_log,
            'saved_at': datetime.datetime.now().isoformat(),
            'stats': self.get_registry_stats()
        }
        
        filename = OUTPUTS_DIR / filename
        with open(filename, 'w') as f:
            json.dump(registry_data, f, indent=2)
        
        print(f"💾 Registry saved to {filename} ({len(self.cases)} cases)")
    
    def get_registry_stats(self) -> Dict:
        """Get registry statistics"""
        states = {}
        for case in self.cases.values():
            state_name = case.state.name
            states[state_name] = states.get(state_name, 0) + 1
        
        return {
            'total_cases': len(self.cases),
            'states': states,
            'total_observations': sum(len(c.linked_observations) for c in self.cases.values()),
            'total_constraints': sum(len(c.linked_constraints) for c in self.cases.values()),
            'registry_log_entries': len(self.registry_log)
        }