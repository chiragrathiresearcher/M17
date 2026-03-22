/**
 * @file system_integration.hpp
 * @brief M17 Framework System Integration Layer
 * 
 * ChiragRathi M17 Autonomous Celestial Discovery Framework
 * 
 * Provides high-level orchestration and coordination of all M17 subsystems
 * including uncertainty propagation, physics simulation, scientific memory,
 * data fusion, and real-time processing pipelines.
 * 
 * Key Features:
 * - Unified API for framework access
 * - Component lifecycle management
 * - Inter-component communication
 * - Configuration management
 * - Performance monitoring and diagnostics
 * - Error handling and recovery
 * - Plugin architecture support
 * 
 * @author ChiragRathi
 * @version 2.0.0
 * @date 2026
 */

#ifndef CHIRAGRATHI_M17_SYSTEM_INTEGRATION_HPP
#define CHIRAGRATHI_M17_SYSTEM_INTEGRATION_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>
#include <future>

#include "../uncertainty/uncertainty_engine.hpp"
#include "../physics/relativistic_physics.hpp"
#include "../memory/scientific_memory.hpp"
#include "../fusion/data_fusion.hpp"

namespace ChiragRathi {
namespace m17 {
namespace integration {

/**
 * @brief System operational modes
 */
enum class OperationalMode {
    INITIALIZATION,     ///< System starting up
    REAL_TIME,         ///< Real-time processing mode
    BATCH_PROCESSING,  ///< Batch analysis mode
    SIMULATION,        ///< Simulation/modeling mode
    MAINTENANCE,       ///< System maintenance mode
    SHUTDOWN,          ///< System shutting down
    ERROR_RECOVERY     ///< Error recovery mode
};

/**
 * @brief Component health status
 */
enum class ComponentHealth {
    HEALTHY,           ///< Operating normally
    DEGRADED,         ///< Reduced performance but functional
    WARNING,          ///< Issues detected, monitoring required
    CRITICAL,         ///< Critical issues, intervention needed
    FAILED,           ///< Component has failed
    UNKNOWN           ///< Health status unknown
};

/**
 * @brief System performance metrics
 */
struct SystemMetrics {
    // Throughput metrics
    double observations_per_second;
    double classifications_per_second;
    double fusions_per_second;
    double propagations_per_second;
    
    // Latency metrics (microseconds)
    double avg_processing_latency_us;
    double max_processing_latency_us;
    double p95_processing_latency_us;
    
    // Resource utilization
    double cpu_usage_percent;
    double memory_usage_mb;
    double gpu_usage_percent;
    double disk_io_mbps;
    double network_io_mbps;
    
    // Quality metrics
    double accuracy_score;
    double reliability_score;
    double anomaly_detection_rate;
    double false_positive_rate;
    
    // System health
    size_t healthy_components;
    size_t total_components;
    double overall_health_score;
    
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Unified M17 framework configuration
 */
class M17Configuration {
public:
    /**
     * @brief Component-specific configurations
     */
    struct UncertaintyConfig {
        size_t batch_size = 10000;
        size_t num_workers = std::thread::hardware_concurrency();
        bool enable_simd = true;
        bool enable_analytical_gradients = true;
        double numerical_tolerance = 1e-12;
    };
    
    struct PhysicsConfig {
        bool enable_relativity = true;
        bool enable_earth_gravity = true;
        bool enable_third_body = true;
        bool enable_solar_radiation = false;
        bool enable_atmospheric_drag = true;
        int gravity_degree = 20;
        int gravity_order = 20;
        double integration_tolerance = 1e-12;
    };
    
    struct MemoryConfig {
        size_t max_active_cases = 100000;
        size_t max_archived_cases = 1000000;
        double similarity_threshold = 0.7;
        bool enable_real_time_learning = true;
        bool enable_pattern_discovery = true;
        double pruning_interval_hours = 24.0;
    };
    
    struct FusionConfig {
        double temporal_correlation_threshold = 60.0;
        double spatial_correlation_threshold = 1.0;
        double outlier_detection_threshold = 3.0;
        bool enable_robust_estimation = true;
        size_t max_concurrent_fusions = 1000;
        double fusion_timeout_seconds = 30.0;
    };
    
    struct SystemConfig {
        OperationalMode default_mode = OperationalMode::REAL_TIME;
        std::string log_level = "INFO";
        std::string output_directory = "./m17_output";
        bool enable_performance_monitoring = true;
        double monitoring_interval_seconds = 10.0;
        size_t max_concurrent_tasks = 1000;
        bool enable_automatic_recovery = true;
    };
    
private:
    UncertaintyConfig uncertainty_config_;
    PhysicsConfig physics_config_;
    MemoryConfig memory_config_;
    FusionConfig fusion_config_;
    SystemConfig system_config_;
    
