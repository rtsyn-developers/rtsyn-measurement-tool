#include <gtest/gtest.h>

extern "C" {
#include <rtsyn/measurement_tool.h>
}

class MeasurementToolTest : public ::testing::Test {
  protected:
    void
    SetUp() override
    {
        tool_ = rtsyn_measurement_tool_create();
        ASSERT_NE(tool_, nullptr);
    }

    void
    TearDown() override
    {
        rtsyn_measurement_tool_destroy(tool_);
        tool_ = nullptr;
    }

    rtsyn_measurement_tool_t *tool_ = nullptr;
};

TEST_F(MeasurementToolTest, CalculatesRuntimeMetricsFromProbeTimestamps)
{
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 7, RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_DEVICE_FIRST, 1000));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 7, RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_DEVICE_FIRST, 1100));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 7, RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_PLUGIN, 1200));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 7, RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_PLUGIN, 1500));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 7, RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_DEVICE_FINAL, 1600));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 7, RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_DEVICE_FINAL, 1900));

    rtsyn_measurement_metrics_t metrics = {};
    ASSERT_TRUE(rtsyn_measurement_tool_calculate_work(tool_, 1000, &metrics));

    EXPECT_EQ(metrics.cycle_id, 7U);
    EXPECT_EQ(metrics.period_ns, 1000U);
    EXPECT_EQ(metrics.actual_period_ns, 900U);
    EXPECT_EQ(metrics.latency_ns, 100U);
    EXPECT_FALSE(metrics.missed_cycle);
    EXPECT_EQ(metrics.devices_read_ns, 100U);
    EXPECT_EQ(metrics.plugins_time_ns, 300U);
    EXPECT_EQ(metrics.devices_write_ns, 300U);

    rtsyn_measurement_metrics_t latest = {};
    ASSERT_TRUE(rtsyn_measurement_tool_get_latest(tool_, &latest));
    EXPECT_EQ(latest.actual_period_ns, metrics.actual_period_ns);
}

TEST_F(MeasurementToolTest, FlagsMissedCycle)
{
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 1, RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_DEVICE_FIRST, 10));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 1, RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_DEVICE_FIRST, 20));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(tool_, 1,
                                                    RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_PLUGIN,
                                                    20));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 1, RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_PLUGIN, 30));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 1, RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_DEVICE_FINAL, 30));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 1, RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_DEVICE_FINAL, 80));

    rtsyn_measurement_metrics_t metrics = {};
    ASSERT_TRUE(rtsyn_measurement_tool_calculate_work(tool_, 50, &metrics));

    EXPECT_EQ(metrics.actual_period_ns, 70U);
    EXPECT_EQ(metrics.latency_ns, 20U);
    EXPECT_TRUE(metrics.missed_cycle);
}

TEST_F(MeasurementToolTest, CalculatesActualPeriodFromCycleBoundaries)
{
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 9, RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_DEVICE_FIRST, 1000));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 9, RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_DEVICE_FIRST, 1010));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 9, RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_PLUGIN, 1020));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 9, RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_PLUGIN, 1050));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 9, RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_DEVICE_FINAL, 1060));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 9, RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_DEVICE_FINAL, 1080));

    rtsyn_measurement_metrics_t metrics = {};
    ASSERT_TRUE(rtsyn_measurement_tool_calculate_cycle(tool_, 1000, 1000, 1995, &metrics));

    EXPECT_EQ(metrics.cycle_id, 9U);
    EXPECT_EQ(metrics.actual_period_ns, 995U);
    EXPECT_EQ(metrics.latency_ns, 5U);
    EXPECT_FALSE(metrics.missed_cycle);
    EXPECT_EQ(metrics.devices_read_ns, 10U);
    EXPECT_EQ(metrics.plugins_time_ns, 30U);
    EXPECT_EQ(metrics.devices_write_ns, 20U);
}

TEST_F(MeasurementToolTest, RejectsIncompleteSamples)
{
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 1, RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_DEVICE_FIRST, 10));

    rtsyn_measurement_metrics_t metrics = {};
    EXPECT_FALSE(rtsyn_measurement_tool_calculate_work(tool_, 50, &metrics));
}

TEST_F(MeasurementToolTest, RejectsOutOfOrderProbePairs)
{
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 1, RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_DEVICE_FIRST, 10));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 1, RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_DEVICE_FIRST, 20));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 1, RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_PLUGIN, 30));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 1, RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_PLUGIN, 25));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 1, RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_DEVICE_FINAL, 40));
    ASSERT_TRUE(rtsyn_measurement_tool_record_probe(
        tool_, 1, RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_DEVICE_FINAL, 50));

    rtsyn_measurement_metrics_t metrics = {};
    EXPECT_FALSE(rtsyn_measurement_tool_calculate_work(tool_, 50, &metrics));
    EXPECT_FALSE(rtsyn_measurement_tool_calculate_cycle(tool_, 50, 10, 60, &metrics));
}

TEST(MeasurementDeadlineTest, DistinguishesToleranceAndSkippedPeriods)
{
    rtsyn_measurement_metrics_t metrics = {};

    ASSERT_TRUE(rtsyn_measurement_tool_calculate_deadline(100, 95, 100, 10, &metrics));
    EXPECT_EQ(metrics.wake_lateness_ns, 0U);
    EXPECT_FALSE(metrics.deadline_missed);
    EXPECT_EQ(metrics.skipped_cycle_count, 0U);

    ASSERT_TRUE(rtsyn_measurement_tool_calculate_deadline(100, 110, 100, 10, &metrics));
    EXPECT_EQ(metrics.wake_lateness_ns, 10U);
    EXPECT_FALSE(metrics.deadline_missed);
    EXPECT_EQ(metrics.skipped_cycle_count, 0U);

    ASSERT_TRUE(rtsyn_measurement_tool_calculate_deadline(100, 301, 100, 10, &metrics));
    EXPECT_EQ(metrics.wake_lateness_ns, 201U);
    EXPECT_TRUE(metrics.deadline_missed);
    EXPECT_EQ(metrics.skipped_cycle_count, 2U);
    EXPECT_EQ(metrics.latency_ns, metrics.wake_lateness_ns);
    EXPECT_EQ(metrics.missed_cycle, metrics.deadline_missed);
}
