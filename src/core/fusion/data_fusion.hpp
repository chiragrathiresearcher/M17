/**
 * @file data_fusion.hpp
 * @brief Multi-Source Data Fusion and Trust Assessment System
 * 
 * ChiragRathi M17 Autonomous Celestial Discovery Framework
 * 
 * Implements intelligent fusion of heterogeneous astronomical data sources
 * with dynamic trust assessment, conflict resolution, and quality control.
 * 
 * Core Features:
 * - Bayesian sensor fusion with uncertainty propagation
 * - Dynamic trust assessment based on historical performance
 * - Multi-temporal data alignment and interpolation
 * - Outlier detection and robust estimation
 * - Source reliability scoring with decay models
 * - Conflict resolution using consensus algorithms
 * 
 * @author ChiragRathi
 * @version 2.0.0
 * @date 2026
 */

#ifndef CHIRAGRATHI_M17_DATA_FUSION_HPP
#define CHIRAGRATHI_M17_DATA_FUSION_HPP

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <chrono>
#include <functional>
#include <queue>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include "../uncertainty/uncertainty_engine.hpp"

namespace ChiragRathi {
namespace m17 {
namespace fusion {

/**
 * @brief Data source types in astronomical observations
 */
enum class SourceType {
    GROUND_TELESCOPE = 0,       ///< Ground-based optical telescope
    SPACE_TELESCOPE = 1,        ///< Space-based telescope (HST, JWST, etc.)
    RADAR_SYSTEM = 2,           ///< Radar tracking system
    SATELLITE_TRACKER = 3,      ///< Dedicated satellite tracking station
    AMATEUR_OBSERVATION = 4,    ///< Amateur astronomer observation
    SURVEY_CATALOG = 5,         ///< Large survey catalog (Gaia, etc.)
    SIMULATION = 6,             ///< Numerical simulation/model
    ARCHIVE_DATA = 7,           ///< Historical archived observations
    ION_CHAMBER = 8,           ///< Radiation monitoring
    MAGNETOMETER = 9,          ///< Magnetic field measurement
    SPECTROMETER = 10,         ///< Spectroscopic observation
    PHOTOMETER = 11,           ///< Photometric measurement
    ASTROMETRIC = 12,          ///< Precise position measurement
    EXTERNAL_API = 13,         ///< External data service
    CROWDSOURCED = 14,         ///< Crowd-sourced observations
    UNKNOWN_SOURCE = 15        ///< Source type not determined
};

/**
 * @brief Observation quality metrics
 */
enum class QualityFlag {
    EXCELLENT = 0,              ///< High-quality, well-calibrated data
    GOOD = 1,                  ///< Good quality with minor issues
    ACCEPTABLE = 2,            ///< Acceptable for non-critical applications
    QUESTIONABLE = 3,          ///< Quality concerns, use with caution
    POOR = 4,                  ///< Poor quality, significant issues
    CORRUPTED = 5,             ///< Data corruption detected
    CALIBRATION_ISSUE = 6,     ///< Calibration problems identified
    ATMOSPHERIC_INTERFERENCE = 7, ///< Weather/seeing issues
    INSTRUMENT_MALFUNCTION = 8, ///< Instrument problems
    TIMING_ERROR = 9,          ///< Timestamp accuracy issues
    REJECTED = 10              ///< Data rejected by quality control
};

/**
 * @brief Multi-dimensional observation with metadata
 * 
 * Comprehensive observation structure containing measured values,
 * uncertainties, source information, and quality assessment.
 */
class Observation {
public:
    using TimePoint = std::chrono::system_clock::time_point;
    using UncertainQuantity = uncertainty::UncertainQuantity;
    
private:
    std::string observation_id_;           ///< Unique observation identifier
    std::string source_id_;                ///< Data source identifier
    SourceType source_type_;               ///< Type of data source
    
    std::vector<UncertainQuantity> values_; ///< Measured values with uncertainties
    std::vector<std::string> value_names_;   ///< Names/types of measured quantities
    
    TimePoint timestamp_;                  ///< Observation timestamp
    double exposure_time_;                 ///< Exposure/integration time [s]
    
    QualityFlag quality_;                  ///< Overall quality assessment
    double confidence_score_;              ///< Source confidence [0,1]
    double reliability_score_;             ///< Historical reliability [0,1]
    
    // Metadata
    std::unordered_map<std::string, double> metadata_;
    std::unordered_map<std::string, std::string> annotations_;
    
