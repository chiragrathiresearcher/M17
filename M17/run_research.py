#!/usr/bin/env python3
"""
K-Framework: Main Research Pipeline
Run: python run_research.py
"""

import argparse
import sys
from pathlib import Path

# Add src to path
sys.path.insert(0, str(Path(__file__).parent / "src"))

from workflow import KFrameworkWorkflow


def main():
    """Main entry point for the K-Framework"""
    parser = argparse.ArgumentParser(
        description="K-Framework: Research-Grade Satellite Audit System",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python run_research.py                           # Run full audit (10 satellites)
  python run_research.py --satellites 5            # Audit 5 satellites
  python run_research.py --norad 25994             # Analyze specific satellite
  python run_research.py --demo                    # Run demonstration mode
  python run_research.py --summary                 # Generate research summary
        """
    )
    
    parser.add_argument("--satellites", type=int, default=10,
                       help="Maximum number of satellites to audit (default: 10)")
    parser.add_argument("--norad", type=str,
                       help="Analyze specific satellite by NORAD ID")
    parser.add_argument("--demo", action="store_true",
                       help="Run demonstration mode with sample data")
    parser.add_argument("--summary", action="store_true",
                       help="Generate research summary only")
    parser.add_argument("--no-uncertainty", action="store_true",
                       help="Disable uncertainty quantification")
    parser.add_argument("--no-memory", action="store_true",
                       help="Disable scientific memory")
    
    args = parser.parse_args()
    
    print("\n" + "=" * 70)
    print("🌌 K-FRAMEWORK: SATELLITE AFTERLIFE RESEARCH")
    print("=" * 70)
    print("in decommissioned NASA satellite analysis")
    print("=" * 70 + "\n")
    
    # Initialize framework
    workflow = KFrameworkWorkflow(
        enable_uncertainty=not args.no_uncertainty,
        enable_memory=not args.no_memory
    )
    
    if args.norad:
        # Analyze specific satellite
        print(f"🎯 Analyzing satellite NORAD {args.norad}")
        result = workflow.analyze_single_satellite(args.norad)
        print(f"\n✅ Analysis complete for NORAD {args.norad}")
        
    elif args.demo:
        # Demonstration mode
        print("🎪 Running demonstration mode...")
        from src.k19_uncertainty import validate_uncertainty_engine
        from src.k20_physics import create_circular_orbit
        
        # Demo K19
        print("\n1. Demonstrating K19 Uncertainty Engine...")
        propagator = validate_uncertainty_engine()
        
        # Demo K20
        print("\n2. Demonstrating K20 Physics Core...")
        satellite = create_circular_orbit(400000, "DEMO_SAT")
        print(f"   Created {satellite.name}: {satellite.velocity_magnitude/1000:.1f} km/s")
        
        # Demo audit
        print("\n3. Running sample audit...")
        result = workflow.run_dead_satellite_audit(max_satellites=3)
        print(f"\n📊 Sample audit complete: {result.get('potential_zombies', 0)} potential zombies found")
        
    elif args.summary:
        # Generate summary only
        print("📊 Generating research summary...")
        summary = workflow.generate_research_summary()
        print(f"\n✅ Summary generated:")
        print(f"   Framework version: {summary.get('framework_version')}")
        print(f"   Outputs location: {summary.get('outputs_location')}")
        if 'case_registry_stats' in summary:
            print(f"   Cases in registry: {summary['case_registry_stats'].get('total_cases', 0)}")
        
    else:
        # Run full audit
        print(f"🔬 Starting dead satellite audit (max: {args.satellites} satellites)")
        print("-" * 70)
        
        result = workflow.run_dead_satellite_audit(max_satellites=args.satellites)
        
        print(f"\n📈 AUDIT RESULTS:")
        print(f"   Satellites audited: {result.get('satellites_audited', 0)}")
        print(f"   With current TLE data: {result.get('satellites_with_current_tle', 0)}")
        print(f"   Potential 'zombie' satellites: {result.get('potential_zombies', 0)}")
        print(f"   Zombie detection rate: {result.get('zombie_rate', 0)*100:.1f}%")
        
        if result.get('saved_to'):
            print(f"\n📁 Full results saved to: {result['saved_to']}")
        
        # Generate research summary
        print("\n📊 Generating research summary...")
        summary = workflow.generate_research_summary()
        print(f"✅ Research summary generated")
    
    print("\n" + "=" * 70)
    print("RESEARCH COMPLETE")
    print("=" * 70)
    print("\nNext steps for publication:")
    print("1. Review outputs in data/outputs/")
    print("2. Create visualizations from JSON results")
    print("3. Write paper with methodology and findings")
    print("4. Register DOI on Zenodo")
    print("5. Submit to arXiv or research journal")
    print("\nRemember: Science is about asking questions,")
    print("not just finding answers. Keep exploring! 🚀")


if __name__ == "__main__":
    main()