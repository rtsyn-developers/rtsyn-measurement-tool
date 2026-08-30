/**
 * @file rtsyn/internal/measurement_tool.h
 * @brief Private RTSyn measurement tool state.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef RTSYN_INTERNAL_MEASUREMENT_TOOL_H
#define RTSYN_INTERNAL_MEASUREMENT_TOOL_H

#include <rtsyn/measurement_tool.h>

struct rtsyn_measurement_tool_s {
    uint64_t cycle_id;
    uint64_t timestamps[RTSYN_MEASUREMENT_PROBE_STAGE_COUNT];
    bool present[RTSYN_MEASUREMENT_PROBE_STAGE_COUNT];
    bool has_latest;
    rtsyn_measurement_metrics_t latest;
};

#endif /* RTSYN_INTERNAL_MEASUREMENT_TOOL_H */