    // Correlation information
    Eigen::MatrixXd correlation_matrix_;   ///< Inter-value correlations
    std::vector<std::string> correlated_obs_; ///< Related observations
    
public:
    /**
     * @brief Construct observation
     * @param id Unique observation identifier
     * @param source_id Data source identifier  
     * @param source_type Type of data source
     * @param values Measured values with uncertainties
     * @param value_names Names of measured quantities
     * @param timestamp Observation timestamp
     */
    Observation(const std::string& id,
               const std::string& source_id,
               SourceType source_type,
               const std::vector<UncertainQuantity>& values,
               const std::vector<std::string>& value_names,
               const TimePoint& timestamp);
    
    // Accessors
    const std::string& observation_id() const { return observation_id_; }
    const std::string& source_id() const { return source_id_; }
    SourceType source_type() const { return source_type_; }
    const std::vector<UncertainQuantity>& values() const { return values_; }
    const std::vector<std::string>& value_names() const { return value_names_; }
    const TimePoint& timestamp() const { return timestamp_; }
    double exposure_time() const { return exposure_time_; }
    QualityFlag quality() const { return quality_; }
    double confidence_score() const { return confidence_score_; }
    double reliability_score() const { return reliability_score_; }
    
    // Mutators
    void set_quality(QualityFlag quality) { quality_ = quality; }
    void set_confidence_score(double score) { confidence_score_ = score; }
    void set_reliability_score(double score) { reliability_score_ = score; }
    void set_exposure_time(double time) { exposure_time_ = time; }
    
    // Metadata management
    void set_metadata(const std::string& key, double value) { metadata_[key] = value; }
    double get_metadata(const std::string& key, double default_val = 0.0) const;
    void set_annotation(const std::string& key, const std::string& value) { annotations_[key] = value; }
    std::string get_annotation(const std::string& key, const std::string& default_val = "") const;
    
    // Correlation management
    void set_correlation_matrix(const Eigen::MatrixXd& corr) { correlation_matrix_ = corr; }
    const Eigen::MatrixXd& correlation_matrix() const { return correlation_matrix_; }
    void add_correlated_observation(const std::string& obs_id) { correlated_obs_.push_back(obs_id); }
    
    // Temporal operations
    double age_seconds() const;
    bool is_contemporary_with(const Observation& other, double tolerance_seconds = 60.0) const;
    
    // Quality assessment
    bool passes_quality_threshold(QualityFlag min_quality = QualityFlag::ACCEPTABLE) const;
    double combined_reliability() const;  // Combines confidence and reliability
    
    // Serialization
    std::string to_json() const;
    static Observation from_json(const std::string& json_data);
};

/**
 * @brief Source reliability tracker with decay models
 * 
 * Tracks historical performance of data sources and provides
 * dynamic reliability scoring with temporal decay.
 */
class SourceReliabilityTracker {
public:
    /**
     * @brief Reliability metrics for a data source
     */
    struct SourceMetrics {
        size_t total_observations;           ///< Total observations received
        size_t accepted_observations;        ///< Observations passing quality control
        size_t rejected_observations;        ///< Observations rejected
        
        double avg_accuracy;                 ///< Average accuracy when validated
        double avg_precision;                ///< Average precision (1/uncertainty)
        double avg_latency_hours;            ///< Average reporting latency
        
        std::chrono::system_clock::time_point last_observation;
        std::chrono::system_clock::time_point first_observation;
        
        // Performance trends
        std::vector<double> accuracy_history;
        std::vector<double> precision_history;
        std::vector<std::chrono::system_clock::time_point> timestamps;
    };
    
private:
    std::unordered_map<std::string, SourceMetrics> source_metrics_;
    
    // Configuration
    double decay_half_life_days_;           ///< Reliability decay half-life
    size_t min_observations_for_rating_;   ///< Minimum observations for reliable rating
    double initial_reliability_;           ///< Initial reliability for new sources
    
public:
    /**
     * @brief Construct reliability tracker
     * @param decay_half_life_days Half-life for reliability decay
     * @param min_observations Minimum observations for rating
     * @param initial_reliability Initial reliability for new sources
     */
    explicit SourceReliabilityTracker(double decay_half_life_days = 30.0,
                                     size_t min_observations = 10,
                                     double initial_reliability = 0.5);
    
    /**
     * @brief Update source metrics with new observation
     * @param source_id Data source identifier
     * @param observation New observation
     * @param validation_result Optional validation result
     */
    void update_source_metrics(const std::string& source_id,
                              const Observation& observation,
                              const std::optional<double>& validation_result = std::nullopt);
    
    /**
     * @brief Calculate current reliability score for source
     * @param source_id Data source identifier
     * @return Current reliability score [0,1]
     */
    double calculate_reliability(const std::string& source_id) const;
    
