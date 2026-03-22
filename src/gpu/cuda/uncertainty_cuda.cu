/**
 * @file uncertainty_cuda.cu
 * @brief CUDA Acceleration Kernels for Uncertainty Propagation
 * 
 * ChiragRathi M17 Autonomous Celestial Discovery Framework
 * 
 * GPU acceleration for uncertainty propagation operations with
 * support for thousands of concurrent uncertainty calculations.
 * 
 * Performance targets:
 * - >1M uncertainty propagations per second
 * - <10μs latency for batch operations
 * - Memory bandwidth utilization >80%
 * 
 * @author ChiragRathi
 * @version 2.0.0
 * @date 2026
 */

#include <cuda_runtime.h>
#include <curand.h>
#include <cublas_v2.h>
#include <cusolver_common.h>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/transform.h>
#include <thrust/reduce.h>
#include <cmath>
#include <memory>

#include "uncertainty_cuda.hpp"

namespace ChiragRathi {
namespace m17 {
namespace cuda {

// CUDA kernel constants
constexpr int BLOCK_SIZE = 256;
constexpr int WARP_SIZE = 32;
constexpr int MAX_SHARED_MEM = 48 * 1024; // 48KB per block

/**
 * @brief GPU-accelerated uncertain quantity structure
 */
struct __align__(16) UncertainQuantityGPU {
    double value;
    double uncertainty;
    double correlation[8];  // Support for up to 8 correlated quantities
    uint32_t source_hash;
    
