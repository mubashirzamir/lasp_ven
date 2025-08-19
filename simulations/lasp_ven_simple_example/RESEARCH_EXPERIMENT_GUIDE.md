# Research Experiment Guide

## Overview
This guide explains how to run comparative analysis experiments for different service placement strategies across various traffic scenarios.

## How OMNeT++ Configuration Works

### Config Name vs Run Number
- **Config Name**: Defines the overall simulation setup (traffic scenario, network topology)
- **Run Number**: Defines specific strategy parameters and experiment repetitions

### Fixed Configuration Structure
```
Config Name: [Traffic Scenario] + [Strategy]
Run Number: [Strategy Parameters] + [Repetition Number]
```

## Available Configurations

### Traffic Scenarios (Config Names)
1. **Baseline** - Default configuration
2. **LightTraffic** - Low vehicle spawn rate (period=15s, number=5)
3. **MediumTraffic** - Moderate vehicle spawn rate (period=8s, number=10)
4. **HeavyTraffic** - High vehicle spawn rate (period=3s, number=50)

### Strategy Options (Run Numbers)
1. **greedy** - Pure latency optimization
2. **threshold** - Load-aware with threshold filtering
3. **greedyLatencyAware** - Balanced load + latency optimization
4. **thresholdLatencyAware** - Advanced load balancing with latency awareness

## Running Comparative Analysis

### Method 1: Using Config Names (Recommended)
1. **Select Config**: Choose traffic scenario (e.g., "LightTraffic")
2. **Select Run**: Choose strategy (e.g., "greedy", "greedyLatencyAware")
3. **Run Simulation**: Execute and collect results
4. **Repeat**: Run all strategies for the same traffic scenario
5. **Compare**: Analyze results across strategies

### Method 2: Using Baseline Config
1. **Select Config**: "Baseline"
2. **Select Run**: Choose strategy from dropdown
3. **Run Simulation**: Execute and collect results
4. **Repeat**: Run all strategies
5. **Compare**: Analyze results

## Research Experiment Matrix

### Primary Research: Greedy vs GreedyLatencyAware

| Traffic Scenario | Strategy | Config Name | Run Number |
|------------------|----------|-------------|------------|
| Light Traffic | greedy | LightTraffic | greedy |
| Light Traffic | greedyLatencyAware | LightTraffic | greedyLatencyAware |
| Medium Traffic | greedy | MediumTraffic | greedy |
| Medium Traffic | greedyLatencyAware | MediumTraffic | greedyLatencyAware |
| Heavy Traffic | greedy | HeavyTraffic | greedy |
| Heavy Traffic | greedyLatencyAware | HeavyTraffic | greedyLatencyAware |

### Secondary Research: All Strategies

| Traffic Scenario | Strategy | Config Name | Run Number |
|------------------|----------|-------------|------------|
| Light Traffic | threshold | LightTraffic | threshold |
| Light Traffic | thresholdLatencyAware | LightTraffic | thresholdLatencyAware |
| Medium Traffic | threshold | MediumTraffic | threshold |
| Medium Traffic | thresholdLatencyAware | MediumTraffic | thresholdLatencyAware |
| Heavy Traffic | threshold | HeavyTraffic | threshold |
| Heavy Traffic | thresholdLatencyAware | HeavyTraffic | thresholdLatencyAware |

## Step-by-Step Experiment Process

### 1. Prepare Traffic Scenarios
Edit `straight.rou.xml` for each traffic scenario:
```xml
<!-- Light Traffic -->
<flow id="flow0" type="vtype0" route="route0" begin="0" period="15" number="5" arrivalPos="0" />

<!-- Medium Traffic -->
<flow id="flow0" type="vtype0" route="route0" begin="0" period="8" number="10" arrivalPos="0" />

<!-- Heavy Traffic -->
<flow id="flow0" type="vtype0" route="route0" begin="0" period="3" number="50" arrivalPos="0" />
```

### 2. Run Experiments
For each traffic scenario:
1. Update `straight.rou.xml` with appropriate period/number
2. Run simulation with each strategy
3. Collect `.sca` and `.vec` files
4. Document results

### 3. Analyze Results
Compare QoS metrics across strategies:
- **requestSuccessRate** (higher is better)
- **requestRejectionRate** (lower is better)
- **serviceCompletionTime** (lower is better)
- **loadBalancingEfficiency** (higher is better)
- **averageLatency** (lower is better)
- **serverUtilization** (moderate is best)

## Expected Results

### Greedy vs GreedyLatencyAware Comparison

| Metric | Greedy | GreedyLatencyAware | Expected Difference |
|--------|--------|-------------------|-------------------|
| averageLatency | Lower | Slightly higher | Greedy wins on pure latency |
| loadBalancingEfficiency | Lower | Higher | GreedyLatencyAware wins on load balancing |
| requestSuccessRate | Variable | Higher | GreedyLatencyAware more reliable |
| serverUtilization | Uneven | More even | GreedyLatencyAware better distribution |

### Traffic Scenario Impact
- **Light Traffic**: Minimal differences between strategies
- **Medium Traffic**: Moderate differences, good for comparison
- **Heavy Traffic**: Maximum differences, best for analysis

## Output Files

### Scalar Results (`.sca`)
```
LaspVenSimpleSimulation.laspManager.app[0].requestSuccessRate:mean = 0.95
LaspVenSimpleSimulation.laspManager.app[0].loadBalancingEfficiency:mean = 0.87
LaspVenSimpleSimulation.laspManager.app[0].serviceCompletionTime:mean = 0.045
```

### Vector Results (`.vec`)
```
LaspVenSimpleSimulation.laspManager.app[0].requestSuccessRate:vector = 0.9,0.92,0.95,0.98,1.0...
LaspVenSimpleSimulation.laspManager.app[0].loadBalancingEfficiency:vector = 0.85,0.87,0.89,0.91...
```

## Tips for Research

1. **Start with Light Traffic**: Easier to debug and understand behavior
2. **Use Medium Traffic for Primary Analysis**: Good balance of load and differences
3. **Use Heavy Traffic for Stress Testing**: Maximum load conditions
4. **Run Multiple Repetitions**: Use different run numbers for statistical significance
5. **Document Everything**: Keep track of which configuration produced which results

## Troubleshooting

### If strategies don't appear in Run Number:
- Check that the strategy parameter is properly defined in the INI file
- Ensure all strategies are listed in the `${strategy=...}` parameter
- Restart OMNeT++ after making changes

### If results seem inconsistent:
- Check that the correct traffic scenario is loaded in `straight.rou.xml`
- Verify that the simulation ran for the full duration
- Check the log files for any errors or warnings