    /**
     * @brief Get source performance metrics
     * @param source_id Data source identifier
     * @return Source metrics structure
     */
    const SourceMetrics* get_source_metrics(const std::string& source_id) const;
    
    /**
     * @brief Get all tracked sources ranked by reliability
     * @return Vector of (source_id, reliability) pairs, sorted by reliability
     */
    std::vector<std::pair<std::string, double>> get_ranked_sources() const;
    
    // Configuration
    void set_decay_half_life(double days) { decay_half_life_days_ = days; }
    void set_minimum_observations(size_t min_obs) { min_observations_for_rating_ = min_obs; }
};

/**
 * @brief Bayesian data fusion engine
 * 
 * Implements optimal Bayesian fusion of multi-source observations
 * with uncertainty propagation and conflict resolution.
 */
class BayesianDataFusion {
public:
    /**
     * @brief Fusion configuration parameters
     */
    struct FusionConfig {
        double temporal_correlation_threshold = 60.0;  ///< seconds
        double spatial_correlation_threshold = 1.0;    ///< degrees
        double outlier_detection_threshold = 3.0;      ///< sigma
        double min_agreement_threshold = 0.7;          ///< [0,1]
        bool enable_robust_estimation = true;          ///< Outlier-robust fusion
        bool enable_temporal_smoothing = true;         ///< Temporal consistency
        int max_iterations = 100;                      ///< Maximum fusion iterations
        double convergence_tolerance = 1e-6;           ///< Convergence criterion
    };
    
    /**
     * @brief Fusion result with diagnostics
     */
    struct FusionResult {
        std::vector<uncertainty::UncertainQuantity> fused_values;
        std::vector<std::string> value_names;
        
        double fusion_quality;                         ///< Overall fusion quality [0,1]
        double consensus_level;                        ///< Inter-source agreement [0,1]
        size_t num_sources_used;                      ///< Number of contributing sources
        size_t num_outliers_rejected;                 ///< Number of outliers removed
        
        std::vector<double> source_weights;           ///< Final source weights
        std::vector<std::string> source_ids;          ///< Contributing sources
        std::vector<std::string> outlier_ids;         ///< Rejected outlier sources
        
        Eigen::MatrixXd posterior_covariance;         ///< Final covariance matrix
        std::string fusion_method;                    ///< Method used for fusion
        
        std::chrono::system_clock::time_point fusion_timestamp;
        double processing_time_ms;                     ///< Fusion processing time
    };
    
private:
    FusionConfig config_;
    SourceReliabilityTracker reliability_tracker_;
    
    // Fusion state
    std::unordered_map<std::string, std::queue<Observation>> observation_buffers_;
    
public:
    /**
     * @brief Construct Bayesian fusion engine
     * @param config Fusion configuration
     */
    explicit BayesianDataFusion(const FusionConfig& config = FusionConfig{});
    
    /**
     * @brief Fuse multiple observations of the same quantity
     * @param observations Vector of related observations
     * @return Fusion result with uncertainty quantification
     */
    FusionResult fuse_observations(const std::vector<Observation>& observations);
    
    /**
     * @brief Add observation to fusion buffer
     * @param observation New observation to buffer
     */
    void add_observation(const Observation& observation);
    
    /**
     * @brief Process buffered observations and produce fusion results
     * @param target_quantity Name of quantity to fuse
     * @param temporal_window_seconds Temporal window for fusion
     * @return Fusion result or nullopt if insufficient data
     */
    std::optional<FusionResult> process_buffered_observations(
        const std::string& target_quantity,
        double temporal_window_seconds = 300.0);
    
    /**
     * @brief Detect and reject outlier observations
     * @param observations Input observations
     * @return Vector of non-outlier observations
     */
    std::vector<Observation> detect_and_reject_outliers(
        const std::vector<Observation>& observations) const;
    
    /**
     * @brief Calculate optimal weights for source fusion
     * @param observations Input observations
     * @return Optimal weight vector
     */
    std::vector<double> calculate_optimal_weights(
        const std::vector<Observation>& observations) const;
    
    /**
     * @brief Perform robust weighted fusion
     * @param observations Input observations
     * @param weights Source weights
     * @return Fused uncertain quantities
     */
    std::vector<uncertainty::UncertainQuantity> weighted_fusion(
        const std::vector<Observation>& observations,
        const std::vector<double>& weights) const;
    
    // Configuration and state
    const FusionConfig& get_config() const { return config_; }
    void set_config(const FusionConfig& config) { config_ = config; }
    SourceReliabilityTracker& get_reliability_tracker() { return reliability_tracker_; }
    