    __device__ __host__
    UncertainQuantityGPU(double v = 0.0, double u = 0.0, uint32_t src = 0)
        : value(v), uncertainty(u), source_hash(src) {
        #pragma unroll
        for (int i = 0; i < 8; ++i) {
            correlation[i] = 0.0;
        }
    }
};

/**
 * @brief CUDA kernel for parallel uncertainty addition
 */
__global__ void uncertainty_add_kernel(
    const UncertainQuantityGPU* __restrict__ a,
    const UncertainQuantityGPU* __restrict__ b,
    UncertainQuantityGPU* __restrict__ result,
    const int n) {
    
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = gridDim.x * blockDim.x;
    
    for (int i = idx; i < n; i += stride) {
        // Value addition
        result[i].value = a[i].value + b[i].value;
        
        // Uncertainty propagation: σ²(a+b) = σ²(a) + σ²(b) + 2ρσ(a)σ(b)
        double sigma_a = a[i].uncertainty;
        double sigma_b = b[i].uncertainty;
        
        // Simplified correlation (first element)
        double correlation = a[i].correlation[0] * b[i].correlation[0];
        
        double variance = sigma_a * sigma_a + sigma_b * sigma_b + 
                         2.0 * correlation * sigma_a * sigma_b;
        result[i].uncertainty = sqrt(fmax(variance, 0.0));
        
        // Combine source hashes
        result[i].source_hash = a[i].source_hash ^ b[i].source_hash;
    }
}

/**
 * @brief CUDA kernel for parallel uncertainty multiplication
 */
__global__ void uncertainty_multiply_kernel(
    const UncertainQuantityGPU* __restrict__ a,
    const UncertainQuantityGPU* __restrict__ b,
    UncertainQuantityGPU* __restrict__ result,
    const int n) {
    
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = gridDim.x * blockDim.x;
    
    for (int i = idx; i < n; i += stride) {
        double val_a = a[i].value;
        double val_b = b[i].value;
        
        // Value multiplication
        result[i].value = val_a * val_b;
        
        // Skip if either value is near zero
        if (fabs(val_a) < 1e-15 || fabs(val_b) < 1e-15) {
            result[i].uncertainty = 0.0;
            result[i].source_hash = a[i].source_hash ^ b[i].source_hash;
            continue;
        }
        
        // Relative uncertainty propagation
        double rel_unc_a = a[i].uncertainty / fabs(val_a);
        double rel_unc_b = b[i].uncertainty / fabs(val_b);
        double correlation = a[i].correlation[0] * b[i].correlation[0];
        
        double rel_variance = rel_unc_a * rel_unc_a + rel_unc_b * rel_unc_b + 
                             2.0 * correlation * rel_unc_a * rel_unc_b;
        
        result[i].uncertainty = fabs(result[i].value) * sqrt(fmax(rel_variance, 0.0));
        result[i].source_hash = a[i].source_hash ^ b[i].source_hash;
    }
}

/**
 * @brief CUDA kernel for power function with uncertainty propagation
 */
__global__ void uncertainty_pow_kernel(
    const UncertainQuantityGPU* __restrict__ base,
    const double exponent,
    UncertainQuantityGPU* __restrict__ result,
    const int n) {
    
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = gridDim.x * blockDim.x;
    
    for (int i = idx; i < n; i += stride) {
        double x = base[i].value;
        double sigma_x = base[i].uncertainty;
        
        // Power operation: y = x^n
        double y = pow(x, exponent);
        result[i].value = y;
        
        // Derivative: dy/dx = n * x^(n-1)
        if (fabs(x) > 1e-15) {
            double derivative = exponent * pow(x, exponent - 1.0);
            result[i].uncertainty = fabs(derivative) * sigma_x;
        } else {
            result[i].uncertainty = 0.0;
        }
        
        result[i].source_hash = base[i].source_hash;
    }
}

/**
 * @brief CUDA kernel for batch function evaluation with uncertainty propagation
 */
__global__ void uncertainty_function_kernel(
    const UncertainQuantityGPU* __restrict__ inputs,
    UncertainQuantityGPU* __restrict__ outputs,
    const double* __restrict__ gradients,
    const int n_inputs,
    const int n_outputs,
    const int batch_size) {
    
    int batch_idx = blockIdx.x;
    int input_idx = threadIdx.x;
    
    if (batch_idx >= batch_size) return;
    
    __shared__ double shared_values[BLOCK_SIZE];
    __shared__ double shared_gradients[BLOCK_SIZE];
    
    // Load input values into shared memory
    if (input_idx < n_inputs) {
        shared_values[input_idx] = inputs[batch_idx * n_inputs + input_idx].value;
        shared_gradients[input_idx] = gradients[batch_idx * n_inputs + input_idx];
    }
    
    __syncthreads();
    
    // Compute uncertainty propagation for this batch
    if (input_idx == 0) {  // Single thread per batch computes result
        double result_variance = 0.0;
        
        for (int i = 0; i < n_inputs; ++i) {
            double gradient = shared_gradients[i];
            double uncertainty = inputs[batch_idx * n_inputs + i].uncertainty;
            result_variance += gradient * gradient * uncertainty * uncertainty;
            
            // Add cross-correlation terms (simplified)
            for (int j = i + 1; j < n_inputs; ++j) {
                double corr = inputs[batch_idx * n_inputs + i].correlation[0] *
                             inputs[batch_idx * n_inputs + j].correlation[0];
                result_variance += 2.0 * gradient * shared_gradients[j] * 
                                  uncertainty * inputs[batch_idx * n_inputs + j].uncertainty * corr;
            }
        }
        
        outputs[batch_idx].uncertainty = sqrt(fmax(result_variance, 0.0));
    }
}

/**
 * @brief CUDA kernel for Monte Carlo uncertainty estimation
 */
__global__ void monte_carlo_uncertainty_kernel(
    const UncertainQuantityGPU* __restrict__ inputs,
    double* __restrict__ samples,
    UncertainQuantityGPU* __restrict__ result,
    curandState* __restrict__ states,
    const int n_inputs,
    const int n_samples) {
    
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = gridDim.x * blockDim.x;
    
    __shared__ double sample_results[BLOCK_SIZE];
    
    for (int sample = idx; sample < n_samples; sample += stride) {
        curandState local_state = states[idx];
        
        // Generate random sample for each input
        double function_result = 0.0;
        for (int i = 0; i < n_inputs; ++i) {
            double mean = inputs[i].value;
            double std_dev = inputs[i].uncertainty;
            double sample_value = mean + std_dev * curand_normal_double(&local_state);
            
            // Simple quadratic function for demonstration: f(x) = x²
            function_result += sample_value * sample_value;
        }
        
        sample_results[threadIdx.x] = function_result;
        states[idx] = local_state;  // Save state
        
        __syncthreads();
        
        // Reduce to compute mean and variance (simplified)
        if (threadIdx.x == 0) {
            double sum = 0.0;
            double sum_sq = 0.0;
            
            for (int i = 0; i < blockDim.x && i < n_samples; ++i) {
                sum += sample_results[i];
                sum_sq += sample_results[i] * sample_results[i];
            }
            
            double mean = sum / blockDim.x;
            double variance = sum_sq / blockDim.x - mean * mean;
            
            atomicAdd(&result->value, mean);
            atomicAdd(&result->uncertainty, sqrt(variance));
        }
    }
}

/**
 * @brief Initialize cuRAND states for Monte Carlo sampling
 */
__global__ void setup_curand_states(curandState* __restrict__ states, 
                                   const unsigned long seed,
                                   const int n_states) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n_states) {
        curand_init(seed, idx, 0, &states[idx]);
    }
}

/**
 * @brief CUDA-accelerated uncertainty propagator class
 */
class CUDAUncertaintyPropagator {
private:
    cublasHandle_t cublas_handle_;
    cusolverDnHandle_t cusolver_handle_;
    
