/**
 * @file scientific_memory.hpp
 * @brief AI-Enhanced Scientific Memory and Pattern Recognition System
 * 
 * ChiragRathi M17 Autonomous Celestial Discovery Framework
 * 
 * Implements intelligent case-based reasoning, anomaly pattern classification,
 * and adaptive learning for autonomous astronomical discovery.
 * 
 * Core Features:
 * - Multi-dimensional case representation with uncertainty
 * - Real-time anomaly classification using deep learning
 * - Temporal pattern evolution tracking
 * - Confidence assessment with epistemic uncertainty
 * - Distributed knowledge graph representation
 * 
 * @author ChiragRathi
 * @version 2.0.0
 * @date 2026
 */

#ifndef CHIRAGRATHI_M17_SCIENTIFIC_MEMORY_HPP
#define CHIRAGRATHI_M17_SCIENTIFIC_MEMORY_HPP

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <chrono>
#include <functional>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include "../uncertainty/uncertainty_engine.hpp"

namespace ChiragRathi {
namespace m17 {
namespace memory {

/**
 * @brief Scientific case classification categories
 */
enum class AnomalyType {
    NOMINAL = 0,                    ///< Normal operation/behavior
    ORBITAL_DECAY = 1,              ///< Atmospheric drag induced decay
    GRAVITATIONAL_ANOMALY = 2,      ///< Unexplained gravitational effects
    ZOMBIE_SATELLITE = 3,           ///< Non-responsive but trackable satellite
    STELLAR_VARIABLE = 4,           ///< Variable star behavior
    TRANSIENT_EVENT = 5,            ///< Short-duration transient
    GRAVITATIONAL_LENS = 6,         ///< Gravitational lensing signature
    DARK_MATTER_CANDIDATE = 7,      ///< Potential dark matter interaction
    DEBRIS_COLLISION = 8,           ///< Spacecraft/debris collision
    ATTITUDE_ANOMALY = 9,           ///< Attitude control system failure
    PROPULSION_EVENT = 10,          ///< Thruster firing or malfunction
    UNKNOWN_FORCE = 11,             ///< Unmodeled acceleration
    SENSOR_MALFUNCTION = 12,        ///< Instrument/sensor error
    INTERFERENCE_PATTERN = 13,      ///< Signal interference
    EXOTIC_OBJECT = 14,             ///< Potential exotic physics signature
    CLASSIFICATION_UNCERTAIN = 15   ///< Insufficient data for classification
};

/**
 * @brief Case confidence and evolution state
 */
enum class CaseState {
    INITIAL_DETECTION,              ///< First observation of anomaly
    UNDER_INVESTIGATION,           ///< Gathering additional data
    PATTERN_CONFIRMED,             ///< Consistent pattern established
    CLASSIFIED_CONFIDENT,          ///< High-confidence classification
    REQUIRES_HUMAN_REVIEW,         ///< Expert review requested
    RESOLVED_EXPLAINED,            ///< Anomaly explained and resolved
    RESOLVED_UNEXPLAINED,          ///< Monitoring ceased, no explanation
    ARCHIVED,                      ///< Stored for historical reference
    ESCALATED_URGENT,              ///< Requires immediate attention
    FALSE_POSITIVE                 ///< Determined to be measurement artifact
};

/**
 * @brief Multi-dimensional feature vector with uncertainty
 * 
 * Represents observational features and derived quantities with
 * full uncertainty propagation for robust pattern matching.
 */
class FeatureVector {
public:
    using UncertainQuantity = uncertainty::UncertainQuantity;
    
private:
    std::vector<UncertainQuantity> features_;
    std::vector<std::string> feature_names_;
    Eigen::MatrixXd correlation_matrix_;
    std::chrono::system_clock::time_point timestamp_;
    
public:
    /**
     * @brief Construct feature vector
     * @param features Vector of uncertain feature values
     * @param names Feature names for interpretability
     */
    FeatureVector(const std::vector<UncertainQuantity>& features,
                 const std::vector<std::string>& names);
    
    /**
     * @brief Construct with correlation matrix
     * @param features Vector of uncertain feature values
     * @param names Feature names
     * @param correlations Feature correlation matrix
     */
    FeatureVector(const std::vector<UncertainQuantity>& features,
                 const std::vector<std::string>& names,
                 const Eigen::MatrixXd& correlations);
    
    // Accessors
    size_t size() const { return features_.size(); }
    const UncertainQuantity& operator[](size_t index) const { return features_[index]; }
    const std::string& feature_name(size_t index) const { return feature_names_[index]; }
    const Eigen::MatrixXd& correlation_matrix() const { return correlation_matrix_; }
    