    // Diagnostics
    size_t get_buffer_size() const;
    std::vector<std::string> get_active_sources() const;
    
private:
    // Internal fusion algorithms
    FusionResult kalman_fusion(const std::vector<Observation>& observations);
    FusionResult weighted_least_squares_fusion(const std::vector<Observation>& observations);
    FusionResult robust_fusion(const std::vector<Observation>& observations);
    
    // Utility methods
    bool are_observations_compatible(const Observation& obs1, const Observation& obs2) const;
    double calculate_temporal_weight(const Observation& obs, 
                                   const std::chrono::system_clock::time_point& reference_time) const;
    Eigen::MatrixXd estimate_cross_covariances(const std::vector<Observation>& observations) const;
};

/**
 * @brief Real-time data fusion coordinator
 * 
 * Coordinates real-time fusion of streaming observations from multiple
 * sources with adaptive quality control and conflict resolution.
 */
class RealTimeFusionCoordinator {
public:
    /**
     * @brief Fusion event callback function type
     */
    using FusionCallback = std::function<void(const BayesianDataFusion::FusionResult&)>;
    using QualityCallback = std::function<void(const std::string&, QualityFlag)>;
    
    /**
     * @brief Coordinator configuration
     */
    struct CoordinatorConfig {
        size_t max_concurrent_fusions = 1000;         ///< Maximum parallel fusions
        double fusion_timeout_seconds = 30.0;         ///< Fusion timeout
        double quality_check_interval_seconds = 60.0; ///< QC check interval
        bool enable_automatic_source_ranking = true;  ///< Auto source ranking
        bool enable_conflict_alerts = true;           ///< Conflict notifications
        double conflict_detection_threshold = 0.3;    ///< Conflict threshold
        size_t max_observation_buffer_size = 100000;  ///< Max buffered observations
    };
    
private:
    CoordinatorConfig config_;
    BayesianDataFusion fusion_engine_;
    
    // Threading and synchronization
    std::vector<std::thread> worker_threads_;
    std::mutex observation_mutex_;
    std::condition_variable fusion_condition_;
    std::atomic<bool> running_{false};
    
    // Callbacks
    FusionCallback fusion_callback_;
    QualityCallback quality_callback_;
    
    // Performance monitoring
    std::atomic<size_t> fusions_completed_{0};
    std::atomic<size_t> observations_processed_{0};
    std::atomic<size_t> conflicts_detected_{0};
    std::chrono::system_clock::time_point start_time_;
    
    // Active fusion tracking
    std::unordered_map<std::string, std::future<BayesianDataFusion::FusionResult>> active_fusions_;
    std::mutex fusion_mutex_;
    
public:
    /**
     * @brief Construct fusion coordinator
     * @param config Coordinator configuration
     * @param fusion_callback Callback for fusion results
     * @param quality_callback Callback for quality issues
     */
    explicit RealTimeFusionCoordinator(
        const CoordinatorConfig& config = CoordinatorConfig{},
        FusionCallback fusion_callback = nullptr,
        QualityCallback quality_callback = nullptr);
    
    /**
     * @brief Start real-time fusion processing
     * @param num_workers Number of worker threads
     */
    void start(size_t num_workers = std::thread::hardware_concurrency());
    
    /**
     * @brief Stop fusion processing
     */
    void stop();
    
    /**
     * @brief Submit observation for fusion
     * @param observation New observation to process
     * @return True if successfully queued
     */
    bool submit_observation(const Observation& observation);
    
    /**
     * @brief Submit batch of observations
     * @param observations Vector of observations
     * @return Number of successfully queued observations
     */
    size_t submit_observations(const std::vector<Observation>& observations);
    
    /**
     * @brief Get processing statistics
     */
    struct ProcessingStats {
        size_t fusions_completed;
        size_t observations_processed;
        size_t conflicts_detected;
        double processing_rate_per_second;
        double uptime_seconds;
        size_t active_fusions;
        size_t buffer_utilization;
    };
    
    ProcessingStats get_processing_stats() const;
    
    // Configuration
    const CoordinatorConfig& get_config() const { return config_; }
    void set_config(const CoordinatorConfig& config) { config_ = config; }
    void set_fusion_callback(FusionCallback callback) { fusion_callback_ = callback; }
    void set_quality_callback(QualityCallback callback) { quality_callback_ = callback; }
    
private:
    void worker_loop();
    void quality_control_loop();
    void process_pending_fusions();
    void detect_conflicts();
};

} // namespace fusion
} // namespace m17
} // namespace ChiragRathi

#endif // CHIRAGRATHI_M17_DATA_FUSION_HPP
