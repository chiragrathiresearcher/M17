# src/satellite_bridge.py
"""
Satellite Data Bridge: Fetch and process real satellite data
"""

import requests
import datetime
import json
from typing import List, Dict, Optional
from pathlib import Path
import time
from config.settings import CELESTRAK_URL, RAW_DATA_DIR, CACHE_DURATION


class SatelliteDataFetcher:
    """Fetch satellite data from various sources"""
    
    def __init__(self, cache_dir: Path = RAW_DATA_DIR):
        self.cache_dir = cache_dir
        self.cache_dir.mkdir(parents=True, exist_ok=True)
    
    def fetch_tle_from_celestrak(self, 
                                 group: str = "active",
                                 catalog: str = None) -> Dict[str, List[str]]:
        """
        Fetch TLE data from Celestrak
        
        Args:
            group: 'active', 'stations', 'last-30-days', 'visual', etc.
            catalog: Specific catalog number
            
        Returns:
            Dictionary mapping satellite names to TLE lines
        """
        cache_file = self.cache_dir / f"celestrak_{group}_{datetime.datetime.now().strftime('%Y%m%d')}.json"
        
        # Check cache
        if cache_file.exists():
            cache_age = time.time() - cache_file.stat().st_mtime
            if cache_age < CACHE_DURATION:
                print(f"📦 Using cached data from {cache_file}")
                with open(cache_file, 'r') as f:
                    return json.load(f)
        
        print(f"🌍 Fetching TLE data from Celestrak (group: {group})...")
        
        if catalog:
            url = f"{CELESTRAK_URL}?CATNR={catalog}"
        else:
            url = f"{CELESTRAK_URL}?GROUP={group}"
        
        try:
            response = requests.get(url, timeout=30)
            response.raise_for_status()
            
            tle_data = self._parse_tle_response(response.text)
            
            # Cache the results
            with open(cache_file, 'w') as f:
                json.dump(tle_data, f, indent=2)
            
            print(f"✅ Fetched {len(tle_data)} satellites")
            return tle_data
            
        except requests.RequestException as e:
            print(f"❌ Failed to fetch TLE data: {e}")
            
            # Try to use backup cache
            backup_files = list(self.cache_dir.glob(f"celestrak_{group}_*.json"))
            if backup_files:
                latest = max(backup_files, key=lambda x: x.stat().st_mtime)
                print(f"🔄 Using backup cache: {latest}")
                with open(latest, 'r') as f:
                    return json.load(f)
            
            return {}
    
    def _parse_tle_response(self, text: str) -> Dict[str, List[str]]:
        """Parse TLE response text into structured data"""
        lines = text.strip().split('\n')
        tle_dict = {}
        
        i = 0
        while i < len(lines) - 2:
            name = lines[i].strip()
            line1 = lines[i + 1].strip()
            line2 = lines[i + 2].strip()
            
            if line1.startswith('1 ') and line2.startswith('2 '):
                tle_dict[name] = [name, line1, line2]
                i += 3
            else:
                i += 1
        
        return tle_dict
    
    def get_nasa_decommissioned_satellites(self) -> List[Dict]:
        """Get list of NASA decommissioned satellites (pre-defined)"""
        # This is a curated list - you can expand this
        nasa_satellites = [
            {
                'name': 'TERRA',
                'norad_id': '25994',
                'launch_date': '1999-12-18',
                'mission_end': '2020-02-24',
                'mass_kg': 4864,
                'notes': 'EOS flagship, still partially operational?'
            },
            {
                'name': 'AQUA',
                'norad_id': '27424',
                'launch_date': '2002-05-04',
                'mission_end': '2022-06-30',
                'mass_kg': 2934,
                'notes': 'EOS afternoon constellation'
            },
            {
                'name': 'LANDSAT 7',
                'norad_id': '25682',
                'launch_date': '1999-04-15',
                'mission_end': '2022-04-06',
                'mass_kg': 2200,
                'notes': 'Extended mission ended, scan line corrector failed'
            },
            {
                'name': 'ICESAT',
                'norad_id': '27642',
                'launch_date': '2003-01-12',
                'mission_end': '2010-08-14',
                'mass_kg': 970,
                'notes': 'Laser altimetry, officially decommissioned'
            },
            {
                'name': 'GRACE',
                'norad_id': '27391',
                'launch_date': '2002-03-17',
                'mission_end': '2017-10-27',
                'mass_kg': 487,
                'notes': 'Gravity Recovery and Climate Experiment'
            },
            {
                'name': 'UARS',
                'norad_id': '21701',
                'launch_date': '1991-09-12',
                'mission_end': '2005-12-14',
                'mass_kg': 6540,
                'notes': 'Upper Atmosphere Research Satellite, re-entered 2011'
            },
            {
                'name': 'EO-1',
                'norad_id': '26998',
                'launch_date': '2000-11-21',
                'mission_end': '2017-03-30',
                'mass_kg': 529,
                'notes': 'Earth Observing-1, technology demonstration'
            },
            {
                'name': 'QUICKSCAT',
                'norad_id': '25678',
                'launch_date': '1999-06-19',
                'mission_end': '2009-11-23',
                'mass_kg': 970,
                'notes': 'SeaWinds scatterometer, antenna failure'
            },
            {
                'name': 'SORCE',
                'norad_id': '27649',
                'launch_date': '2003-01-25',
                'mission_end': '2020-02-25',
                'mass_kg': 315,
                'notes': 'Solar Radiation and Climate Experiment'
            },
            {
                'name': 'CALIPSO',
                'norad_id': '29108',
                'launch_date': '2006-04-28',
                'mission_end': '2023-08-01',
                'mass_kg': 635,
                'notes': 'Cloud-Aerosol Lidar and Infrared Pathfinder'
            }
        ]
        
        print(f"📚 Found {len(nasa_satellites)} NASA decommissioned satellites")
        return nasa_satellites


