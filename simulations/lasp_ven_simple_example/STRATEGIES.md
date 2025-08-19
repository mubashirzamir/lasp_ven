# Service Placement Strategies

## Overview
This document describes all available service placement strategies in the LASP VEN simulation, including both original and latency-aware variants.

## Available Strategies

### 1. GreedyStrategy (Original)
**Description**: Simple strategy that selects the server with the lowest latency.

**How it works**:
- Evaluates all available servers
- Calculates latency for each server using `ServicePlacementUtils::estimateLatency()`
- Selects the server with the lowest latency
- Ignores server load completely

**Parameters**: None (uses default latency calculation)

**Use case**: When latency is the only concern and server load balancing is not important.

**Complexity**: Simple

### 2. ThresholdStrategy (Original)
**Description**: Load-aware strategy that filters servers by load threshold, then selects based on a scoring system.

**How it works**:
- Filters servers to only those under the load threshold
- Calculates a placement score considering latency, load penalty, and priority bonus
- Selects the server with the lowest score
- Falls back to greedy approach if no servers are under threshold

**Parameters**:
- `loadThreshold`: Maximum allowed server utilization (default: 0.8 = 80%)

**Use case**: When load balancing is important and you want to avoid overloading servers.

**Complexity**: Medium

### 3. GreedyLatencyAwareStrategy (New)
**Description**: Combines server load and network latency using weighted scoring to select the best server.

**How it works**:
- Evaluates all available servers
- Calculates a combined score: `(loadWeight × normalizedLoad) + (latencyWeight × normalizedLatency)`
- Selects the server with the lowest combined score
- Normalizes both metrics to 0-1 range for fair comparison

**Parameters**:
- `loadWeight`: Weight for server load consideration (default: 0.5)
- `latencyWeight`: Weight for latency consideration (default: 0.5)

**Use case**: When you want to balance both load and latency equally.

**Complexity**: Medium

### 4. ThresholdLatencyAwareStrategy (New)
**Description**: First filters servers by load threshold, then selects the best one based on combined load and latency metrics.

**How it works**:
1. **First pass**: Collects all servers under the load threshold
2. **Second pass**: Among eligible servers, selects the one with best combined score
3. **Fallback**: If no servers are under threshold, falls back to greedy approach with all servers

**Parameters**:
- `loadThreshold`: Maximum allowed server utilization (default: 0.8 = 80%)
- `loadWeight`: Weight for server load consideration (default: 0.5)
- `latencyWeight`: Weight for latency consideration (default: 0.5)

**Use case**: When you want to maintain load balance while considering latency.

**Complexity**: High

## Strategy Comparison

| Strategy | Load Consideration | Latency Consideration | Complexity | Best For |
|----------|-------------------|----------------------|------------|----------|
| `greedy` | ❌ | ✅ | Simple | Pure latency optimization |
| `threshold` | ✅ | ❌ | Medium | Load balancing |
| `greedyLatencyAware` | ✅ | ✅ | Medium | Balanced optimization |
| `thresholdLatencyAware` | ✅ | ✅ | High | Sophisticated load balancing with latency awareness |

## Configuration

### In OMNeT++ Configuration
```ini
# Original Strategies
[Config GreedyStrategy]
description = "Greedy-based service placement strategy"
extends = Baseline
*.laspManager.app[0].strategy = "greedy"

[Config ThresholdStrategy]
description = "Threshold-based service placement strategy"
extends = Baseline
*.laspManager.app[0].strategy = "threshold"
*.laspManager.app[0].loadThreshold = 0.8

# New Latency-Aware Strategies
[Config GreedyLatencyAwareStrategy]
description = "Greedy latency-aware service placement strategy"
extends = Baseline
*.laspManager.app[0].strategy = "greedyLatencyAware"
*.laspManager.app[0].loadWeight = 0.5
*.laspManager.app[0].latencyWeight = 0.5

[Config ThresholdLatencyAwareStrategy]
description = "Threshold latency-aware service placement strategy"
extends = Baseline
*.laspManager.app[0].strategy = "thresholdLatencyAware"
*.laspManager.app[0].loadThreshold = 0.8
*.laspManager.app[0].loadWeight = 0.5
*.laspManager.app[0].latencyWeight = 0.5
```

