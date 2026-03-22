/**
 * @file uncertainty_engine.cpp
 * @brief High-Performance Uncertainty Propagation Implementation
 * 
 * ChiragRathi M17 Autonomous Celestial Discovery Framework
 * 
 * @author ChiragRathi
 * @version 2.0.0
 * @date 2026
 */

#include "uncertainty_engine.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <thread>
#include <random>

namespace ChiragRathi {
namespace m17 {
namespace uncertainty {

// UncertainQuantity Implementation

UncertainQuantity::UncertainQuantity(Scalar value, Scalar uncertainty, 
                                   const std::string& source)
    : value_(value)
    , std_uncertainty_(std::abs(uncertainty))
    , source_id_(source)
    , timestamp_(std::chrono::system_clock::now()) {
    
    if (std_uncertainty_ < 0) {
        throw std::invalid_argument("Uncertainty must be non-negative");
    }
}

UncertainQuantity::UncertainQuantity(Scalar value, Scalar uncertainty,
                                   const Vector& correlations,
                                   const std::string& source)
    : value_(value)
    , std_uncertainty_(std::abs(uncertainty))
    , correlations_(correlations)
    , source_id_(source)
    , timestamp_(std::chrono::system_clock::now()) {
    
    if (std_uncertainty_ < 0) {
        throw std::invalid_argument("Uncertainty must be non-negative");
    }
}

UncertainQuantity UncertainQuantity::operator+(const UncertainQuantity& other) const {
    // Addition: σ²(a+b) = σ²(a) + σ²(b) + 2ρσ(a)σ(b)
    Scalar result_value = value_ + other.value_;
    
    // Compute correlation coefficient (simplified)
    Scalar correlation = 0.0;
    if (!correlations_.empty() && !other.correlations_.empty()) {
        correlation = correlations_.dot(other.correlations_) / 
                     (correlations_.norm() * other.correlations_.norm());
    }
    
    Scalar result_uncertainty = std::sqrt(
        std_uncertainty_ * std_uncertainty_ +
        other.std_uncertainty_ * other.std_uncertainty_ +
        2.0 * correlation * std_uncertainty_ * other.std_uncertainty_
    );
    
    std::string combined_source = source_id_ + "+" + other.source_id_;
    
    return UncertainQuantity(result_value, result_uncertainty, combined_source);
}

UncertainQuantity UncertainQuantity::operator-(const UncertainQuantity& other) const {
    // Subtraction: σ²(a-b) = σ²(a) + σ²(b) - 2ρσ(a)σ(b)
    Scalar result_value = value_ - other.value_;
    
    Scalar correlation = 0.0;
    if (!correlations_.empty() && !other.correlations_.empty()) {
        correlation = correlations_.dot(other.correlations_) / 
                     (correlations_.norm() * other.correlations_.norm());
    }
    
    Scalar result_uncertainty = std::sqrt(
        std_uncertainty_ * std_uncertainty_ +
        other.std_uncertainty_ * other.std_uncertainty_ -
        2.0 * correlation * std_uncertainty_ * other.std_uncertainty_
    );
    
    std::string combined_source = source_id_ + "-" + other.source_id_;
    
    return UncertainQuantity(result_value, result_uncertainty, combined_source);
}

UncertainQuantity UncertainQuantity::operator*(const UncertainQuantity& other) const {
    // Multiplication: σ²(ab) = (σ(a)/a)² + (σ(b)/b)² + 2ρ(σ(a)/a)(σ(b)/b)
    Scalar result_value = value_ * other.value_;
    
    if (std::abs(value_) < 1e-15 || std::abs(other.value_) < 1e-15) {
        return UncertainQuantity(result_value, 0.0, source_id_ + "*" + other.source_id_);
    }
    
    Scalar rel_unc_a = std_uncertainty_ / std::abs(value_);
    Scalar rel_unc_b = other.std_uncertainty_ / std::abs(other.value_);
    
    Scalar correlation = 0.0;
    if (!correlations_.empty() && !other.correlations_.empty()) {
        correlation = correlations_.dot(other.correlations_) / 
                     (correlations_.norm() * other.correlations_.norm());
    }
    
    Scalar relative_uncertainty = std::sqrt(
        rel_unc_a * rel_unc_a +
        rel_unc_b * rel_unc_b +
        2.0 * correlation * rel_unc_a * rel_unc_b
    );
    
    Scalar result_uncertainty = relative_uncertainty * std::abs(result_value);
    
    return UncertainQuantity(result_value, result_uncertainty, 
                           source_id_ + "*" + other.source_id_);
}

UncertainQuantity UncertainQuantity::operator/(const UncertainQuantity& other) const {
    if (std::abs(other.value_) < 1e-15) {
        throw std::domain_error("Division by zero in uncertain quantity");
    }
    
    // Division: same relative uncertainty formula as multiplication
    Scalar result_value = value_ / other.value_;
    
    Scalar rel_unc_a = std_uncertainty_ / std::abs(value_);
    Scalar rel_unc_b = other.std_uncertainty_ / std::abs(other.value_);
    
    Scalar correlation = 0.0;
    if (!correlations_.empty() && !other.correlations_.empty()) {
        correlation = correlations_.dot(other.correlations_) / 
                     (correlations_.norm() * other.correlations_.norm());
    }
    
    Scalar relative_uncertainty = std::sqrt(
        rel_unc_a * rel_unc_a +
        rel_unc_b * rel_unc_b -
        2.0 * correlation * rel_unc_a * rel_unc_b
    );
    
    Scalar result_uncertainty = relative_uncertainty * std::abs(result_value);
    
    return UncertainQuantity(result_value, result_uncertainty,
                           source_id_ + "/" + other.source_id_);
}

UncertainQuantity UncertainQuantity::pow(Scalar exponent) const {
    // Power: σ(x^n) = |n| |x|^(n-1) σ(x)
    Scalar result_value = std::pow(value_, exponent);
    
    Scalar derivative = exponent * std::pow(value_, exponent - 1.0);
    Scalar result_uncertainty = std::abs(derivative) * std_uncertainty_;
    
    return UncertainQuantity(result_value, result_uncertainty,
                           source_id_ + "^" + std::to_string(exponent));
}

UncertainQuantity UncertainQuantity::sqrt() const {
    if (value_ < 0) {
        throw std::domain_error("Square root of negative uncertain quantity");
    }
    
    return pow(0.5);
}

UncertainQuantity UncertainQuantity::log() const {
    if (value_ <= 0) {
        throw std::domain_error("Logarithm of non-positive uncertain quantity");
    }
    
    // ln(x): σ(ln(x)) = σ(x)/x
    Scalar result_value = std::log(value_);
    Scalar result_uncertainty = std_uncertainty_ / value_;
    
    return UncertainQuantity(result_value, result_uncertainty,
                           "log(" + source_id_ + ")");
}

UncertainQuantity UncertainQuantity::exp() const {
    // exp(x): σ(e^x) = e^x σ(x)
    Scalar result_value = std::exp(value_);
    Scalar result_uncertainty = result_value * std_uncertainty_;
    
    return UncertainQuantity(result_value, result_uncertainty,
                           "exp(" + source_id_ + ")");
}

UncertainQuantity UncertainQuantity::sin() const {
    // sin(x): σ(sin(x)) = |cos(x)| σ(x)
    Scalar result_value = std::sin(value_);
    Scalar derivative = std::cos(value_);
    Scalar result_uncertainty = std::abs(derivative) * std_uncertainty_;
    
    return UncertainQuantity(result_value, result_uncertainty,
                           "sin(" + source_id_ + ")");
}

UncertainQuantity UncertainQuantity::cos() const {
    // cos(x): σ(cos(x)) = |sin(x)| σ(x)
    Scalar result_value = std::cos(value_);
    Scalar derivative = -std::sin(value_);
    Scalar result_uncertainty = std::abs(derivative) * std_uncertainty_;
    
    return UncertainQuantity(result_value, result_uncertainty,
                           "cos(" + source_id_ + ")");
}

bool UncertainQuantity::is_compatible_with(const UncertainQuantity& other, 
                                         Scalar significance_level) const {
    // Statistical compatibility test using normal distribution
    Scalar difference = std::abs(value_ - other.value_);
    Scalar combined_uncertainty = std::sqrt(
        std_uncertainty_ * std_uncertainty_ + 
        other.std_uncertainty_ * other.std_uncertainty_
    );
    
    if (combined_uncertainty < 1e-15) {
        return std::abs(difference) < 1e-15;
    }
    
    Scalar z_score = difference / combined_uncertainty;
    
    // Two-sided test with normal distribution
    Scalar critical_value = -std::log(significance_level / 2.0) / std::sqrt(2.0 * M_PI);
    
    return z_score <= critical_value;
}

std::string UncertainQuantity::to_string() const {
    std::ostringstream oss;
    oss << value_ << " ± " << std_uncertainty_;
    if (!source_id_.empty()) {
        oss << " [" << source_id_ << "]";
    }
    return oss.str();
}

// SIMDUncertaintyPropagator Implementation

SIMDUncertaintyPropagator::SIMDUncertaintyPropagator(size_t batch_size,
                                                   size_t num_threads,
                                                   bool use_analytical)
    : use_analytical_gradients_(use_analytical) {
    
    // Auto-detect optimal batch size for SIMD
    if (batch_size == 0) {
        // Target: fill one cache line per batch for optimal memory access
        batch_size_ = 64; // Typical for AVX-512
    } else {
        batch_size_ = batch_size;
    }
    
    // Auto-detect thread count
    if (num_threads == 0) {
        num_threads_ = std::thread::hardware_concurrency();
    } else {
        num_threads_ = num_threads;
    }
}

void SIMDUncertaintyPropagator::propagate_batch_avx512(
    const BatchVector& inputs, 
    BatchVector& outputs,
    const DerivativeFunction& derivatives) const {
    
#ifdef __AVX512F__
    const size_t simd_width = 8; // 8 doubles in AVX-512
    const size_t num_complete_batches = inputs.size() / simd_width;
    
    // Process complete SIMD batches
    for (size_t batch = 0; batch < num_complete_batches; ++batch) {
        size_t start_idx = batch * simd_width;
        
        // Load values into SIMD registers
        __m512d values = _mm512_loadu_pd(&inputs[start_idx].value());
        
        // Apply function (simplified - would need vectorized implementation)
        // This is a placeholder for actual SIMD function implementation
        __m512d results = _mm512_mul_pd(values, values); // x^2 example
        
        // Store results
        alignas(64) double result_array[simd_width];
        _mm512_storeu_pd(result_array, results);
        
        for (size_t i = 0; i < simd_width; ++i) {
            outputs[start_idx + i] = UncertainQuantity(
                result_array[i],
                inputs[start_idx + i].uncertainty() * 2.0 * inputs[start_idx + i].value(), // d/dx(x²) = 2x
                "simd_" + inputs[start_idx + i].source()
            );
        }
    }
    
    // Handle remaining elements
    for (size_t i = num_complete_batches * simd_width; i < inputs.size(); ++i) {
        double x = inputs[i].value();
        double result = x * x;
        double uncertainty = inputs[i].uncertainty() * 2.0 * x;
        outputs[i] = UncertainQuantity(result, uncertainty, 
                                     "scalar_" + inputs[i].source());
    }
#else
    // Fallback to scalar implementation
    propagate_batch_avx2(inputs, outputs, derivatives);
#endif
}

void SIMDUncertaintyPropagator::propagate_batch_avx2(
    const BatchVector& inputs,
    BatchVector& outputs, 
    const DerivativeFunction& derivatives) const {
    
#ifdef __AVX2__
    const size_t simd_width = 4; // 4 doubles in AVX2
    const size_t num_complete_batches = inputs.size() / simd_width;
    
    // Process complete SIMD batches
    for (size_t batch = 0; batch < num_complete_batches; ++batch) {
        size_t start_idx = batch * simd_width;
        
        // Load values into SIMD registers
        __m256d values = _mm256_loadu_pd(
            reinterpret_cast<const double*>(&inputs[start_idx])
        );
        
        // Apply function (example: x^2)
        __m256d results = _mm256_mul_pd(values, values);
        
        // Store results
        alignas(32) double result_array[simd_width];
        _mm256_storeu_pd(result_array, results);
        
        for (size_t i = 0; i < simd_width; ++i) {
            outputs[start_idx + i] = UncertainQuantity(
                result_array[i],
                inputs[start_idx + i].uncertainty() * 2.0 * inputs[start_idx + i].value(),
                "avx2_" + inputs[start_idx + i].source()
            );
        }
    }
    
    // Handle remaining elements
    for (size_t i = num_complete_batches * simd_width; i < inputs.size(); ++i) {
        double x = inputs[i].value();
        double result = x * x;
        double uncertainty = inputs[i].uncertainty() * 2.0 * x;
        outputs[i] = UncertainQuantity(result, uncertainty,
                                     "scalar_" + inputs[i].source());
    }
#else
    // Fallback to scalar implementation
    for (size_t i = 0; i < inputs.size(); ++i) {
        double x = inputs[i].value();
        double result = x * x;
        double uncertainty = inputs[i].uncertainty() * 2.0 * x;
        outputs[i] = UncertainQuantity(result, uncertainty,
                                     "scalar_" + inputs[i].source());
    }
#endif
}

Eigen::VectorXd SIMDUncertaintyPropagator::compute_numerical_gradient(
    const std::function<double(const Eigen::VectorXd&)>& function,
    const Eigen::VectorXd& point,
    double h) const {
    
    const size_t n = point.size();
    Eigen::VectorXd gradient(n);
    
    // Central difference with error control
    for (size_t i = 0; i < n; ++i) {
        Eigen::VectorXd point_plus = point;
        Eigen::VectorXd point_minus = point;
        
        point_plus[i] += h;
        point_minus[i] -= h;
        
        double f_plus = function(point_plus);
        double f_minus = function(point_minus);
        
        gradient[i] = (f_plus - f_minus) / (2.0 * h);
    }
    
    return gradient;
}

UncertainQuantity SIMDUncertaintyPropagator::propagate(
    const BatchVector& inputs,
    const std::function<double(const Eigen::VectorXd&)>& function,
    const DerivativeFunction& derivatives) const {
    
    if (inputs.empty()) {
        throw std::invalid_argument("Empty input vector");
    }
    
    // Extract values and build input vector
    Eigen::VectorXd values(inputs.size());
    for (size_t i = 0; i < inputs.size(); ++i) {
        values[i] = inputs[i].value();
    }
    
    // Evaluate function
    double result_value = function(values);
    
    // Compute gradient (analytical or numerical)
    Eigen::VectorXd gradient;
    if (derivatives && use_analytical_gradients_) {
        gradient = derivatives(values);
    } else {
        gradient = compute_numerical_gradient(function, values);
    }
    
    // Propagate uncertainty using gradient
    double result_variance = 0.0;
    for (size_t i = 0; i < inputs.size(); ++i) {
        double uncertainty_i = inputs[i].uncertainty();
        result_variance += gradient[i] * gradient[i] * uncertainty_i * uncertainty_i;
    }
    
    // Add cross-correlation terms (simplified)
    for (size_t i = 0; i < inputs.size(); ++i) {
        for (size_t j = i + 1; j < inputs.size(); ++j) {
            // Assume small correlation for simplicity
            double correlation = 0.0;
            result_variance += 2.0 * gradient[i] * gradient[j] * 
                             inputs[i].uncertainty() * inputs[j].uncertainty() * correlation;
        }
    }
    
    double result_uncertainty = std::sqrt(result_variance);
    
    // Combine source IDs
    std::string combined_source = "propagated(";
    for (size_t i = 0; i < inputs.size(); ++i) {
        if (i > 0) combined_source += ",";
        combined_source += inputs[i].source();
    }
    combined_source += ")";
    
    return UncertainQuantity(result_value, result_uncertainty, combined_source);
}

std::vector<UncertainQuantity> SIMDUncertaintyPropagator::propagate_batch(
    const std::vector<BatchVector>& input_batches,
    const std::function<double(const Eigen::VectorXd&)>& function,
    const DerivativeFunction& derivatives) const {
    
    std::vector<UncertainQuantity> results;
    results.reserve(input_batches.size());
    
    // Use TBB for parallel processing
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, input_batches.size()),
        [&](const tbb::blocked_range<size_t>& range) {
            for (size_t i = range.begin(); i != range.end(); ++i) {
                results[i] = propagate(input_batches[i], function, derivatives);
            }
        }
    );
    