    // Statistical operations
    double mahalanobis_distance(const FeatureVector& other) const;
    double euclidean_distance(const FeatureVector& other) const;
    double correlation_coefficient(const FeatureVector& other) const;
    
    // Feature engineering
    FeatureVector normalize() const;
    FeatureVector standardize() const;
    FeatureVector principal_components(int num_components) const;
    
    // Serialization
    std::string to_json() const;
    static FeatureVector from_json(const std::string& json_data);
    
    // Time-based operations
    void set_timestamp(const std::chrono::system_clock::time_point& time) { timestamp_ = time; }
    std::chrono::system_clock::time_point timestamp() const { return timestamp_; }
    double age_seconds() const;
};

/**
 * @brief Comprehensive scientific case representation
 * 
 * Encapsulates all information about a detected anomaly including
 * observational data, classification results, temporal evolution,
 * and confidence assessment.
 */
class ScientificCase {
public:
    using TimePoint = std::chrono::system_clock::time_point;
    
private:
    std::string case_id_;                           ///< Unique case identifier
    AnomalyType classification_;                    ///< Current classification
    CaseState state_;                              ///< Case evolution state
    
    FeatureVector initial_features_;               ///< Initial detection features
    std::vector<FeatureVector> feature_evolution_; ///< Temporal feature evolution
    
    double confidence_score_;                      ///< Classification confidence [0,1]
    uncertainty::UncertainQuantity confidence_uncertainty_; ///< Confidence uncertainty
    
    std::vector<std::string> evidence_chain_;      ///< Supporting evidence
    std::vector<std::string> hypotheses_;          ///< Alternative explanations
    
    TimePoint creation_time_;                      ///< Case creation timestamp
    TimePoint last_update_;                        ///< Last modification time
    
    std::unordered_map<std::string, double> metadata_; ///< Additional metadata
    
    // Bayesian updating
    Eigen::VectorXd prior_probabilities_;          ///< Prior class probabilities
    Eigen::VectorXd posterior_probabilities_;      ///< Updated probabilities
    
public:
    /**
     * @brief Construct scientific case
     * @param case_id Unique identifier
     * @param initial_features Initial observational features
     * @param initial_classification Initial classification guess
     */
    ScientificCase(const std::string& case_id,
                  const FeatureVector& initial_features,
                  AnomalyType initial_classification = AnomalyType::CLASSIFICATION_UNCERTAIN);
    
    // Accessors
    const std::string& case_id() const { return case_id_; }
    AnomalyType classification() const { return classification_; }
    CaseState state() const { return state_; }
    double confidence_score() const { return confidence_score_; }
    const uncertainty::UncertainQuantity& confidence_uncertainty() const { return confidence_uncertainty_; }
    const FeatureVector& current_features() const;
    const std::vector<FeatureVector>& feature_evolution() const { return feature_evolution_; }
    
    // Case management
    void update_classification(AnomalyType new_class, double confidence);
    void update_state(CaseState new_state);
    void add_evidence(const std::string& evidence);
    void add_hypothesis(const std::string& hypothesis);
    void add_features(const FeatureVector& new_features);
    
    // Bayesian inference
    void update_posterior(const Eigen::VectorXd& likelihood);
    Eigen::VectorXd get_posterior_probabilities() const { return posterior_probabilities_; }
    
    // Temporal analysis
    double time_since_creation_hours() const;
    double time_since_update_seconds() const;
    bool has_evolved() const { return feature_evolution_.size() > 1; }
    
    // Metadata management
    void set_metadata(const std::string& key, double value) { metadata_[key] = value; }
    double get_metadata(const std::string& key, double default_value = 0.0) const;
    
    // Pattern matching
    double similarity_score(const ScientificCase& other) const;
    bool is_compatible_with(const ScientificCase& other, double threshold = 0.8) const;
    
    // Serialization
    std::string to_json() const;
    static ScientificCase from_json(const std::string& json_data);
};

/**
 * @brief Deep learning anomaly classifier with uncertainty quantification
 * 
 * Implements neural network architectures optimized for astronomical
 * anomaly classification with epistemic and aleatoric uncertainty estimation.
 */
class DeepAnomalyClassifier {
public:
    /**
     * @brief Neural network architecture configuration
     */
    struct NetworkConfig {
        std::vector<int> hidden_layers = {256, 128, 64};  ///< Hidden layer sizes
        double dropout_rate = 0.3;                       ///< Dropout probability
        double learning_rate = 0.001;                    ///< Learning rate
        int num_monte_carlo_samples = 100;               ///< MC dropout samples
        std::string activation = "relu";                 ///< Activation function
        std::string optimizer = "adam";                  ///< Optimization algorithm
        bool use_batch_normalization = true;             ///< Batch normalization
        bool enable_uncertainty = true;                  ///< Uncertainty quantification
    };
    
