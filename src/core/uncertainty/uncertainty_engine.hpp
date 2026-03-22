/**
 * @file uncertainty_engine.hpp
 * @brief High-Performance Uncertainty Propagation Engine
 * 
 * ChiragRathi M17 Autonomous Celestial Discovery Framework
 * 
 * This module implements SIMD-optimized uncertainty propagation algorithms
 * for astronomical data processing with microsecond-level performance.
 * 
 * Mathematical Foundation:
 * For function f(x₁, x₂, ..., xₙ) with uncertain inputs, propagated uncertainty:
 * σ_f² = Σᵢ (∂f/∂xᵢ)² σ_xᵢ² + 2ΣᵢΣⱼ>ᵢ (∂f/∂xᵢ)(∂f/∂xⱼ) σ_xᵢxⱼ
 * 
 * @author ChiragRathi
 * @version 2.0.0
 * @date 2026
 * 
 * @copyright MIT License
 * Copyright (c) 2026 ChiragRathi
 */

#ifndef CHIRAGRATHI_M17_UNCERTAINTY_ENGINE_HPP
#define CHIRAGRATHI_M17_UNCERTAINTY_ENGINE_HPP

#include <vector>
#include <array>
#include <memory>
#include <string>
#include <chrono>
#include <immintrin.h>  // SIMD intrinsics
#include <Eigen/Dense>
#include <tbb/parallel_for.h>

namespace ChiragRathi {
namespace m17 {
namespace uncertainty {

/**
 * @brief Uncertain quantity with full covariance support
 * 
 * Represents a physical quantity with associated uncertainty and
 * correlations. Optimized for vectorized operations.
 */
class UncertainQuantity {
public:
    using Scalar = double;
    using Vector = Eigen::VectorXd;
    using Matrix = Eigen::MatrixXd;

private:
    Scalar value_;                  ///< Central value
    Scalar std_uncertainty_;        ///< Standard uncertainty  
    Vector correlations_;           ///< Correlation coefficients
    std::string source_id_;         ///< Data source identifier
    std::chrono::system_clock::time_point timestamp_;  ///< Creation time

public:
    /**
     * @brief Construct uncertain quantity
     * @param value Central value
     * @param uncertainty Standard uncertainty
     * @param source Source identifier
     */
    UncertainQuantity(Scalar value, Scalar uncertainty, 
                     const std::string& source = "");

    /**
     * @brief Construct with full covariance
     * @param value Central value
     * @param uncertainty Standard uncertainty
     * @param correlations Correlation vector
     * @param source Source identifier
     */
    UncertainQuantity(Scalar value, Scalar uncertainty,
                     const Vector& correlations,
                     const std::string& source = "");

    // Accessors
    Scalar value() const noexcept { return value_; }
    Scalar uncertainty() const noexcept { return std_uncertainty_; }
    const Vector& correlations() const noexcept { return correlations_; }
    const std::string& source() const noexcept { return source_id_; }

    // Arithmetic operations with uncertainty propagation
    UncertainQuantity operator+(const UncertainQuantity& other) const;
    UncertainQuantity operator-(const UncertainQuantity& other) const;
    UncertainQuantity operator*(const UncertainQuantity& other) const;
    UncertainQuantity operator/(const UncertainQuantity& other) const;
    
    // Mathematical functions
    UncertainQuantity pow(Scalar exponent) const;
    UncertainQuantity sqrt() const;
    UncertainQuantity log() const;
    UncertainQuantity exp() const;
    UncertainQuantity sin() const;
    UncertainQuantity cos() const;

    // Comparison with significance testing
    bool is_compatible_with(const UncertainQuantity& other, 
                           Scalar significance_level = 0.05) const;

    // Serialization
    std::string to_string() const;
    void to_binary(std::vector<uint8_t>& buffer) const;
    static UncertainQuantity from_binary(const std::vector<uint8_t>& buffer);
};

/**
 * @brief High-performance uncertainty propagator using SIMD
 * 
 * Implements vectorized uncertainty propagation for batched operations
 * with support for arbitrary functions and analytical derivatives.
 */
class SIMDUncertaintyPropagator {
public:
    using BatchVector = std::vector<UncertainQuantity>;
    using DerivativeFunction = std::function<Eigen::VectorXd(const Eigen::VectorXd&)>;

private:
    size_t batch_size_;             ///< Optimal batch size for SIMD
    size_t num_threads_;            ///< Thread pool size
    bool use_analytical_gradients_; ///< Use analytical vs numerical gradients

    // SIMD-optimized kernels
    void propagate_batch_avx512(const BatchVector& inputs, 
                               BatchVector& outputs,
                               const DerivativeFunction& derivatives) const;
    
    void propagate_batch_avx2(const BatchVector& inputs,
                             BatchVector& outputs, 
                             const DerivativeFunction& derivatives) const;

    // Numerical differentiation with error control
    Eigen::VectorXd compute_numerical_gradient(
        const std::function<double(const Eigen::VectorXd&)>& function,
        const Eigen::VectorXd& point,
        double h = 1e-8) const;

public:
    /**
     * @brief Construct propagator with optimization settings
     * @param batch_size Optimal batch size (0 = auto-detect)
     * @param num_threads Thread count (0 = hardware concurrency)
     * @param use_analytical Use analytical gradients when available
     */
    explicit SIMDUncertaintyPropagator(size_t batch_size = 0,
                                      size_t num_threads = 0,
                                      bool use_analytical = true);

    /**
     * @brief Propagate uncertainty through arbitrary function
     * @param inputs Input uncertain quantities
     * @param function Mathematical function f(x₁, x₂, ..., xₙ)
     * @param derivatives Analytical derivatives (optional)
     * @return Output with propagated uncertainty
     */
    UncertainQuantity propagate(const BatchVector& inputs,
                               const std::function<double(const Eigen::VectorXd&)>& function,
                               const DerivativeFunction& derivatives = nullptr) const;

