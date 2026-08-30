/**
 * @file measurement_tool.c
 * @brief Runtime timing metric calculator implementation.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <stdlib.h>
#include <string.h>

#include "rtsyn/internal/measurement_tool.h"

static uint64_t
rtsyn_measurement_abs_diff(uint64_t left, uint64_t right)
{
    return left > right ? left - right : right - left;
}

static bool
rtsyn_measurement_probe_pair_is_ordered(const rtsyn_measurement_tool_t *tool,
                                        rtsyn_measurement_probe_stage_t before,
                                        rtsyn_measurement_probe_stage_t after)
{
    return tool->timestamps[after] >= tool->timestamps[before];
}

static bool
rtsyn_measurement_probe_order_is_valid(const rtsyn_measurement_tool_t *tool)
{
    return rtsyn_measurement_probe_pair_is_ordered(
               tool, RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_DEVICE_FIRST,
               RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_DEVICE_FIRST)
           && rtsyn_measurement_probe_pair_is_ordered(
               tool, RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_PLUGIN,
               RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_PLUGIN)
           && rtsyn_measurement_probe_pair_is_ordered(
               tool, RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_DEVICE_FINAL,
               RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_DEVICE_FINAL)
           && rtsyn_measurement_probe_pair_is_ordered(
               tool, RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_DEVICE_FIRST,
               RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_PLUGIN)
           && rtsyn_measurement_probe_pair_is_ordered(
               tool, RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_PLUGIN,
               RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_DEVICE_FINAL);
}

rtsyn_measurement_tool_t *
rtsyn_measurement_tool_create(void)
{
    rtsyn_measurement_tool_t *tool =
        (rtsyn_measurement_tool_t *)malloc(sizeof(rtsyn_measurement_tool_t));
    if (!tool)
    {
        return NULL;
    }

    rtsyn_measurement_tool_reset(tool);
    return tool;
}

void
rtsyn_measurement_tool_destroy(rtsyn_measurement_tool_t *tool)
{
    free(tool);
}

void
rtsyn_measurement_tool_reset(rtsyn_measurement_tool_t *tool)
{
    if (!tool)
    {
        return;
    }

    memset(tool, 0, sizeof(*tool));
}

bool
rtsyn_measurement_tool_record_probe(rtsyn_measurement_tool_t *tool, uint64_t cycle_id,
                                    rtsyn_measurement_probe_stage_t stage,
                                    uint64_t timestamp_ns)
{
    if (!tool || stage >= RTSYN_MEASUREMENT_PROBE_STAGE_COUNT || timestamp_ns == 0)
    {
        return false;
    }

    if (tool->cycle_id != cycle_id)
    {
        memset(tool->timestamps, 0, sizeof(tool->timestamps));
        memset(tool->present, 0, sizeof(tool->present));
        tool->cycle_id = cycle_id;
    }

    tool->timestamps[stage] = timestamp_ns;
    tool->present[stage] = true;
    return true;
}

bool
rtsyn_measurement_tool_calculate_cycle(rtsyn_measurement_tool_t *tool, uint64_t period_ns,
                                       uint64_t cycle_started_at_ns,
                                       uint64_t cycle_finished_at_ns,
                                       rtsyn_measurement_metrics_t *out_metrics)
{
    if (!tool || !out_metrics || period_ns == 0 || cycle_finished_at_ns < cycle_started_at_ns)
    {
        return false;
    }

    for (uint32_t i = 0; i < RTSYN_MEASUREMENT_PROBE_STAGE_COUNT; i++)
    {
        if (!tool->present[i])
        {
            return false;
        }
    }

    if (!rtsyn_measurement_probe_order_is_valid(tool))
    {
        return false;
    }

    rtsyn_measurement_metrics_t metrics = {
        .cycle_id = tool->cycle_id,
        .period_ns = period_ns,
        .actual_period_ns = cycle_finished_at_ns - cycle_started_at_ns,
    };
    metrics.latency_ns = rtsyn_measurement_abs_diff(period_ns, metrics.actual_period_ns);
    metrics.missed_cycle = metrics.actual_period_ns > period_ns;
    metrics.devices_read_ns =
        tool->timestamps[RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_DEVICE_FIRST]
        - tool->timestamps[RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_DEVICE_FIRST];
    metrics.plugins_time_ns =
        tool->timestamps[RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_PLUGIN]
        - tool->timestamps[RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_PLUGIN];
    metrics.devices_write_ns =
        tool->timestamps[RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_DEVICE_FINAL]
        - tool->timestamps[RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_DEVICE_FINAL];

    tool->latest = metrics;
    tool->has_latest = true;
    *out_metrics = metrics;
    return true;
}

bool
rtsyn_measurement_tool_calculate_work(rtsyn_measurement_tool_t *tool, uint64_t period_ns,
                                      rtsyn_measurement_metrics_t *out_metrics)
{
    if (!tool || !out_metrics)
    {
        return false;
    }

    return rtsyn_measurement_tool_calculate_cycle(
        tool, period_ns, tool->timestamps[RTSYN_MEASUREMENT_PROBE_STAGE_BEFORE_DEVICE_FIRST],
        tool->timestamps[RTSYN_MEASUREMENT_PROBE_STAGE_AFTER_DEVICE_FINAL], out_metrics);
}

bool
rtsyn_measurement_tool_calculate_deadline(uint64_t period_ns, uint64_t wake_time_ns,
                                          uint64_t scheduled_deadline_ns,
                                          uint64_t tolerance_ns,
                                          rtsyn_measurement_metrics_t *metrics)
{
    if (!metrics || period_ns == 0)
    {
        return false;
    }

    metrics->wake_lateness_ns = wake_time_ns > scheduled_deadline_ns
                                    ? wake_time_ns - scheduled_deadline_ns
                                    : 0;
    metrics->deadline_missed = metrics->wake_lateness_ns > tolerance_ns;
    metrics->skipped_cycle_count = metrics->wake_lateness_ns / period_ns;

    /* Compatibility aliases for existing telemetry consumers. */
    metrics->latency_ns = metrics->wake_lateness_ns;
    metrics->missed_cycle = metrics->deadline_missed;
    return true;
}

bool
rtsyn_measurement_tool_get_latest(const rtsyn_measurement_tool_t *tool,
                                  rtsyn_measurement_metrics_t *out_metrics)
{
    if (!tool || !out_metrics || !tool->has_latest)
    {
        return false;
    }

    *out_metrics = tool->latest;
    return true;
}