class DeadSatelliteAuditor:
    """Audit dead/retired satellites for potential functionality"""
    
    def __init__(self, fetcher: SatelliteDataFetcher):
        self.fetcher = fetcher
        self.results = []
    
    def audit_satellite(self, satellite_info: Dict) -> Dict:
        """Audit a single satellite"""
        print(f"\n🔍 Auditing {satellite_info['name']} (NORAD: {satellite_info['norad_id']})...")
        
        # Fetch current TLE data
        tle_data = self.fetcher.fetch_tle_from_celestrak(catalog=satellite_info['norad_id'])
        
        result = {
            'satellite': satellite_info['name'],
            'norad_id': satellite_info['norad_id'],
            'audit_date': datetime.datetime.now().isoformat(),
            'has_current_tle': False,
            'anomalies': [],
            'zombie_score': 0.0,  # 0-1 likelihood of being functional
            'recommendations': []
        }
        
        if tle_data:
            # Satellite has current orbital data (it's being tracked)
            result['has_current_tle'] = True
            result['zombie_score'] += 0.3
            
            # Check orbital stability
            for name, tle_lines in tle_data.items():
                if satellite_info['norad_id'] in tle_lines[1]:
                    result['tle_source'] = name
                    result['latest_tle_date'] = datetime.datetime.now().isoformat()
                    
                    # Parse TLE for analysis
                    from .k20_physics import TLEProcessor
                    try:
                        orbital_state = TLEProcessor.tle_to_orbital_state(tle_lines, satellite_info['name'])
                        
                        # Analyze orbital elements
                        elements = orbital_state.orbital_elements
                        
                        # Check for anomalous orbital behavior
                        if elements['eccentricity'] < 0.001:
                            result['anomalies'].append({
                                'type': 'circular_orbit',
                                'description': 'Orbit is nearly circular (unusual for dead satellite)',
                                'severity': 'medium'
                            })
                            result['zombie_score'] += 0.2
                        
                        if elements['altitude'] > 1000000:  # Above 1000 km
                            result['anomalies'].append({
                                'type': 'high_altitude',
                                'description': f"High altitude: {elements['altitude']/1000:.0f} km",
                                'severity': 'low'
                            })
                        
                        result['orbital_elements'] = elements
                        
                    except Exception as e:
                        result['anomalies'].append({
                            'type': 'tle_parse_error',
                            'description': f"Failed to parse TLE: {str(e)}",
                            'severity': 'high'
                        })
        
        # Generate recommendations
        if result['zombie_score'] > 0.5:
            result['recommendations'].append("Consider further investigation - potential zombie satellite")
        elif result['zombie_score'] > 0.3:
            result['recommendations'].append("Monitor for orbital changes")
        
        if not result['has_current_tle']:
            result['recommendations'].append("Check historical databases for last known position")
        
        print(f"  Zombie Score: {result['zombie_score']:.2f}")
        print(f"  Anomalies: {len(result['anomalies'])}")
        
        return result
    
    def audit_all(self, max_satellites: int = 10) -> List[Dict]:
        """Audit multiple satellites"""
        satellites = self.fetcher.get_nasa_decommissioned_satellites()
        
        if max_satellites:
            satellites = satellites[:max_satellites]
        
        print(f"\n🔬 Starting audit of {len(satellites)} decommissioned NASA satellites")
        print("=" * 60)
        
        results = []
        for sat in satellites:
            try:
                result = self.audit_satellite(sat)
                results.append(result)
                
                # Save intermediate results
                self.save_results(results)
                
            except Exception as e:
                print(f"❌ Failed to audit {sat['name']}: {e}")
                results.append({
                    'satellite': sat['name'],
                    'error': str(e),
                    'audit_date': datetime.datetime.now().isoformat()
                })
        
        print("\n" + "=" * 60)
        print(f"✅ Audit complete: {len(results)} satellites analyzed")
        
        return results
    
    def save_results(self, results: List[Dict], filename: str = None):
        """Save audit results"""
        if not filename:
            filename = f"satellite_audit_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        
        from config.settings import OUTPUTS_DIR
        output_file = OUTPUTS_DIR / filename
        
        with open(output_file, 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"💾 Results saved to {output_file}")
        return output_file
    
    def generate_report(self, results: List[Dict]) -> Dict:
        """Generate comprehensive audit report"""
        total = len(results)
        with_tle = sum(1 for r in results if r.get('has_current_tle', False))
        high_zombie = sum(1 for r in results if r.get('zombie_score', 0) > 0.5)
        
        report = {
            'generated_at': datetime.datetime.now().isoformat(),
            'satellites_audited': total,
            'satellites_with_current_tle': with_tle,
            'potential_zombies': high_zombie,
            'zombie_rate': high_zombie / total if total > 0 else 0,
            'detailed_results': results,
            'summary': f"Audit identified {high_zombie} potential 'zombie' satellites "
                      f"({high_zombie/total*100:.1f}% of audited)"
        }
        
        return report