    /**
     * @brief Batch propagation for high throughput
     * @param input_batches Vector of input batches
     * @param function Mathematical function
     * @param derivatives Analytical derivatives
     * @return Vector of outputs with propagated uncertainties
     */
    std::vector<UncertainQuantity> propagate_batch(
        const std::vector<BatchVector>& input_batches,
        const std::function<double(const Eigen::VectorXd&)>& function,
        const DerivativeFunction& derivatives = nullptr) const;

    // Performance monitoring
    struct PerformanceMetrics {
        size_t operations_processed;
        std::chrono::microseconds total_time;
        std::chrono::microseconds avg_latency;
        double throughput_ops_per_sec;
    };

    PerformanceMetrics get_performance_metrics() const;
    void reset_performance_metrics();
};

/**
 * @brief Real-time uncertainty stream processor
 * 
 * Lock-free implementation for processing continuous streams of
 * uncertain observations with guaranteed latency bounds.
 */
class RealTimeStreamProcessor {
public:
    using StreamCallback = std::function<void(const UncertainQuantity&)>;
    using ErrorCallback = std::function<void(const std::string&)>;

private:
    static constexpr size_t QUEUE_SIZE = 1048576;  ///< 1M element queue
    static constexpr size_t MAX_BATCH_SIZE = 10000;

    // Lock-free circular buffer
    std::array<UncertainQuantity, QUEUE_SIZE> queue_;
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
    
    // Processing components
    std::unique_ptr<SIMDUncertaintyPropagator> propagator_;
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> running_{false};
    
    // Callbacks
    StreamCallback output_callback_;
    ErrorCallback error_callback_;

    // Performance tracking
    mutable std::atomic<uint64_t> processed_count_{0};
    mutable std::atomic<uint64_t> error_count_{0};
    std::chrono::steady_clock::time_point start_time_;

    void worker_loop();
    bool try_enqueue(const UncertainQuantity& item);
    bool try_dequeue(UncertainQuantity& item);

public:
    /**
     * @brief Construct stream processor
     * @param output_callback Called for each processed result
     * @param error_callback Called on processing errors
     * @param num_workers Number of worker threads
     */
    RealTimeStreamProcessor(StreamCallback output_callback,
                           ErrorCallback error_callback,
                           size_t num_workers = 0);

    ~RealTimeStreamProcessor();

    /**
     * @brief Start processing stream
     */
    void start();

    /**
     * @brief Stop processing stream
     */
    void stop();

    /**
     * @brief Submit observation for processing
     * @param observation Input uncertain quantity
     * @return true if successfully queued, false if queue full
     */
    bool submit(const UncertainQuantity& observation);

    /**
     * @brief Submit batch of observations
     * @param observations Vector of uncertain quantities
     * @return Number of successfully queued items
     */
    size_t submit_batch(const std::vector<UncertainQuantity>& observations);

    // Performance monitoring
    struct StreamMetrics {
        uint64_t processed_count;
        uint64_t error_count;
        double throughput_ops_per_sec;
        double queue_utilization;
        std::chrono::microseconds avg_latency;
    };

    StreamMetrics get_metrics() const;

    // Configuration
    void set_batch_size(size_t batch_size);
    void set_latency_target(std::chrono::microseconds target);
};

/**
 * @brief Astronomical uncertainty calculations
 * 
 * Specialized functions for common astronomical uncertainty propagation
 * scenarios with optimized implementations.
 */
namespace astronomical {

/**
 * @brief Propagate astrometric uncertainties
 * @param ra Right ascension with uncertainty (degrees)
 * @param dec Declination with uncertainty (degrees)
 * @param epoch Observation epoch
 * @return Propagated position uncertainty
 */
UncertainQuantity propagate_astrometric(const UncertainQuantity& ra,
                                       const UncertainQuantity& dec,
                                       double epoch);

/**
 * @brief Propagate photometric uncertainties
 * @param magnitude Apparent magnitude with uncertainty
 * @param color Color index with uncertainty
 * @param extinction Extinction coefficient
 * @return Absolute magnitude with uncertainty
 */
UncertainQuantity propagate_photometric(const UncertainQuantity& magnitude,
                                       const UncertainQuantity& color,
                                       const UncertainQuantity& extinction);

/**
 * @brief Propagate orbital element uncertainties
 * @param elements Orbital elements (a, e, i, Ω, ω, M)
 * @param epoch Epoch of elements
 * @param target_time Target time for propagation
 * @return Position and velocity with uncertainties
 */
std::pair<Eigen::Vector3d, Eigen::Vector3d> 
propagate_orbital_elements(const std::vector<UncertainQuantity>& elements,
                          double epoch, double target_time);

/**
 * @brief Gravitational lensing uncertainty propagation
 * @param source_position Source position with uncertainty
 * @param lens_mass Lens mass with uncertainty
 * @param lens_distance Lens distance with uncertainty
 * @return Lensed image positions and magnifications
 */
struct LensingResult {
    std::vector<UncertainQuantity> image_positions;
    std::vector<UncertainQuantity> magnifications;
    UncertainQuantity time_delay;
};

LensingResult propagate_gravitational_lensing(
    const std::pair<UncertainQuantity, UncertainQuantity>& source_position,
    const UncertainQuantity& lens_mass,
    const UncertainQuantity& lens_distance);

} // namespace astronomical

} // namespace uncertainty
} // namespace m17
} // namespace ChiragRathi

#endif // CHIRAGRATHI_M17_UNCERTAINTY_ENGINE_HPP