### Weight Tuning Examples (for Latency-Aware Strategies)

**Latency-focused** (prioritize low latency):
```ini
*.laspManager.app[0].loadWeight = 0.3
*.laspManager.app[0].latencyWeight = 0.7
```

**Load-focused** (prioritize load balancing):
```ini
*.laspManager.app[0].loadWeight = 0.7
*.laspManager.app[0].latencyWeight = 0.3
```

**Balanced** (equal consideration):
```ini
*.laspManager.app[0].loadWeight = 0.5
*.laspManager.app[0].latencyWeight = 0.5
```

## Latency Calculation
All strategies use the `ServicePlacementUtils::estimateLatency()` method:
- **Propagation delay**: Based on distance between vehicle and server
- **Processing delay**: Based on request size and server compute capacity
- **Queueing delay**: Based on current server load

## Research Applications

### Primary Research Focus: Greedy vs GreedyLatencyAware
- **`greedy`**: Baseline strategy for pure latency optimization
- **`greedyLatencyAware`**: Enhanced strategy balancing both load and latency
- **QoS comparison**: Using metrics like success rate, completion time, and load balancing efficiency
- **Performance analysis**: Understanding the trade-off between latency optimization and load balancing

### Secondary Strategies
- **`threshold`**: Load balancing baseline (bonus for research)
- **`thresholdLatencyAware`**: Advanced load balancing with latency awareness (bonus for research)

## Strategy Selection Guide

### Choose `greedy` when:
- You need the simplest possible strategy
- Latency is the only performance metric
- Server load is not a concern

### Choose `threshold` when:
- You need load balancing
- You want to avoid server overload
- Latency is not the primary concern

### Choose `greedyLatencyAware` when:
- You want to balance both load and latency
- You need configurable weights
- You want a single-pass evaluation

### Choose `thresholdLatencyAware` when:
- You need sophisticated load balancing
- You want latency awareness within load constraints
- You need fallback mechanisms

## Debugging
All strategies include detailed WARN statements:
- `[FLOW-X]` for general flow tracking
- `[LATENCY-AWARE-GREEDY]` for GreedyLatencyAwareStrategy
- `[LATENCY-AWARE-THRESHOLD]` for ThresholdLatencyAwareStrategy

These logs show the decision-making process and help understand strategy behavior.

## QoS Metrics for Strategy Comparison

### Available Metrics
- **`requestSuccessRate`**: Percentage of requests successfully placed (higher is better)
- **`requestRejectionRate`**: Count of requests that couldn't be placed (lower is better)
- **`serviceCompletionTime`**: End-to-end service delivery time in seconds (lower is better)
- **`loadBalancingEfficiency`**: How evenly load is distributed across servers (higher is better)
- **`averageLatency`**: Average estimated latency from placement decisions (lower is better)
- **`serverUtilization`**: Average server utilization across all edge servers (moderate is best)

### Research Comparison Focus
For comparing `greedy` vs `greedyLatencyAware`:
1. **Latency Performance**: Compare `averageLatency` and `serviceCompletionTime`
2. **Load Balancing**: Compare `loadBalancingEfficiency` and `serverUtilization`
3. **Overall QoS**: Compare `requestSuccessRate` and `requestRejectionRate`

## Performance Considerations

### Computational Complexity
- **`greedy`**: O(n) - single pass through servers
- **`threshold`**: O(n) - single pass with scoring
- **`greedyLatencyAware`**: O(n) - single pass with combined scoring
- **`thresholdLatencyAware`**: O(2n) - two passes (filtering + scoring)

### Memory Usage
- **`greedy`**: Minimal - no additional storage
- **`threshold`**: Minimal - no additional storage
- **`greedyLatencyAware`**: Minimal - no additional storage
- **`thresholdLatencyAware`**: O(n) - stores eligible servers list
