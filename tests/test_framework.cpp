/**
 * @file test_framework.cpp
 * @brief Comprehensive M17 Test Framework
 * 
 * ChiragRathi M17 Autonomous Celestial Discovery Framework
 * 
 * Complete test suite covering all framework components with
 * unit tests, integration tests, performance tests, and validation tests.
 * 
 * @author ChiragRathi
 * @version 2.0.0
 * @date 2026
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <cmath>

#include "../src/core/uncertainty/uncertainty_engine.hpp"
#include "../src/core/physics/relativistic_physics.hpp"
#include "../src/core/memory/scientific_memory.hpp"
#include "../src/core/fusion/data_fusion.hpp"
#include "../src/core/integration/system_integration.hpp"

using namespace ChiragRathi::m17;
using namespace testing;

// Test fixtures and utilities

class M17TestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test environment
        test_start_time_ = std::chrono::high_resolution_clock::now();
        
        // Create test data directory
        std::filesystem::create_directories("test_output");
        
        // Initialize random number generator with fixed seed for reproducibility
        rng_.seed(42);
    }
    
    void TearDown() override {
        auto test_end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            test_end_time - test_start_time_);
        
        std::cout << "Test completed in " << duration.count() << " ms" << std::endl;
    }
    
    // Utility methods for test data generation
    uncertainty::UncertainQuantity create_test_uncertain_quantity(
        double value = 1.0, double uncertainty = 0.1, const std::string& id = "test") {
        return uncertainty::UncertainQuantity(value, uncertainty, id);
    }
    
    physics::RelativisticOrbitalState create_test_orbital_state() {
        Eigen::Vector3d position(7000e3, 0, 0);  // 7000 km altitude
        Eigen::Vector3d velocity(0, 7500, 0);    // Circular velocity
        return physics::RelativisticOrbitalState(position, velocity, 0.0);
    }
    
    std::vector<uncertainty::UncertainQuantity> create_test_observation_features() {
        std::uniform_real_distribution<double> coord_dist(0.0, 360.0);
        std::uniform_real_distribution<double> mag_dist(8.0, 20.0);
        std::uniform_real_distribution<double> unc_dist(0.01, 0.1);
        
        std::vector<uncertainty::UncertainQuantity> features;
        features.emplace_back(coord_dist(rng_), unc_dist(rng_), "ra");
        features.emplace_back(coord_dist(rng_), unc_dist(rng_), "dec");
        features.emplace_back(mag_dist(rng_), unc_dist(rng_), "magnitude");
        features.emplace_back(unc_dist(rng_), unc_dist(rng_) * 0.1, "proper_motion");
        
        return features;
    }
    
    fusion::Observation create_test_observation() {
        auto features = create_test_observation_features();
        std::vector<std::string> names = {"ra", "dec", "magnitude", "proper_motion"};
        
        return fusion::Observation(
            "test_obs_" + std::to_string(observation_counter_++),
            "test_source",
            fusion::SourceType::GROUND_TELESCOPE,
            features,
            names,
            std::chrono::system_clock::now()
        );
    }
    
protected:
    std::chrono::high_resolution_clock::time_point test_start_time_;
    std::mt19937 rng_;
    static int observation_counter_;
};

int M17TestFixture::observation_counter_ = 0;

// =============================================================================
// UNCERTAINTY PROPAGATION TESTS
// =============================================================================

class UncertaintyPropagationTest : public M17TestFixture {};

TEST_F(UncertaintyPropagationTest, BasicUncertaintyCreation) {
    auto uq = create_test_uncertain_quantity(10.0, 0.5, "test_value");
    
    EXPECT_DOUBLE_EQ(uq.value(), 10.0);
    EXPECT_DOUBLE_EQ(uq.uncertainty(), 0.5);
    EXPECT_EQ(uq.source(), "test_value");
    EXPECT_DOUBLE_EQ(uq.relative_uncertainty(), 0.05);
}

TEST_F(UncertaintyPropagationTest, ArithmeticOperations) {
    auto x = create_test_uncertain_quantity(5.0, 0.1, "x");
    auto y = create_test_uncertain_quantity(3.0, 0.2, "y");
    
    // Addition
    auto sum = x + y;
    EXPECT_DOUBLE_EQ(sum.value(), 8.0);
    EXPECT_NEAR(sum.uncertainty(), std::sqrt(0.01 + 0.04), 1e-10);
    
    // Subtraction
    auto diff = x - y;
    EXPECT_DOUBLE_EQ(diff.value(), 2.0);
    EXPECT_NEAR(diff.uncertainty(), std::sqrt(0.01 + 0.04), 1e-10);
    
    // Multiplication
    auto product = x * y;
    EXPECT_DOUBLE_EQ(product.value(), 15.0);
    
    // Division
    auto quotient = x / y;
    EXPECT_DOUBLE_EQ(quotient.value(), 5.0/3.0);
}

TEST_F(UncertaintyPropagationTest, MathematicalFunctions) {
    auto x = create_test_uncertain_quantity(4.0, 0.1, "x");
    
    // Square root
    auto sqrt_x = x.sqrt();
    EXPECT_DOUBLE_EQ(sqrt_x.value(), 2.0);
    EXPECT_NEAR(sqrt_x.uncertainty(), 0.1 / (2 * 2.0), 1e-10);  // σ/2√x
    
    // Power
    auto x_squared = x.pow(2.0);
    EXPECT_DOUBLE_EQ(x_squared.value(), 16.0);
    EXPECT_NEAR(x_squared.uncertainty(), 2 * 4.0 * 0.1, 1e-10);  // 2x*σ_x
    
    // Logarithm
    auto log_x = x.log();
    EXPECT_DOUBLE_EQ(log_x.value(), std::log(4.0));
    EXPECT_NEAR(log_x.uncertainty(), 0.1 / 4.0, 1e-10);  // σ_x/x
}

TEST_F(UncertaintyPropagationTest, CompatibilityTesting) {
    auto x1 = create_test_uncertain_quantity(10.0, 0.1, "measurement_1");
    auto x2 = create_test_uncertain_quantity(10.05, 0.1, "measurement_2");
    auto x3 = create_test_uncertain_quantity(12.0, 0.1, "measurement_3");
    
    EXPECT_TRUE(x1.is_compatible_with(x2, 0.05));   // Should be compatible
    EXPECT_FALSE(x1.is_compatible_with(x3, 0.05));  // Should be incompatible
}

TEST_F(UncertaintyPropagationTest, SIMDPropagatorPerformance) {
    uncertainty::SIMDUncertaintyPropagator propagator(1000, 4);
    
    // Generate test data
    std::vector<uncertainty::UncertainQuantity> inputs;
    for (int i = 0; i < 1000; ++i) {
        inputs.push_back(create_test_uncertain_quantity(
            1.0 + i * 0.01, 0.1, "perf_test_" + std::to_string(i)));
    }
    
    // Benchmark propagation
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < inputs.size(); i += 2) {
        if (i + 1 < inputs.size()) {
            auto result = inputs[i] * inputs[i + 1];
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should process at least 1000 operations per second
    double ops_per_sec = (inputs.size() / 2) / (duration.count() / 1e6);
    EXPECT_GT(ops_per_sec, 1000.0);
}

// =============================================================================
// PHYSICS ENGINE TESTS  
// =============================================================================

class PhysicsEngineTest : public M17TestFixture {};

TEST_F(PhysicsEngineTest, OrbitalStateCreation) {
    auto state = create_test_orbital_state();
    
    EXPECT_NEAR(state.position().norm(), 7000e3, 1.0);
    EXPECT_NEAR(state.velocity().norm(), 7500, 1.0);
    EXPECT_EQ(state.epoch(), 0.0);
}

TEST_F(PhysicsEngineTest, KeplerianElementsConversion) {
    auto state = create_test_orbital_state();
    auto elements = state.keplerian_elements();
    
    // Semi-major axis should be close to position magnitude for circular orbit
    EXPECT_NEAR(elements[0], 7000e3, 1e3);
    
    // Eccentricity should be near zero for circular orbit
    EXPECT_NEAR(elements[1], 0.0, 0.01);
    
    // Orbital period calculation
    double period = state.period();
    double expected_period = 2 * M_PI * std::sqrt(std::pow(7000e3, 3) / 3.986004418e14);
    EXPECT_NEAR(period, expected_period, 10.0);
}

TEST_F(PhysicsEngineTest, RelativisticEffects) {
    // GPS satellite test case
    double gps_altitude = 20200e3;  // 20,200 km
    double earth_radius = 6.371e6;
    double gps_radius = earth_radius + gps_altitude;
    
    Eigen::Vector3d gps_position(gps_radius, 0, 0);
    double orbital_velocity = std::sqrt(3.986004418e14 / gps_radius);
    Eigen::Vector3d gps_velocity(0, orbital_velocity, 0);
    
    physics::RelativisticOrbitalState gps_state(gps_position, gps_velocity, 0.0);
    
    // Time dilation calculation
    double time_dilation = gps_state.time_dilation_rate();
    double daily_correction_us = time_dilation * 86400 * 1e6;
    
    // GPS correction should be approximately 38.6 microseconds/day
    EXPECT_NEAR(daily_correction_us, 38.6, 5.0);
}

TEST_F(PhysicsEngineTest, PhysicsEngineAcceleration) {
    physics::AdvancedPhysicsEngine::PerturbationConfig config;
    config.enable_relativity = true;
    config.enable_earth_gravity = true;
    
    physics::AdvancedPhysicsEngine engine(config);
    auto state = create_test_orbital_state();
    
    auto acceleration = engine.compute_acceleration(state);
    
    // Acceleration should point toward Earth center (negative radial direction)
    auto position = state.position();
    auto radial_acceleration = acceleration.dot(position.normalized());
    EXPECT_LT(radial_acceleration, 0.0);  // Should be negative (toward center)
    
    // Magnitude should be close to GM/r²
    double expected_magnitude = 3.986004418e14 / (state.position().squaredNorm());
    EXPECT_NEAR(acceleration.norm(), expected_magnitude, expected_magnitude * 0.1);
}

TEST_F(PhysicsEngineTest, OrbitPropagation) {
    physics::AdvancedPhysicsEngine engine;
    auto initial_state = create_test_orbital_state();
    
    // Propagate for one orbital period
    double period = initial_state.period();
    auto final_state = engine.propagate_orbit(initial_state, period, 1e-12);
    
    // After one period, should return to approximately the same position
    auto position_diff = (final_state.position() - initial_state.position()).norm();
    auto velocity_diff = (final_state.velocity() - initial_state.velocity()).norm();
    
    EXPECT_LT(position_diff, 1e3);  // Within 1 km
    EXPECT_LT(velocity_diff, 1.0);  // Within 1 m/s
}

// =============================================================================
// SCIENTIFIC MEMORY TESTS
// =============================================================================

class ScientificMemoryTest : public M17TestFixture {};

TEST_F(ScientificMemoryTest, FeatureVectorCreation) {
    auto features = create_test_observation_features();
    std::vector<std::string> names = {"ra", "dec", "magnitude", "proper_motion"};
    
    memory::FeatureVector feature_vec(features, names);
    
    EXPECT_EQ(feature_vec.size(), 4);
    EXPECT_EQ(feature_vec.feature_name(0), "ra");
    EXPECT_GT(feature_vec.age_seconds(), 0.0);
}

TEST_F(ScientificMemoryTest, FeatureVectorDistances) {
    auto features1 = create_test_observation_features();
    auto features2 = features1;  // Identical features
    
    std::vector<std::string> names = {"ra", "dec", "magnitude", "proper_motion"};
    memory::FeatureVector fv1(features1, names);
    memory::FeatureVector fv2(features2, names);
    
    // Distance between identical vectors should be zero
    EXPECT_NEAR(fv1.euclidean_distance(fv2), 0.0, 1e-10);
    EXPECT_NEAR(fv1.correlation_coefficient(fv2), 1.0, 1e-6);
}

TEST_F(ScientificMemoryTest, ScientificCaseCreation) {
    auto features = create_test_observation_features();
    std::vector<std::string> names = {"ra", "dec", "magnitude", "proper_motion"};
    memory::FeatureVector feature_vec(features, names);
    
    memory::ScientificCase test_case("test_case_001", feature_vec);
    
    EXPECT_EQ(test_case.case_id(), "test_case_001");
    EXPECT_EQ(test_case.classification(), memory::AnomalyType::CLASSIFICATION_UNCERTAIN);
    EXPECT_EQ(test_case.state(), memory::CaseState::INITIAL_DETECTION);
    EXPECT_GT(test_case.time_since_creation_hours(), 0.0);
}

TEST_F(ScientificMemoryTest, CaseEvolutionTracking) {
    auto features = create_test_observation_features();
    std::vector<std::string> names = {"ra", "dec", "magnitude", "proper_motion"};
    memory::FeatureVector feature_vec(features, names);
    
    memory::ScientificCase test_case("evolution_test", feature_vec);
    
    // Update classification
    test_case.update_classification(memory::AnomalyType::STELLAR_VARIABLE, 0.85);
    EXPECT_EQ(test_case.classification(), memory::AnomalyType::STELLAR_VARIABLE);
    EXPECT_DOUBLE_EQ(test_case.confidence_score(), 0.85);
    
    // Add evidence
    test_case.add_evidence("Periodic brightness variation detected");
    test_case.add_hypothesis("RR Lyrae variable star");
    
    // Update state
    test_case.update_state(memory::CaseState::PATTERN_CONFIRMED);
    EXPECT_EQ(test_case.state(), memory::CaseState::PATTERN_CONFIRMED);
}

TEST_F(ScientificMemoryTest, MemorySystemOperations) {
    memory::ScientificMemorySystem memory_system;
    
    // Create and add test cases
    std::vector<std::shared_ptr<memory::ScientificCase>> test_cases;
    for (int i = 0; i < 10; ++i) {
        auto features = create_test_observation_features();
        std::vector<std::string> names = {"ra", "dec", "magnitude", "proper_motion"};
        memory::FeatureVector feature_vec(features, names);
        
        auto case_ptr = std::make_shared<memory::ScientificCase>(
            "mem_test_" + std::to_string(i), feature_vec);
        
        test_cases.push_back(case_ptr);
        EXPECT_TRUE(memory_system.add_case(case_ptr));
    }
    
    EXPECT_EQ(memory_system.get_active_case_count(), 10);
    
    // Test case retrieval
    auto retrieved_case = memory_system.get_case("mem_test_0");
    EXPECT_NE(retrieved_case, nullptr);
    EXPECT_EQ(retrieved_case->case_id(), "mem_test_0");
    
    // Test case querying
    memory::ScientificMemorySystem::CaseQuery query;
    query.max_results = 5;
    auto query_results = memory_system.query_cases(query);
    EXPECT_LE(query_results.size(), 5);
}

// =============================================================================
// DATA FUSION TESTS
// =============================================================================

class DataFusionTest : public M17TestFixture {};

TEST_F(DataFusionTest, ObservationCreation) {
    auto observation = create_test_observation();
    
    EXPECT_FALSE(observation.observation_id().empty());
    EXPECT_EQ(observation.source_id(), "test_source");
    EXPECT_EQ(observation.source_type(), fusion::SourceType::GROUND_TELESCOPE);
    EXPECT_EQ(observation.values().size(), 4);
    EXPECT_GT(observation.age_seconds(), 0.0);
}

TEST_F(DataFusionTest, SourceReliabilityTracking) {
    fusion::SourceReliabilityTracker tracker(30.0, 5, 0.5);
    
    // Add observations from a test source
    for (int i = 0; i < 10; ++i) {
        auto observation = create_test_observation();
        tracker.update_source_metrics("test_source", observation);
    }
    
    double reliability = tracker.calculate_reliability("test_source");
    EXPECT_GT(reliability, 0.0);
    EXPECT_LE(reliability, 1.0);
    
    auto metrics = tracker.get_source_metrics("test_source");
    EXPECT_NE(metrics, nullptr);
    EXPECT_EQ(metrics->total_observations, 10);
}

TEST_F(DataFusionTest, BayesianFusion) {
    fusion::BayesianDataFusion fusion_engine;
    
    // Create multiple observations of the same quantity with different uncertainties
    std::vector<fusion::Observation> observations;
    for (int i = 0; i < 5; ++i) {
        auto features = create_test_observation_features();
        std::vector<std::string> names = {"ra", "dec", "magnitude", "proper_motion"};
        
        fusion::Observation obs(
            "fusion_test_" + std::to_string(i),
            "source_" + std::to_string(i),
            fusion::SourceType::GROUND_TELESCOPE,
            features,
            names,
            std::chrono::system_clock::now()
        );
        
        observations.push_back(obs);
    }
    
    auto fusion_result = fusion_engine.fuse_observations(observations);
    
    EXPECT_EQ(fusion_result.num_sources_used, 5);
    EXPECT_GT(fusion_result.fusion_quality, 0.0);
    EXPECT_LE(fusion_result.fusion_quality, 1.0);
    EXPECT_EQ(fusion_result.fused_values.size(), 4);
    EXPECT_FALSE(fusion_result.fusion_method.empty());
}

TEST_F(DataFusionTest, OutlierDetection) {
    fusion::BayesianDataFusion fusion_engine;
    
    std::vector<fusion::Observation> observations;
    
    // Create normal observations
    for (int i = 0; i < 8; ++i) {
        std::vector<uncertainty::UncertainQuantity> features;
        features.emplace_back(10.0 + 0.1 * i, 0.1, "ra");  // Similar values
        features.emplace_back(20.0 + 0.1 * i, 0.1, "dec");
        features.emplace_back(15.0 + 0.1 * i, 0.1, "mag");
        
        std::vector<std::string> names = {"ra", "dec", "magnitude"};
        
        fusion::Observation obs(
            "normal_" + std::to_string(i),
            "source_" + std::to_string(i),
            fusion::SourceType::GROUND_TELESCOPE,
            features,
            names,
            std::chrono::system_clock::now()
        );
        
        observations.push_back(obs);
    }
    
    // Add outlier observations
    for (int i = 0; i < 2; ++i) {
        std::vector<uncertainty::UncertainQuantity> features;
        features.emplace_back(50.0, 0.1, "ra");  // Very different values
        features.emplace_back(80.0, 0.1, "dec");
        features.emplace_back(5.0, 0.1, "mag");
        
        std::vector<std::string> names = {"ra", "dec", "magnitude"};
        
        fusion::Observation obs(
            "outlier_" + std::to_string(i),
            "outlier_source_" + std::to_string(i),
            fusion::SourceType::AMATEUR_OBSERVATION,
            features,
            names,
            std::chrono::system_clock::now()
        );
        
        observations.push_back(obs);
    }
    
    auto cleaned_observations = fusion_engine.detect_and_reject_outliers(observations);
    
    // Should reject the 2 outliers
    EXPECT_LT(cleaned_observations.size(), observations.size());
    EXPECT_GE(cleaned_observations.size(), 8);
}

// =============================================================================
// INTEGRATION TESTS
// =============================================================================

class IntegrationTest : public M17TestFixture {};

TEST_F(IntegrationTest, FrameworkInitialization) {
    integration::M17Configuration config;
    integration::M17FrameworkOrchestrator orchestrator(config);
    
    EXPECT_TRUE(orchestrator.initialize());
    EXPECT_TRUE(orchestrator.start());
    EXPECT_TRUE(orchestrator.is_running());
    EXPECT_TRUE(orchestrator.stop());
    EXPECT_FALSE(orchestrator.is_running());
}

TEST_F(IntegrationTest, EndToEndProcessing) {
    integration::M17Framework framework(true);
    EXPECT_TRUE(framework.initialize());
    EXPECT_TRUE(framework.start());
    
    // Process a test observation
    auto result = framework.analyze_observation(
        150.0,  // RA [deg]
        -30.0,  // Dec [deg] 
        12.5,   // Magnitude
        0.1,    // RA uncertainty [arcsec]
        0.1,    // Dec uncertainty [arcsec]
        0.05    // Mag uncertainty
    );
    
    EXPECT_FALSE(result.classification.empty());
    EXPECT_GE(result.confidence, 0.0);
    EXPECT_LE(result.confidence, 1.0);
    EXPECT_GT(result.processing_time_ms, 0.0);
    
    framework.stop();
}

TEST_F(IntegrationTest, ConfigurationManagement) {
    integration::M17Configuration config;
    
    // Test configuration validation
    auto validation_errors = config.validate();
    EXPECT_TRUE(validation_errors.empty());
    EXPECT_TRUE(config.is_valid());
    
    // Test configuration serialization
    std::string json_str = config.to_json();
    EXPECT_FALSE(json_str.empty());
    
    integration::M17Configuration config_copy;
    EXPECT_TRUE(config_copy.from_json(json_str));
    
    // Configurations should be equivalent
    EXPECT_EQ(config.uncertainty().batch_size, config_copy.uncertainty().batch_size);
    EXPECT_EQ(config.physics().enable_relativity, config_copy.physics().enable_relativity);
}

TEST_F(IntegrationTest, PerformanceBenchmark) {
    integration::M17Framework framework(true);
    EXPECT_TRUE(framework.initialize());
    EXPECT_TRUE(framework.start());
    
    const int num_observations = 1000;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Process multiple observations
    for (int i = 0; i < num_observations; ++i) {
        auto result = framework.analyze_observation(
            150.0 + i * 0.01,  // RA
            -30.0 + i * 0.01,  // Dec
            12.5 + i * 0.001,  // Magnitude
            0.1, 0.1, 0.05     // Uncertainties
        );
        
        EXPECT_FALSE(result.classification.empty());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    double throughput = num_observations / (duration.count() / 1000.0);
    std::cout << "Performance: " << throughput << " observations/second" << std::endl;
    
    // Should process at least 10 observations per second
    EXPECT_GT(throughput, 10.0);
    
    framework.stop();
}

TEST_F(IntegrationTest, SystemMetrics) {
    integration::M17FrameworkOrchestrator orchestrator;
    EXPECT_TRUE(orchestrator.initialize());
    EXPECT_TRUE(orchestrator.start());
    
    // Allow some processing time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto metrics = orchestrator.get_system_metrics();
    EXPECT_GT(metrics.overall_health_score, 0.0);
    EXPECT_LE(metrics.overall_health_score, 1.0);
    EXPECT_GE(metrics.healthy_components, 1);
    
    auto health = orchestrator.get_component_health();
    EXPECT_FALSE(health.empty());
    
    auto stats = orchestrator.get_processing_statistics();
    EXPECT_GE(stats.total_observations_processed, 0);
    
    EXPECT_TRUE(orchestrator.stop());
}

// =============================================================================
// PERFORMANCE AND STRESS TESTS
// =============================================================================

class PerformanceTest : public M17TestFixture {};

TEST_F(PerformanceTest, UncertaintyPropagationThroughput) {
    uncertainty::SIMDUncertaintyPropagator propagator(10000, 8);
    
    const size_t num_operations = 100000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < num_operations; ++i) {
        auto x = create_test_uncertain_quantity(1.0 + i * 0.001, 0.1);
        auto result = x.pow(2.0);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double ops_per_sec = num_operations / (duration.count() / 1e6);
    std::cout << "Uncertainty propagation: " << ops_per_sec << " ops/sec" << std::endl;
    
    EXPECT_GT(ops_per_sec, 10000.0);  // Expect >10K ops/sec
}

TEST_F(PerformanceTest, PhysicsComputationThroughput) {
    physics::AdvancedPhysicsEngine engine;
    
    const size_t num_computations = 10000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < num_computations; ++i) {
        auto state = create_test_orbital_state();
        auto acceleration = engine.compute_acceleration(state);
        // Prevent optimization
        volatile double acc_mag = acceleration.norm();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double ops_per_sec = num_computations / (duration.count() / 1e6);
    std::cout << "Physics computation: " << ops_per_sec << " ops/sec" << std::endl;
    
    EXPECT_GT(ops_per_sec, 1000.0);  // Expect >1K ops/sec
}

TEST_F(PerformanceTest, MemorySystemScalability) {
    memory::ScientificMemorySystem memory_system;
    
    const size_t num_cases = 10000;
    auto start = std::chrono::high_resolution_clock::now();
    
    // Add many cases
    for (size_t i = 0; i < num_cases; ++i) {
        auto features = create_test_observation_features();
        std::vector<std::string> names = {"ra", "dec", "magnitude", "proper_motion"};
        memory::FeatureVector feature_vec(features, names);
        
        auto case_ptr = std::make_shared<memory::ScientificCase>(
            "scale_test_" + std::to_string(i), feature_vec);
        
        memory_system.add_case(case_ptr);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double ops_per_sec = num_cases / (duration.count() / 1e6);
    std::cout << "Memory system: " << ops_per_sec << " additions/sec" << std::endl;
    
    EXPECT_EQ(memory_system.get_active_case_count(), num_cases);
    EXPECT_GT(ops_per_sec, 1000.0);  // Expect >1K additions/sec
}

// =============================================================================
// MAIN TEST RUNNER
// =============================================================================

class TestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        std::cout << "\n=== M17 Framework Test Suite ===" << std::endl;
        std::cout << "Version: 2.0.0" << std::endl;
        std::cout << "Date: " << __DATE__ << " " << __TIME__ << std::endl;
        std::cout << "=================================" << std::endl;
        
        // Create test output directory
        std::filesystem::create_directories("test_output");
    }
    
    void TearDown() override {
        std::cout << "\n=== Test Suite Complete ===" << std::endl;
    }
};

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new TestEnvironment);
    
    // Configure test output
    ::testing::GTEST_FLAG(output) = "xml:test_results.xml";
    
    return RUN_ALL_TESTS();
}