    std::unordered_map<std::string, std::string> custom_settings_;
    mutable std::mutex config_mutex_;
    
public:
    // Configuration accessors
    const UncertaintyConfig& uncertainty() const { return uncertainty_config_; }
    const PhysicsConfig& physics() const { return physics_config_; }
    const MemoryConfig& memory() const { return memory_config_; }
    const FusionConfig& fusion() const { return fusion_config_; }
    const SystemConfig& system() const { return system_config_; }
    
    // Configuration mutators
    void set_uncertainty_config(const UncertaintyConfig& config);
    void set_physics_config(const PhysicsConfig& config);
    void set_memory_config(const MemoryConfig& config);
    void set_fusion_config(const FusionConfig& config);
    void set_system_config(const SystemConfig& config);
    
    // Custom settings
    void set_custom_setting(const std::string& key, const std::string& value);
    std::string get_custom_setting(const std::string& key, const std::string& default_value = "") const;
    
    // Serialization
    bool load_from_file(const std::string& config_file);
    bool save_to_file(const std::string& config_file) const;
    std::string to_json() const;
    bool from_json(const std::string& json_str);
    
    // Validation
    std::vector<std::string> validate() const;
    bool is_valid() const { return validate().empty(); }
};

/**
 * @brief Component interface for M17 subsystems
 */
class M17Component {
public:
    virtual ~M17Component() = default;
    
    /**
     * @brief Initialize component with configuration
     * @param config System configuration
     * @return True if initialization successful
     */
    virtual bool initialize(const M17Configuration& config) = 0;
    
    /**
     * @brief Start component operation
     * @return True if start successful
     */
    virtual bool start() = 0;
    
    /**
     * @brief Stop component operation
     * @return True if stop successful
     */
    virtual bool stop() = 0;
    
    /**
     * @brief Get component health status
     * @return Current health status
     */
    virtual ComponentHealth get_health() const = 0;
    
    /**
     * @brief Get component name
     * @return Component identifier string
     */
    virtual std::string get_name() const = 0;
    
    /**
     * @brief Get component version
     * @return Version string
     */
    virtual std::string get_version() const = 0;
    
    /**
     * @brief Reset component to clean state
     * @return True if reset successful
     */
    virtual bool reset() = 0;
    
    /**
     * @brief Get component-specific metrics
     * @return Map of metric name to value
     */
    virtual std::unordered_map<std::string, double> get_metrics() const = 0;
};

/**
 * @brief M17 Framework orchestration engine
 * 
 * Central coordinator for all M17 framework components providing
 * unified API access, lifecycle management, and system monitoring.
 */
class M17FrameworkOrchestrator {
public:
    using ErrorCallback = std::function<void(const std::string&, const std::exception&)>;
    using MetricsCallback = std::function<void(const SystemMetrics&)>;
    using AlertCallback = std::function<void(const std::string&, const std::string&)>;
    
private:
    // Configuration and state
    M17Configuration config_;
    OperationalMode current_mode_;
    std::atomic<bool> running_{false};
    
    // Core components
    std::unique_ptr<uncertainty::SIMDUncertaintyPropagator> uncertainty_engine_;
    std::unique_ptr<physics::AdvancedPhysicsEngine> physics_engine_;
    std::unique_ptr<memory::ScientificMemorySystem> memory_system_;
    std::unique_ptr<fusion::RealTimeFusionCoordinator> fusion_coordinator_;
    std::unique_ptr<uncertainty::RealTimeStreamProcessor> stream_processor_;
    
    // Component registry
    std::unordered_map<std::string, std::shared_ptr<M17Component>> components_;
    
    // System monitoring
    std::thread monitoring_thread_;
    std::atomic<bool> monitoring_active_{false};
    SystemMetrics current_metrics_;
    mutable std::mutex metrics_mutex_;
    
    // Callbacks
    ErrorCallback error_callback_;
    MetricsCallback metrics_callback_;
    AlertCallback alert_callback_;
    
    // Performance tracking
    std::chrono::system_clock::time_point startup_time_;
    std::atomic<size_t> total_observations_processed_{0};
    std::atomic<size_t> total_anomalies_detected_{0};
    std::atomic<size_t> total_fusions_completed_{0};
    
public:
    /**
     * @brief Construct M17 framework orchestrator
     * @param config Framework configuration
     */
    explicit M17FrameworkOrchestrator(const M17Configuration& config = M17Configuration{});
    
    /**
     * @brief Destructor - ensures clean shutdown
     */
    ~M17FrameworkOrchestrator();
    
