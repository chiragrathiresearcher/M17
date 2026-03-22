/**
 * @file relativistic_physics.cpp
 * @brief Implementation of high-precision relativistic orbital mechanics
 * 
 * ChiragRathi M17 Autonomous Celestial Discovery Framework
 * 
 * @author ChiragRathi
 * @version 2.0.0
 * @date 2026
 */

#include "relativistic_physics.hpp"
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>

namespace ChiragRathi {
namespace m17 {
namespace physics {

// RelativisticOrbitalState Implementation

RelativisticOrbitalState::RelativisticOrbitalState(const Vector3d& position,
                                                 const Vector3d& velocity,
                                                 double epoch,
                                                 ReferenceFrame frame,
                                                 double mass)
    : position_(position)
    , velocity_(velocity)
    , acceleration_(Vector3d::Zero())
    , position_covariance_(Matrix3d::Identity() * 1e-6)  // Default 1mm position uncertainty
    , velocity_covariance_(Matrix3d::Identity() * 1e-9)  // Default 1mm/s velocity uncertainty
    , pv_cross_covariance_(Matrix3d::Zero())
    , frame_(frame)
    , epoch_(epoch)
    , mass_(mass)
    , keplerian_valid_(false) {
}

RelativisticOrbitalState::RelativisticOrbitalState(const Vector3d& position,
                                                 const Vector3d& velocity,
                                                 const Matrix3d& pos_cov,
                                                 const Matrix3d& vel_cov,
                                                 const Matrix3d& pv_cross_cov,
                                                 double epoch,
                                                 ReferenceFrame frame,
                                                 double mass)
    : position_(position)
    , velocity_(velocity)
    , acceleration_(Vector3d::Zero())
    , position_covariance_(pos_cov)
    , velocity_covariance_(vel_cov)
    , pv_cross_covariance_(pv_cross_cov)
    , frame_(frame)
    , epoch_(epoch)
    , mass_(mass)
    , keplerian_valid_(false) {
}

void RelativisticOrbitalState::set_position(const Vector3d& pos) {
    position_ = pos;
    keplerian_valid_ = false;
}

void RelativisticOrbitalState::set_velocity(const Vector3d& vel) {
    velocity_ = vel;
    keplerian_valid_ = false;
}

void RelativisticOrbitalState::set_acceleration(const Vector3d& acc) {
    acceleration_ = acc;
}

void RelativisticOrbitalState::set_covariance(const Matrix3d& pos_cov, 
                                             const Matrix3d& vel_cov, 
                                             const Matrix3d& pv_cross_cov) {
    position_covariance_ = pos_cov;
    velocity_covariance_ = vel_cov;
    pv_cross_covariance_ = pv_cross_cov;
}

void RelativisticOrbitalState::set_epoch(double epoch) {
    epoch_ = epoch;
}

std::array<double, 6> RelativisticOrbitalState::keplerian_elements() const {
    if (keplerian_valid_) {
        return keplerian_elements_;
    }
    
    // Convert Cartesian to Keplerian elements
    const double mu = PhysicalConstants::GM_EARTH;
    const Vector3d r = position_;
    const Vector3d v = velocity_;
    
    const double r_mag = r.norm();
    const double v_mag = v.norm();
    
    // Specific orbital energy
    const double energy = 0.5 * v_mag * v_mag - mu / r_mag;
    
    // Semi-major axis
    const double a = -mu / (2.0 * energy);
    
    // Angular momentum vector
    const Vector3d h = r.cross(v);
    const double h_mag = h.norm();
    
    // Eccentricity vector
    const Vector3d e_vec = ((v_mag * v_mag - mu / r_mag) * r - r.dot(v) * v) / mu;
    const double e = e_vec.norm();
    
    // Inclination
    const double i = std::acos(h(2) / h_mag);
    
    // Longitude of ascending node
    const Vector3d n = Vector3d(-h(1), h(0), 0.0);
    const double n_mag = n.norm();
    double Omega = 0.0;
    if (n_mag > 1e-15) {
        Omega = std::acos(n(0) / n_mag);
        if (n(1) < 0) Omega = 2.0 * M_PI - Omega;
    }
    
    // Argument of periapsis
    double omega = 0.0;
    if (n_mag > 1e-15 && e > 1e-15) {
        omega = std::acos(n.dot(e_vec) / (n_mag * e));
        if (e_vec(2) < 0) omega = 2.0 * M_PI - omega;
    }
    
    // True anomaly
    double nu = 0.0;
    if (e > 1e-15) {
        nu = std::acos(e_vec.dot(r) / (e * r_mag));
        if (r.dot(v) < 0) nu = 2.0 * M_PI - nu;
    }
    
    keplerian_elements_ = {a, e, i, Omega, omega, nu};
    keplerian_valid_ = true;
    
    return keplerian_elements_;
}

double RelativisticOrbitalState::semimajor_axis() const {
    return keplerian_elements()[0];
}

double RelativisticOrbitalState::eccentricity() const {
    return keplerian_elements()[1];
}

double RelativisticOrbitalState::inclination() const {
    return keplerian_elements()[2];
}

double RelativisticOrbitalState::period() const {
    const double a = semimajor_axis();
    const double mu = PhysicalConstants::GM_EARTH;
    return 2.0 * M_PI * std::sqrt(a * a * a / mu);
}

double RelativisticOrbitalState::gravitational_redshift() const {
    const double r = position_.norm();
    const double GM = PhysicalConstants::GM_EARTH;
    const double c2 = PhysicalConstants::SPEED_OF_LIGHT * PhysicalConstants::SPEED_OF_LIGHT;
    
    return GM / (r * c2);
}

double RelativisticOrbitalState::special_relativistic_factor() const {
    const double v2 = velocity_.squaredNorm();
    const double c2 = PhysicalConstants::SPEED_OF_LIGHT * PhysicalConstants::SPEED_OF_LIGHT;
    
    return std::sqrt(1.0 - v2 / c2);
}

double RelativisticOrbitalState::time_dilation_rate() const {
    // Combined gravitational and special relativistic time dilation
    const double gravitational = gravitational_redshift();
    const double special = 1.0 - special_relativistic_factor();
    
    return gravitational - special;
}

uncertainty::UncertainQuantity RelativisticOrbitalState::position_magnitude() const {
    const double r = position_.norm();
    
    // Propagate position uncertainty to magnitude
    if (r > 1e-15) {
        const Vector3d unit_r = position_ / r;
        const double sigma_r = std::sqrt(unit_r.transpose() * position_covariance_ * unit_r);
        return uncertainty::UncertainQuantity(r, sigma_r, "position_magnitude");
    } else {
        return uncertainty::UncertainQuantity(0.0, 0.0, "position_magnitude");
    }
}

uncertainty::UncertainQuantity RelativisticOrbitalState::velocity_magnitude() const {
    const double v = velocity_.norm();
    
    if (v > 1e-15) {
        const Vector3d unit_v = velocity_ / v;
        const double sigma_v = std::sqrt(unit_v.transpose() * velocity_covariance_ * unit_v);
        return uncertainty::UncertainQuantity(v, sigma_v, "velocity_magnitude");
    } else {
        return uncertainty::UncertainQuantity(0.0, 0.0, "velocity_magnitude");
    }
}

// AdvancedPhysicsEngine Implementation

AdvancedPhysicsEngine::AdvancedPhysicsEngine(const PerturbationConfig& config)
    : config_(config) {
    
    // Initialize gravitational field coefficients (simplified)
    if (config_.enable_earth_gravity) {
        C_coefficients_.resize(config_.gravity_degree + 1);
        S_coefficients_.resize(config_.gravity_degree + 1);
        
        for (int l = 0; l <= config_.gravity_degree; ++l) {
            C_coefficients_[l].resize(l + 1, 0.0);
            S_coefficients_[l].resize(l + 1, 0.0);
        }
        
        // Set key coefficients
        C_coefficients_[0][0] = 1.0;  // Central term
        C_coefficients_[2][0] = -PhysicalConstants::J2_EARTH;  // J2
    }
    
    // Default atmosphere model (exponential)
    atmosphere_density_ = [](const Vector3d& pos, double epoch) {
        const double altitude = pos.norm() - PhysicalConstants::R_EARTH_EQUATORIAL;
        const double h0 = 8500.0;  // Scale height in meters
        const double rho0 = 1.225;  // Sea level density kg/m³
        
        if (altitude < 0) return rho0;
        return rho0 * std::exp(-altitude / h0);
    };
}

Vector3d AdvancedPhysicsEngine::compute_acceleration(const RelativisticOrbitalState& state) const {
    Vector3d total_acceleration = Vector3d::Zero();
    
    const Vector3d& pos = state.position();
    const Vector3d& vel = state.velocity();
    const double epoch = state.epoch();
    
    // Newtonian gravity
    total_acceleration += newtonian_gravity(pos);
    
    if (config_.enable_relativity) {
        // Post-Newtonian corrections
        total_acceleration += schwarzschild_correction(pos, vel);
        total_acceleration += lense_thirring_correction(pos, vel);
        total_acceleration += geodetic_precession(pos, vel);
    }
    
    if (config_.enable_earth_gravity) {
        // Earth gravity field
        total_acceleration += earth_gravity_field(pos, epoch);
    }
    
    if (config_.enable_third_body) {
        // Third-body perturbations
        total_acceleration += third_body_perturbation(pos, epoch);
    }
    
    if (config_.enable_solar_radiation) {
        // Solar radiation pressure
        total_acceleration += solar_radiation_pressure(pos, vel, epoch);
    }
    
    if (config_.enable_atmospheric_drag) {
        // Atmospheric drag
        total_acceleration += atmospheric_drag(pos, vel, epoch);
    }
    
    return total_acceleration;
}

Vector3d AdvancedPhysicsEngine::newtonian_gravity(const Vector3d& position) const {
    const double r = position.norm();
    
    if (r < PhysicalConstants::R_EARTH_EQUATORIAL) {
        // Inside Earth - should not happen for satellites
        return Vector3d::Zero();
    }
    
    const double GM = PhysicalConstants::GM_EARTH;
    return -GM * position / (r * r * r);
}

Vector3d AdvancedPhysicsEngine::schwarzschild_correction(const Vector3d& position, 
                                                        const Vector3d& velocity) const {
    const double r = position.norm();
    const double v2 = velocity.squaredNorm();
    const double rv_dot = position.dot(velocity);
    
    if (r < PhysicalConstants::R_EARTH_EQUATORIAL) {
        return Vector3d::Zero();
    }
    
    const double GM = PhysicalConstants::GM_EARTH;
    const double c2 = PhysicalConstants::SPEED_OF_LIGHT * PhysicalConstants::SPEED_OF_LIGHT;
    
    // First-order post-Newtonian correction
    const double term1 = 4.0 * GM / r - v2;
    const Vector3d term2 = 4.0 * rv_dot * velocity / r;
    
    const double coefficient = GM / (r * r * r * c2);
    
    return coefficient * (term1 * position + term2);
}

Vector3d AdvancedPhysicsEngine::lense_thirring_correction(const Vector3d& position,
                                                        const Vector3d& velocity) const {
    const double r = position.norm();
    
    if (r < PhysicalConstants::R_EARTH_EQUATORIAL) {
        return Vector3d::Zero();
    }
    
    const double GM = PhysicalConstants::GM_EARTH;
    const double c2 = PhysicalConstants::SPEED_OF_LIGHT * PhysicalConstants::SPEED_OF_LIGHT;
    const double J_earth = PhysicalConstants::EARTH_ANGULAR_MOMENTUM;
    
    // Earth's angular momentum vector (simplified - along Z axis)
    const Vector3d J_vec(0.0, 0.0, J_earth);
    
    // Frame-dragging acceleration
    const double coefficient = 2.0 * GM / (c2 * r * r * r);
    const Vector3d cross_product = J_vec.cross(position);
    
    return coefficient * cross_product / (r * r);
}

Vector3d AdvancedPhysicsEngine::geodetic_precession(const Vector3d& position,
                                                   const Vector3d& velocity) const {
    // Simplified geodetic precession effect
    const double r = position.norm();
    
    if (r < PhysicalConstants::R_EARTH_EQUATORIAL) {
        return Vector3d::Zero();
    }
    
    const double GM = PhysicalConstants::GM_EARTH;
    const double c2 = PhysicalConstants::SPEED_OF_LIGHT * PhysicalConstants::SPEED_OF_LIGHT;
    
    // Geodetic precession rate
    const double omega_g = 1.9e-8;  // rad/s for Earth
    const Vector3d omega_vec(0.0, 0.0, omega_g);
    
    // Additional acceleration due to geodetic precession
    return 2.0 * omega_vec.cross(velocity);
}

Vector3d AdvancedPhysicsEngine::earth_gravity_field(const Vector3d& position, double epoch) const {
    // Spherical harmonic expansion of Earth's gravity field
    const double r = position.norm();
    const double x = position(0);
    const double y = position(1);
    const double z = position(2);
    
    if (r < PhysicalConstants::R_EARTH_EQUATORIAL) {
        return Vector3d::Zero();
    }
    
    const double GM = PhysicalConstants::GM_EARTH;
    const double R_e = PhysicalConstants::R_EARTH_EQUATORIAL;
    
    // Convert to spherical coordinates
    const double lat = std::asin(z / r);
    const double lon = std::atan2(y, x);
    
    Vector3d acceleration = Vector3d::Zero();
    
    // J2 term (dominant oblateness effect)
    const double J2 = PhysicalConstants::J2_EARTH;
    const double sin_lat = std::sin(lat);
    const double cos_lat = std::cos(lat);
    const double Re_r_2 = (R_e / r) * (R_e / r);
    
    const double factor = 1.5 * J2 * GM * Re_r_2 / (r * r * r);
    
    // Acceleration in spherical coordinates
    const double a_r = factor * (1.0 - 3.0 * sin_lat * sin_lat);
    const double a_lat = factor * sin_lat * cos_lat;
    
    // Convert back to Cartesian
    const double cos_lon = std::cos(lon);
    const double sin_lon = std::sin(lon);
    
    acceleration(0) = a_r * cos_lat * cos_lon - a_lat * sin_lat * cos_lon;
    acceleration(1) = a_r * cos_lat * sin_lon - a_lat * sin_lat * sin_lon;
    acceleration(2) = a_r * sin_lat + a_lat * cos_lat;
    
    return acceleration;
}

Vector3d AdvancedPhysicsEngine::third_body_perturbation(const Vector3d& position, double epoch) const {
    Vector3d acceleration = Vector3d::Zero();
    
    if (sun_ephemeris_) {
        // Sun perturbation
        const Vector3d sun_pos = sun_ephemeris_(epoch);
        const Vector3d rel_pos = position - sun_pos;
        const double r_sun = sun_pos.norm();
        const double r_rel = rel_pos.norm();
        
        const double GM_sun = PhysicalConstants::GM_SUN;
        
        if (r_sun > 1e6 && r_rel > 1e6) {  // Avoid singularities
            acceleration += -GM_sun * (rel_pos / (r_rel * r_rel * r_rel) + 
                                     sun_pos / (r_sun * r_sun * r_sun));
        }
    }
    
    if (moon_ephemeris_) {
        // Moon perturbation
        const Vector3d moon_pos = moon_ephemeris_(epoch);
        const Vector3d rel_pos = position - moon_pos;
        const double r_moon = moon_pos.norm();
        const double r_rel = rel_pos.norm();
        
        const double GM_moon = PhysicalConstants::GM_MOON;
        
        if (r_moon > 1e6 && r_rel > 1e6) {  // Avoid singularities
            acceleration += -GM_moon * (rel_pos / (r_rel * r_rel * r_rel) + 
                                      moon_pos / (r_moon * r_moon * r_moon));
        }
    }
    
    return acceleration;
}

Vector3d AdvancedPhysicsEngine::solar_radiation_pressure(const Vector3d& position,
                                                        const Vector3d& velocity,
                                                        double epoch) const {
    // Simplified solar radiation pressure model
    const double AU = PhysicalConstants::AU;
    const double solar_flux = 1361.0;  // W/m² at 1 AU
    const double c = PhysicalConstants::SPEED_OF_LIGHT;
    
    // Default Sun position (simplified - assume Sun at origin for SRP calculation)
    Vector3d sun_direction = -position.normalized();
    const double sun_distance = position.norm();
    
    // Radiation pressure at distance
    const double flux = solar_flux * AU * AU / (sun_distance * sun_distance);
    const double pressure = flux / c;
    
    // Acceleration depends on area-to-mass ratio and reflectivity
    const double area_mass_ratio = config_.area_to_mass_ratio;
    const double reflectivity = config_.reflectivity;
    
    const double acceleration_magnitude = pressure * area_mass_ratio * (1.0 + reflectivity);
    
    return acceleration_magnitude * sun_direction;
}

Vector3d AdvancedPhysicsEngine::atmospheric_drag(const Vector3d& position,
                                                const Vector3d& velocity,
                                                double epoch) const {
    const double altitude = position.norm() - PhysicalConstants::R_EARTH_EQUATORIAL;
    
    if (altitude < 0 || altitude > 1000e3) {  // Below surface or above 1000 km
        return Vector3d::Zero();
    }
    
    // Atmospheric density
    const double rho = atmosphere_density_(position, epoch);
    
    if (rho < 1e-15) {
        return Vector3d::Zero();
    }
    
    // Relative velocity (accounting for Earth rotation)
    const double omega_earth = PhysicalConstants::EARTH_ROTATION_RATE;
    const Vector3d earth_rotation(0.0, 0.0, omega_earth);
    const Vector3d atmosphere_velocity = earth_rotation.cross(position);
    const Vector3d relative_velocity = velocity - atmosphere_velocity;
    
    const double v_rel = relative_velocity.norm();
    
    if (v_rel < 1e-6) {
        return Vector3d::Zero();
    }
    
    // Drag acceleration
    const double drag_coeff = config_.drag_coefficient;
    const double area_mass_ratio = config_.area_to_mass_ratio;
    
    const double acceleration_magnitude = -0.5 * drag_coeff * rho * area_mass_ratio * v_rel;
    
    return acceleration_magnitude * relative_velocity.normalized();
}

RelativisticOrbitalState AdvancedPhysicsEngine::propagate_orbit(
    const RelativisticOrbitalState& initial_state,
    double time_span,
    double tolerance) const {
    
    // State vector: [x, y, z, vx, vy, vz]
    std::vector<double> state = {
        initial_state.position()(0), initial_state.position()(1), initial_state.position()(2),
        initial_state.velocity()(0), initial_state.velocity()(1), initial_state.velocity()(2)
    };
    
    // Integrate using 4th-order Runge-Kutta with adaptive step size
    const double initial_step = std::min(60.0, std::abs(time_span) / 1000.0);  // 1 minute or 1/1000 of span
    double current_time = 0.0;
    double step_size = initial_step;
    
    while (std::abs(current_time) < std::abs(time_span)) {
        // Ensure we don't overshoot
        if (std::abs(current_time + step_size) > std::abs(time_span)) {
            step_size = time_span - current_time;
        }
        
        // Current orbital state
        Vector3d pos(state[0], state[1], state[2]);
        Vector3d vel(state[3], state[4], state[5]);
        RelativisticOrbitalState current_orbital_state(pos, vel, 
                                                      initial_state.epoch() + current_time/86400.0,
                                                      initial_state.frame(),
                                                      initial_state.mass());
        
        // Compute acceleration
        Vector3d acc = compute_acceleration(current_orbital_state);
        
        // RK4 integration step
        std::vector<double> k1(6), k2(6), k3(6), k4(6);
        
        // k1
        k1[0] = vel(0); k1[1] = vel(1); k1[2] = vel(2);
        k1[3] = acc(0); k1[4] = acc(1); k1[5] = acc(2);
        
        // k2
        Vector3d pos2 = pos + Vector3d(k1[0], k1[1], k1[2]) * step_size * 0.5;
        Vector3d vel2 = vel + Vector3d(k1[3], k1[4], k1[5]) * step_size * 0.5;
        RelativisticOrbitalState state2(pos2, vel2, 
                                       initial_state.epoch() + (current_time + step_size*0.5)/86400.0,
                                       initial_state.frame(), initial_state.mass());
        Vector3d acc2 = compute_acceleration(state2);
        k2[0] = vel2(0); k2[1] = vel2(1); k2[2] = vel2(2);
        k2[3] = acc2(0); k2[4] = acc2(1); k2[5] = acc2(2);
        
        // k3
        Vector3d pos3 = pos + Vector3d(k2[0], k2[1], k2[2]) * step_size * 0.5;
        Vector3d vel3 = vel + Vector3d(k2[3], k2[4], k2[5]) * step_size * 0.5;
        RelativisticOrbitalState state3(pos3, vel3,
                                       initial_state.epoch() + (current_time + step_size*0.5)/86400.0,
                                       initial_state.frame(), initial_state.mass());
        Vector3d acc3 = compute_acceleration(state3);
        k3[0] = vel3(0); k3[1] = vel3(1); k3[2] = vel3(2);
        k3[3] = acc3(0); k3[4] = acc3(1); k3[5] = acc3(2);
        
        // k4
        Vector3d pos4 = pos + Vector3d(k3[0], k3[1], k3[2]) * step_size;
        Vector3d vel4 = vel + Vector3d(k3[3], k3[4], k3[5]) * step_size;
        RelativisticOrbitalState state4(pos4, vel4,
                                       initial_state.epoch() + (current_time + step_size)/86400.0,
                                       initial_state.frame(), initial_state.mass());
        Vector3d acc4 = compute_acceleration(state4);
        k4[0] = vel4(0); k4[1] = vel4(1); k4[2] = vel4(2);
        k4[3] = acc4(0); k4[4] = acc4(1); k4[5] = acc4(2);
        
        // Update state
        for (int i = 0; i < 6; ++i) {
            state[i] += step_size * (k1[i] + 2*k2[i] + 2*k3[i] + k4[i]) / 6.0;
        }
        
        current_time += step_size;
        
        // Adaptive step size (simplified)
        // In a full implementation, this would use error estimation
        const double max_step = 3600.0;  // 1 hour maximum
        const double min_step = 0.1;     // 0.1 second minimum
        step_size = std::max(min_step, std::min(max_step, step_size));
    }
    
    // Create final state
    Vector3d final_pos(state[0], state[1], state[2]);
    Vector3d final_vel(state[3], state[4], state[5]);
    
    RelativisticOrbitalState final_state(final_pos, final_vel,
                                        initial_state.epoch() + time_span/86400.0,
                                        initial_state.frame(),
                                        initial_state.mass());
    
    // Set final acceleration
    final_state.set_acceleration(compute_acceleration(final_state));
    
    return final_state;
}

} // namespace physics
} // namespace m17
} // namespace ChiragRathi