    thrust::device_vector<UncertainQuantityGPU> device_inputs_;
    thrust::device_vector<UncertainQuantityGPU> device_outputs_;
    thrust::device_vector<curandState> rand_states_;
    
    int max_batch_size_;
    
public:
    explicit CUDAUncertaintyPropagator(int max_batch_size = 100000) 
        : max_batch_size_(max_batch_size) {
        
        // Initialize CUDA libraries
        cublasCreate(&cublas_handle_);
        cusolverDnCreate(&cusolver_handle_);
        
        // Pre-allocate device memory
        device_inputs_.reserve(max_batch_size_);
        device_outputs_.reserve(max_batch_size_);
        rand_states_.resize(max_batch_size_);
        
        // Initialize random states
        int grid_size = (max_batch_size_ + BLOCK_SIZE - 1) / BLOCK_SIZE;
        setup_curand_states<<<grid_size, BLOCK_SIZE>>>(
            thrust::raw_pointer_cast(rand_states_.data()),
            time(nullptr),
            max_batch_size_
        );
        cudaDeviceSynchronize();
    }
    
    ~CUDAUncertaintyPropagator() {
        cublasDestroy(cublas_handle_);
        cusolverDnDestroy(cusolver_handle_);
    }
    
    /**
     * @brief Add two batches of uncertain quantities
     */
    void add_batch(const std::vector<UncertainQuantityGPU>& a,
                   const std::vector<UncertainQuantityGPU>& b,
                   std::vector<UncertainQuantityGPU>& result) {
        
        int n = a.size();
        if (b.size() != n || n > max_batch_size_) {
            throw std::runtime_error("Invalid batch size");
        }
        
        // Copy data to device
        thrust::device_vector<UncertainQuantityGPU> dev_a = a;
        thrust::device_vector<UncertainQuantityGPU> dev_b = b;
        thrust::device_vector<UncertainQuantityGPU> dev_result(n);
        
        // Launch kernel
        int grid_size = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
        uncertainty_add_kernel<<<grid_size, BLOCK_SIZE>>>(
            thrust::raw_pointer_cast(dev_a.data()),
            thrust::raw_pointer_cast(dev_b.data()),
            thrust::raw_pointer_cast(dev_result.data()),
            n
        );
        
        // Check for errors
        cudaError_t err = cudaGetLastError();
        if (err != cudaSuccess) {
            throw std::runtime_error("CUDA kernel error: " + std::string(cudaGetErrorString(err)));
        }
        
        cudaDeviceSynchronize();
        
        // Copy result back to host
        result.resize(n);
        thrust::copy(dev_result.begin(), dev_result.end(), result.begin());
    }
    