    return results;
}

// RealTimeStreamProcessor Implementation

RealTimeStreamProcessor::RealTimeStreamProcessor(StreamCallback output_callback,
                                               ErrorCallback error_callback,
                                               size_t num_workers)
    : output_callback_(output_callback)
    , error_callback_(error_callback)
    , start_time_(std::chrono::steady_clock::now()) {
    
    if (num_workers == 0) {
        num_workers = std::thread::hardware_concurrency();
    }
    
    propagator_ = std::make_unique<SIMDUncertaintyPropagator>();
    
    // Initialize worker threads
    worker_threads_.reserve(num_workers);
}

RealTimeStreamProcessor::~RealTimeStreamProcessor() {
    stop();
}

void RealTimeStreamProcessor::start() {
    running_ = true;
    
    for (size_t i = 0; i < worker_threads_.capacity(); ++i) {
        worker_threads_.emplace_back(&RealTimeStreamProcessor::worker_loop, this);
    }
}

void RealTimeStreamProcessor::stop() {
    running_ = false;
    
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();
}

void RealTimeStreamProcessor::worker_loop() {
    std::vector<UncertainQuantity> batch;
    batch.reserve(MAX_BATCH_SIZE);
    
    while (running_) {
        batch.clear();
        
        // Collect batch
        UncertainQuantity item;
        while (batch.size() < MAX_BATCH_SIZE && try_dequeue(item)) {
            batch.push_back(item);
        }
        
        if (batch.empty()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            continue;
        }
        
        // Process batch
        try {
            for (const auto& input : batch) {
                // Simple processing example: identity function
                output_callback_(input);
                processed_count_++;
            }
        } catch (const std::exception& e) {
            error_callback_(e.what());
            error_count_++;
        }
    }
}

bool RealTimeStreamProcessor::try_enqueue(const UncertainQuantity& item) {
    size_t current_tail = tail_.load(std::memory_order_relaxed);
    size_t next_tail = (current_tail + 1) % QUEUE_SIZE;
    
    if (next_tail == head_.load(std::memory_order_acquire)) {
        return false; // Queue full
    }
    
    queue_[current_tail] = item;
    tail_.store(next_tail, std::memory_order_release);
    return true;
}

bool RealTimeStreamProcessor::try_dequeue(UncertainQuantity& item) {
    size_t current_head = head_.load(std::memory_order_relaxed);
    
    if (current_head == tail_.load(std::memory_order_acquire)) {
        return false; // Queue empty
    }
    
    item = queue_[current_head];
    head_.store((current_head + 1) % QUEUE_SIZE, std::memory_order_release);
    return true;
}

bool RealTimeStreamProcessor::submit(const UncertainQuantity& observation) {
    return try_enqueue(observation);
}

size_t RealTimeStreamProcessor::submit_batch(
    const std::vector<UncertainQuantity>& observations) {
    
    size_t queued_count = 0;
    for (const auto& obs : observations) {
        if (try_enqueue(obs)) {
            ++queued_count;
        } else {
            break; // Queue full
        }
    }
    return queued_count;
}

RealTimeStreamProcessor::StreamMetrics RealTimeStreamProcessor::get_metrics() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    
    uint64_t processed = processed_count_.load();
    uint64_t errors = error_count_.load();
    
    double throughput = (elapsed.count() > 0) ? 
        static_cast<double>(processed) / elapsed.count() : 0.0;
    
    size_t queue_size = (tail_.load() - head_.load() + QUEUE_SIZE) % QUEUE_SIZE;
    double queue_utilization = static_cast<double>(queue_size) / QUEUE_SIZE;
    
    // Estimate average latency (simplified)
    auto avg_latency = std::chrono::microseconds(
        static_cast<int64_t>(queue_utilization * 1000) // μs per item
    );
    
    return StreamMetrics{
        processed,
        errors,
        throughput,
        queue_utilization,
        avg_latency
    };
}

} // namespace uncertainty
} // namespace m17
} // namespace ChiragRathi
