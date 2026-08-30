/**
 * @file rtsyn/measurement_tool.h
 * @author Sergio Hidalgo (sergiohg.dev@gmail.com)
 * @brief Runtime timing metric calculator for RTSyn.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef RTSYN_MEASUREMENT_TOOL_H
#define RTSYN_MEASUREMENT_TOOL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum rtsyn_measurement_probe_stage_e : uint8_t {
    RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_DEVICE_FIRST = 0,
    RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_DEVICE_FIRST,
    RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_PLUGIN,
    RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_PLUGIN,
    RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_DEVICE_FINAL,
    RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_DEVICE_FINAL,
    RTSYN_MEASUREMENT_PROBE_STAGE_COUNT,
} rtsyn_measurement_probe_stage_t;

typedef struct rtsyn_measurement_tool_s rtsyn_measurement_tool_t;

typedef struct rtsyn_measurement_metrics_e {
    uint64_t cycle_id;
    uint64_t period_ns;
    uint64_t actual_period_ns;
    uint64_t latency_ns;
    uint64_t wake_lateness_ns;
    uint64_t skipped_cycle_count;
    bool missed_cycle;
    bool deadline_missed;
    uint64_t devices_read_ns;
    uint64_t plugins_time_ns;
    uint64_t devices_write_ns;
} rtsyn_measurement_metrics_t;

rtsyn_measurement_tool_t *
rtsyn_measurement_tool_create(void);

void
rtsyn_measurement_tool_destroy(rtsyn_measurement_tool_t *tool);

void
rtsyn_measurement_tool_reset(rtsyn_measurement_tool_t *tool);

bool
rtsyn_measurement_tool_record_probe(rtsyn_measurement_tool_t *tool, uint64_t cycle_id,
                                    rtsyn_measurement_probe_stage_t stage,
                                    uint64_t timestamp_ns);

/**
 * @brief Calculate metrics using only the measured runtime work window.
 *
 * `actual_period_ns` is computed from `BEFORE_DEVICE_FIRST` to
 * `AFTER_DEVICE_FINAL`. This excludes engine sleep time.
 */
bool
rtsyn_measurement_tool_calculate_work(rtsyn_measurement_tool_t *tool, uint64_t period_ns,
                                      rtsyn_measurement_metrics_t *out_metrics);

/**
 * @brief Calculate metrics using explicit full-cycle boundaries.
 *
 * `actual_period_ns` is computed as `cycle_finished_at_ns - cycle_started_at_ns`.
 * Use this from the engine when the desired period measurement includes sleep.
 */
bool
rtsyn_measurement_tool_calculate_cycle(rtsyn_measurement_tool_t *tool, uint64_t period_ns,
                                       uint64_t cycle_started_at_ns,
                                       uint64_t cycle_finished_at_ns,
                                       rtsyn_measurement_metrics_t *out_metrics);

/**
 * @brief Add scheduled-deadline metrics to an existing measurement.
 *
 * Wake lateness is zero for an early/on-time wake. `deadline_missed` becomes
 * true only when lateness is greater than `tolerance_ns`, while
 * `skipped_cycle_count` counts complete periods of lateness.
 */
bool
rtsyn_measurement_tool_calculate_deadline(uint64_t period_ns, uint64_t wake_time_ns,
                                          uint64_t scheduled_deadline_ns,
                                          uint64_t tolerance_ns,
                                          rtsyn_measurement_metrics_t *metrics);

bool
rtsyn_measurement_tool_get_latest(const rtsyn_measurement_tool_t *tool,
                                  rtsyn_measurement_metrics_t *out_metrics);

#ifdef __cplusplus
}
#endif

#endif /* RTSYN_MEASUREMENT_TOOL_H */