    /**
     * @brief Multiply two batches of uncertain quantities
     */
    void multiply_batch(const std::vector<UncertainQuantityGPU>& a,
                       const std::vector<UncertainQuantityGPU>& b,
                       std::vector<UncertainQuantityGPU>& result) {
        
        int n = a.size();
        if (b.size() != n || n > max_batch_size_) {
            throw std::runtime_error("Invalid batch size");
        }
        
        thrust::device_vector<UncertainQuantityGPU> dev_a = a;
        thrust::device_vector<UncertainQuantityGPU> dev_b = b;
        thrust::device_vector<UncertainQuantityGPU> dev_result(n);
        
        int grid_size = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
        uncertainty_multiply_kernel<<<grid_size, BLOCK_SIZE>>>(
            thrust::raw_pointer_cast(dev_a.data()),
            thrust::raw_pointer_cast(dev_b.data()),
            thrust::raw_pointer_cast(dev_result.data()),
            n
        );
        
        cudaDeviceSynchronize();
        
        result.resize(n);
        thrust::copy(dev_result.begin(), dev_result.end(), result.begin());
    }
    
    /**
     * @brief Compute power of uncertain quantities
     */
    void pow_batch(const std::vector<UncertainQuantityGPU>& base,
                   double exponent,
                   std::vector<UncertainQuantityGPU>& result) {
        
        int n = base.size();
        if (n > max_batch_size_) {
            throw std::runtime_error("Batch too large");
        }
        
        thrust::device_vector<UncertainQuantityGPU> dev_base = base;
        thrust::device_vector<UncertainQuantityGPU> dev_result(n);
        
        int grid_size = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
        uncertainty_pow_kernel<<<grid_size, BLOCK_SIZE>>>(
            thrust::raw_pointer_cast(dev_base.data()),
            exponent,
            thrust::raw_pointer_cast(dev_result.data()),
            n
        );
        
        cudaDeviceSynchronize();
        
        result.resize(n);
        thrust::copy(dev_result.begin(), dev_result.end(), result.begin());
    }
    
    /**
     * @brief Monte Carlo uncertainty estimation
     */
    UncertainQuantityGPU monte_carlo_propagate(
        const std::vector<UncertainQuantityGPU>& inputs,
        int n_samples = 10000) {
        
        int n_inputs = inputs.size();
        if (n_samples > max_batch_size_) {
            n_samples = max_batch_size_;
        }
        
        thrust::device_vector<UncertainQuantityGPU> dev_inputs = inputs;
        thrust::device_vector<double> dev_samples(n_samples);
        
        UncertainQuantityGPU result(0.0, 0.0);
        thrust::device_vector<UncertainQuantityGPU> dev_result(1, result);
        
        int grid_size = (n_samples + BLOCK_SIZE - 1) / BLOCK_SIZE;
        monte_carlo_uncertainty_kernel<<<grid_size, BLOCK_SIZE>>>(
            thrust::raw_pointer_cast(dev_inputs.data()),
            thrust::raw_pointer_cast(dev_samples.data()),
            thrust::raw_pointer_cast(dev_result.data()),
            thrust::raw_pointer_cast(rand_states_.data()),
            n_inputs,
            n_samples
        );
        
        cudaDeviceSynchronize();
        
        thrust::copy(dev_result.begin(), dev_result.end(), &result);
        
        // Normalize by number of samples
        result.value /= n_samples;
        result.uncertainty /= sqrt(static_cast<double>(n_samples));
        
        return result;
    }
    