    /**
     * @brief Classification result with uncertainty
     */
    struct ClassificationResult {
        AnomalyType predicted_class;                     ///< Most likely class
        Eigen::VectorXd class_probabilities;            ///< Probability distribution
        uncertainty::UncertainQuantity confidence;      ///< Confidence with uncertainty
        double epistemic_uncertainty;                   ///< Model uncertainty
        double aleatoric_uncertainty;                   ///< Data uncertainty
        std::vector<std::string> explanation;           ///< Feature importance explanation
    };
    
private:
    NetworkConfig config_;
    bool is_trained_;
    
    // Model state (in real implementation, would use TensorFlow/PyTorch)
    std::vector<Eigen::MatrixXd> weights_;
    std::vector<Eigen::VectorXd> biases_;
    
    // Training data statistics
    FeatureVector feature_means_;
    FeatureVector feature_stds_;
    
    // Performance metrics
    double training_accuracy_;
    double validation_accuracy_;
    Eigen::MatrixXd confusion_matrix_;
    
public:
    /**
     * @brief Construct classifier
     * @param config Network configuration
     */
    explicit DeepAnomalyClassifier(const NetworkConfig& config = NetworkConfig{});
    
    /**
     * @brief Train classifier on labeled data
     * @param training_features Training feature vectors
     * @param training_labels Training labels
     * @param validation_split Fraction of data for validation
     * @return Training accuracy
     */
    double train(const std::vector<FeatureVector>& training_features,
                const std::vector<AnomalyType>& training_labels,
                double validation_split = 0.2);
    
    /**
     * @brief Classify single case with uncertainty
     * @param features Input feature vector
     * @return Classification result with uncertainty quantification
     */
    ClassificationResult classify(const FeatureVector& features) const;
    
    /**
     * @brief Batch classification for efficiency
     * @param feature_batch Vector of feature vectors
     * @return Vector of classification results
     */
    std::vector<ClassificationResult> classify_batch(
        const std::vector<FeatureVector>& feature_batch) const;
    
    /**
     * @brief Online learning update
     * @param new_features New training features
     * @param new_labels New training labels
     */
    void update_online(const std::vector<FeatureVector>& new_features,
                      const std::vector<AnomalyType>& new_labels);
    
    /**
     * @brief Get feature importance scores
     * @param features Input features
     * @return Feature importance values
     */
    std::vector<double> get_feature_importance(const FeatureVector& features) const;
    
    // Model management
    bool is_trained() const { return is_trained_; }
    double get_training_accuracy() const { return training_accuracy_; }
    double get_validation_accuracy() const { return validation_accuracy_; }
    const Eigen::MatrixXd& get_confusion_matrix() const { return confusion_matrix_; }
    
    // Serialization
    bool save_model(const std::string& filepath) const;
    bool load_model(const std::string& filepath);
    
    // Configuration
    const NetworkConfig& get_config() const { return config_; }
    void set_config(const NetworkConfig& config) { config_ = config; }
};

/**
 * @brief Comprehensive scientific memory system
 * 
 * Central repository for all anomaly cases with intelligent retrieval,
 * pattern matching, and knowledge discovery capabilities.
 */
class ScientificMemorySystem {
public:
    /**
     * @brief Memory system configuration
     */
    struct MemoryConfig {
        size_t max_active_cases = 100000;              ///< Maximum active cases
        size_t max_archived_cases = 1000000;           ///< Maximum archived cases
        double similarity_threshold = 0.7;             ///< Case similarity threshold
        double confidence_threshold = 0.8;             ///< High confidence threshold
        int max_feature_evolution_steps = 1000;        ///< Max temporal evolution steps
        bool enable_real_time_learning = true;         ///< Online learning
        bool enable_pattern_discovery = true;          ///< Automatic pattern discovery
        double pruning_interval_hours = 24.0;          ///< Memory pruning interval
    };
    
    /**
     * @brief Query structure for case retrieval
     */
    struct CaseQuery {
        std::vector<AnomalyType> target_classes;       ///< Target classifications
        std::vector<CaseState> target_states;          ///< Target case states
        FeatureVector reference_features;              ///< Reference feature vector
        double max_distance = 1.0;                     ///< Maximum feature distance
        double min_confidence = 0.0;                   ///< Minimum confidence score
        int max_results = 100;                         ///< Maximum number of results
        bool sort_by_similarity = true;                ///< Sort results by similarity
    };
    
private:
    MemoryConfig config_;
    