    /**
     * @brief Initialize all framework components
     * @return True if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Start framework operation
     * @param mode Operational mode to start in
     * @return True if start successful
     */
    bool start(OperationalMode mode = OperationalMode::REAL_TIME);
    
    /**
     * @brief Stop framework operation
     * @param graceful_shutdown_timeout_ms Timeout for graceful shutdown
     * @return True if stop successful
     */
    bool stop(uint32_t graceful_shutdown_timeout_ms = 30000);
    
    /**
     * @brief Check if framework is running
     * @return True if framework is operational
     */
    bool is_running() const { return running_.load(); }
    
    /**
     * @brief Get current operational mode
     * @return Current mode
     */
    OperationalMode get_mode() const { return current_mode_; }
    
    /**
     * @brief Switch to new operational mode
     * @param new_mode Target operational mode
     * @return True if mode switch successful
     */
    bool set_mode(OperationalMode new_mode);
    
    // High-level processing interface
    
    /**
     * @brief Process single astronomical observation
     * @param observation Input observation data
     * @return Processing result with classification and analysis
     */
    struct ProcessingResult {
        std::shared_ptr<memory::ScientificCase> classified_case;
        std::optional<fusion::BayesianDataFusion::FusionResult> fusion_result;
        std::vector<uncertainty::UncertainQuantity> propagated_uncertainties;
        std::optional<physics::RelativisticOrbitalState> orbital_analysis;
        double processing_time_ms;
        bool success;
        std::string error_message;
    };
    
    ProcessingResult process_observation(const fusion::Observation& observation);
    
    /**
     * @brief Process batch of observations
     * @param observations Vector of observations to process
     * @return Vector of processing results
     */
    std::vector<ProcessingResult> process_observations(const std::vector<fusion::Observation>& observations);
    
    /**
     * @brief Submit observation for asynchronous processing
     * @param observation Input observation
     * @return Future for processing result
     */
    std::future<ProcessingResult> submit_observation_async(const fusion::Observation& observation);
    
    // Component access interface
    
    /**
     * @brief Get uncertainty propagation engine
     * @return Reference to uncertainty engine
     */
    uncertainty::SIMDUncertaintyPropagator& get_uncertainty_engine();
    
    /**
     * @brief Get physics simulation engine
     * @return Reference to physics engine
     */
    physics::AdvancedPhysicsEngine& get_physics_engine();
    
    /**
     * @brief Get scientific memory system
     * @return Reference to memory system
     */
    memory::ScientificMemorySystem& get_memory_system();
    
    /**
     * @brief Get data fusion coordinator
     * @return Reference to fusion coordinator
     */
    fusion::RealTimeFusionCoordinator& get_fusion_coordinator();
    
    // System monitoring and diagnostics
    
    /**
     * @brief Get current system metrics
     * @return Current performance and health metrics
     */
    SystemMetrics get_system_metrics() const;
    
    /**
     * @brief Get component health report
     * @return Map of component names to health status
     */
    std::unordered_map<std::string, ComponentHealth> get_component_health() const;
    
    /**
     * @brief Get system uptime
     * @return Uptime in seconds
     */
    double get_uptime_seconds() const;
    
    /**
     * @brief Get processing statistics
     */
    struct ProcessingStatistics {
        size_t total_observations_processed;
        size_t total_anomalies_detected;
        size_t total_fusions_completed;
        double average_processing_rate;
        double peak_processing_rate;
        double anomaly_detection_rate;
        std::chrono::system_clock::time_point since;
    };
    
    ProcessingStatistics get_processing_statistics() const;
    
    // Configuration management
    
    /**
     * @brief Get current configuration
     * @return Framework configuration
     */
    const M17Configuration& get_configuration() const { return config_; }
    
    /**
     * @brief Update framework configuration
     * @param new_config New configuration
     * @param apply_immediately Apply changes immediately
     * @return True if update successful
     */
    bool update_configuration(const M17Configuration& new_config, bool apply_immediately = false);
    
    /**
     * @brief Reload configuration from file
     * @param config_file Configuration file path
     * @return True if reload successful
     */
    bool reload_configuration(const std::string& config_file);
    
    // Callback management
    
    /**
     * @brief Set error handling callback
     * @param callback Error callback function
     */
    void set_error_callback(ErrorCallback callback) { error_callback_ = callback; }
    
    /**
     * @brief Set metrics monitoring callback
     * @param callback Metrics callback function
     */
    void set_metrics_callback(MetricsCallback callback) { metrics_callback_ = callback; }
    
    /**
     * @brief Set system alert callback
     * @param callback Alert callback function
     */
    void set_alert_callback(AlertCallback callback) { alert_callback_ = callback; }
    