    /**
     * @brief Get GPU memory usage statistics
     */
    struct MemoryInfo {
        size_t free_bytes;
        size_t total_bytes;
        size_t used_bytes;
    };
    
    MemoryInfo get_memory_info() const {
        size_t free, total;
        cudaMemGetInfo(&free, &total);
        return {free, total, total - free};
    }
    
    /**
     * @brief Get device properties
     */
    cudaDeviceProp get_device_properties() const {
        int device_id;
        cudaGetDevice(&device_id);
        
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, device_id);
        return prop;
    }
};

/**
 * @brief CUDA stream-based asynchronous processing
 */
class CUDAStreamProcessor {
private:
    static constexpr int NUM_STREAMS = 4;
    cudaStream_t streams_[NUM_STREAMS];
    std::unique_ptr<CUDAUncertaintyPropagator> propagator_;
    
public:
    CUDAStreamProcessor() {
        // Create CUDA streams
        for (int i = 0; i < NUM_STREAMS; ++i) {
            cudaStreamCreate(&streams_[i]);
        }
        
        propagator_ = std::make_unique<CUDAUncertaintyPropagator>();
    }
    
    ~CUDAStreamProcessor() {
        for (int i = 0; i < NUM_STREAMS; ++i) {
            cudaStreamDestroy(streams_[i]);
        }
    }
    
    /**
     * @brief Asynchronous batch processing
     */
    void process_async(const std::vector<UncertainQuantityGPU>& inputs,
                      std::vector<UncertainQuantityGPU>& outputs,
                      int stream_id = 0) {
        
        if (stream_id >= NUM_STREAMS) {
            stream_id = 0;
        }
        
        // Implementation would use streams for async processing
        // This is a simplified version
        propagator_->pow_batch(inputs, 2.0, outputs);
    }
};

} // namespace cuda
} // namespace m17
} // namespace ChiragRathi

// C interface for other languages
extern "C" {

/**
 * @brief C interface for CUDA uncertainty propagation
 */
void* cuda_propagator_create(int max_batch_size) {
    try {
        return new ChiragRathi::m17::cuda::CUDAUncertaintyPropagator(max_batch_size);
    } catch (...) {
        return nullptr;
    }
}

void cuda_propagator_destroy(void* propagator) {
    if (propagator) {
        delete static_cast<ChiragRathi::m17::cuda::CUDAUncertaintyPropagator*>(propagator);
    }
}

int cuda_add_batch(void* propagator,
                   const double* a_values, const double* a_uncertainties,
                   const double* b_values, const double* b_uncertainties,
                   double* result_values, double* result_uncertainties,
                   int batch_size) {
    
    if (!propagator || !a_values || !a_uncertainties || 
        !b_values || !b_uncertainties || !result_values || !result_uncertainties) {
        return -1;
    }
    
    try {
        auto* prop = static_cast<ChiragRathi::m17::cuda::CUDAUncertaintyPropagator*>(propagator);
        
        std::vector<ChiragRathi::m17::cuda::UncertainQuantityGPU> a(batch_size);
        std::vector<ChiragRathi::m17::cuda::UncertainQuantityGPU> b(batch_size);
        std::vector<ChiragRathi::m17::cuda::UncertainQuantityGPU> result;
        
        for (int i = 0; i < batch_size; ++i) {
            a[i] = ChiragRathi::m17::cuda::UncertainQuantityGPU(a_values[i], a_uncertainties[i]);
            b[i] = ChiragRathi::m17::cuda::UncertainQuantityGPU(b_values[i], b_uncertainties[i]);
        }
        
        prop->add_batch(a, b, result);
        
        for (int i = 0; i < batch_size; ++i) {
            result_values[i] = result[i].value;
            result_uncertainties[i] = result[i].uncertainty;
        }
        
        return 0;
    } catch (...) {
        return -1;
    }
}

} // extern "C"