    // Case storage
    std::unordered_map<std::string, std::shared_ptr<ScientificCase>> active_cases_;
    std::unordered_map<std::string, std::shared_ptr<ScientificCase>> archived_cases_;
    
    // Classification system
    DeepAnomalyClassifier classifier_;
    
    // Indexing structures
    std::vector<std::shared_ptr<ScientificCase>> temporal_index_;  ///< Time-ordered cases
    std::unordered_map<AnomalyType, std::vector<std::string>> class_index_;  ///< Class-based index
    
    // Knowledge graph (sparse matrix representation)
    Eigen::SparseMatrix<double> case_similarity_graph_;
    std::unordered_map<std::string, int> case_to_index_;
    std::vector<std::string> index_to_case_;
    
    // Performance metrics
    struct PerformanceMetrics {
        size_t total_cases_processed = 0;
        size_t successful_classifications = 0;
        size_t false_positives = 0;
        size_t false_negatives = 0;
        double average_classification_time_ms = 0.0;
        double memory_usage_mb = 0.0;
    } metrics_;
    
    // Threading
    mutable std::shared_mutex memory_mutex_;
    
public:
    /**
     * @brief Construct scientific memory system
     * @param config Memory configuration
     */
    explicit ScientificMemorySystem(const MemoryConfig& config = MemoryConfig{});
    
    /**
     * @brief Add new case to memory
     * @param new_case Scientific case to add
     * @return True if successfully added
     */
    bool add_case(std::shared_ptr<ScientificCase> new_case);
    
    /**
     * @brief Retrieve case by ID
     * @param case_id Case identifier
     * @return Pointer to case or nullptr if not found
     */
    std::shared_ptr<ScientificCase> get_case(const std::string& case_id) const;
    
    /**
     * @brief Query cases matching criteria
     * @param query Query specification
     * @return Vector of matching cases
     */
    std::vector<std::shared_ptr<ScientificCase>> query_cases(const CaseQuery& query) const;
    
    /**
     * @brief Classify new observation
     * @param features Observational features
     * @param case_id Optional case ID (generated if empty)
     * @return New scientific case with classification
     */
    std::shared_ptr<ScientificCase> classify_observation(
        const FeatureVector& features,
        const std::string& case_id = "");
    
    /**
     * @brief Update existing case with new observations
     * @param case_id Case identifier
     * @param new_features New observational features
     * @return True if successfully updated
     */
    bool update_case(const std::string& case_id, const FeatureVector& new_features);
    
    /**
     * @brief Find similar cases
     * @param reference_case Reference case for similarity search
     * @param max_results Maximum number of results
     * @param min_similarity Minimum similarity threshold
     * @return Vector of similar cases with similarity scores
     */
    std::vector<std::pair<std::shared_ptr<ScientificCase>, double>> 
    find_similar_cases(const ScientificCase& reference_case,
                      int max_results = 10,
                      double min_similarity = 0.5) const;
    
    /**
     * @brief Discover patterns across cases
     * @param target_class Target anomaly type to analyze
     * @param min_cases Minimum number of cases required
     * @return Vector of discovered patterns
     */
    std::vector<std::string> discover_patterns(AnomalyType target_class, int min_cases = 10) const;
    
    // Memory management
    void archive_resolved_cases();
    void prune_low_confidence_cases();
    void optimize_similarity_graph();
    
    // Statistics and monitoring
    size_t get_active_case_count() const;
    size_t get_archived_case_count() const;
    PerformanceMetrics get_performance_metrics() const { return metrics_; }
    std::unordered_map<AnomalyType, int> get_class_distribution() const;
    
    // Serialization and persistence
    bool save_to_file(const std::string& filepath) const;
    bool load_from_file(const std::string& filepath);
    
    // Configuration
    const MemoryConfig& get_config() const { return config_; }
    void set_config(const MemoryConfig& config) { config_ = config; }
    
    // Training and learning
    bool retrain_classifier();
    void enable_online_learning(bool enable = true);
    
    /**
     * @brief Generate comprehensive case report
     * @param case_id Case identifier
     * @return Detailed case analysis report
     */
    std::string generate_case_report(const std::string& case_id) const;
    
    /**
     * @brief Export cases for external analysis
     * @param format Export format ("json", "csv", "hdf5")
     * @param filepath Output file path
     * @param query Optional query filter
     * @return True if export successful
     */
    bool export_cases(const std::string& format,
                     const std::string& filepath,
                     const CaseQuery* query = nullptr) const;
};

} // namespace memory
} // namespace m17
} // namespace ChiragRathi

#endif // CHIRAGRATHI_M17_SCIENTIFIC_MEMORY_HPP