    // Plugin architecture
    
    /**
     * @brief Register custom component
     * @param name Component name
     * @param component Component implementation
     * @return True if registration successful
     */
    bool register_component(const std::string& name, std::shared_ptr<M17Component> component);
    
    /**
     * @brief Unregister custom component
     * @param name Component name
     * @return True if unregistration successful
     */
    bool unregister_component(const std::string& name);
    
    /**
     * @brief Get registered component
     * @param name Component name
     * @return Component pointer or nullptr if not found
     */
    std::shared_ptr<M17Component> get_component(const std::string& name);
    
    // Utility functions
    
    /**
     * @brief Validate system health and configuration
     * @return Vector of validation issues (empty if all OK)
     */
    std::vector<std::string> validate_system() const;
    
    /**
     * @brief Perform system self-test
     * @return True if all self-tests pass
     */
    bool run_self_test();
    
    /**
     * @brief Generate comprehensive system report
     * @param include_diagnostics Include detailed diagnostics
     * @return System status report
     */
    std::string generate_system_report(bool include_diagnostics = false) const;
    
    /**
     * @brief Export system logs and metrics
     * @param output_directory Directory for exported data
     * @param include_raw_data Include raw observation data
     * @return True if export successful
     */
    bool export_system_data(const std::string& output_directory, bool include_raw_data = false) const;
    
private:
    // Internal methods
    bool initialize_components();
    bool start_components();
    bool stop_components();
    void monitoring_loop();
    void update_system_metrics();
    void handle_component_failure(const std::string& component_name, const std::exception& error);
    ProcessingResult process_observation_internal(const fusion::Observation& observation);
    
    // Utility methods
    ComponentHealth compute_overall_health() const;
    void emit_alert(const std::string& level, const std::string& message);
    void emit_error(const std::string& context, const std::exception& error);
};

/**
 * @brief Convenience wrapper for simplified M17 access
 * 
 * Provides a simplified interface for common M17 operations,
 * hiding the complexity of the full orchestrator for basic use cases.
 */
class M17Framework {
private:
    std::unique_ptr<M17FrameworkOrchestrator> orchestrator_;
    bool auto_initialized_;
    
public:
    /**
     * @brief Construct simplified M17 framework
     * @param auto_initialize Automatically initialize with defaults
     */
    explicit M17Framework(bool auto_initialize = true);
    
    /**
     * @brief Destructor
     */
    ~M17Framework();
    
    /**
     * @brief Initialize framework with configuration
     * @param config_file Configuration file path (optional)
     * @return True if initialization successful
     */
    bool initialize(const std::string& config_file = "");
    
    /**
     * @brief Start framework
     * @return True if start successful
     */
    bool start();
    
    /**
     * @brief Stop framework
     */
    void stop();
    
    /**
     * @brief Process single observation (simplified interface)
     * @param ra Right ascension [degrees]
     * @param dec Declination [degrees]
     * @param magnitude Apparent magnitude
     * @param ra_uncertainty RA uncertainty [arcsec]
     * @param dec_uncertainty Dec uncertainty [arcsec]
     * @param mag_uncertainty Magnitude uncertainty
     * @return Simplified processing result
     */
    struct SimpleResult {
        bool is_anomaly;
        std::string classification;
        double confidence;
        double processing_time_ms;
        std::string notes;
    };
    
    SimpleResult analyze_observation(double ra, double dec, double magnitude,
                                   double ra_uncertainty, double dec_uncertainty, double mag_uncertainty);
    
    /**
     * @brief Get simple framework status
     * @return Human-readable status string
     */
    std::string get_status() const;
    
    /**
     * @brief Check if framework is ready for processing
     * @return True if ready
     */
    bool is_ready() const;
    
    /**
     * @brief Get access to full orchestrator
     * @return Reference to orchestrator
     */
    M17FrameworkOrchestrator& get_orchestrator();
};

// Utility functions

/**
 * @brief Convert operational mode to string
 * @param mode Operational mode
 * @return String representation
 */
std::string operational_mode_to_string(OperationalMode mode);

/**
 * @brief Convert component health to string
 * @param health Component health status
 * @return String representation
 */
std::string component_health_to_string(ComponentHealth health);

/**
 * @brief Load M17 configuration from file
 * @param config_file Configuration file path
 * @return Loaded configuration
 */
M17Configuration load_configuration(const std::string& config_file);

/**
 * @brief Create default M17 configuration
 * @return Default configuration suitable for most use cases
 */
M17Configuration create_default_configuration();

} // namespace integration
} // namespace m17
} // namespace ChiragRathi

#endif // CHIRAGRATHI_M17_SYSTEM_INTEGRATION_HPP
