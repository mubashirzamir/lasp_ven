# Vehicle Traffic Control for VEN Experiments

## Current Configuration

The simulation uses a **flow-based** approach for vehicle spawning, which allows easy control of vehicle density.

### Current Settings (Baseline):
- **Spawn Period**: 5 seconds (one vehicle every 5 seconds)
- **Simulation Duration**: 300 seconds
- **Total Vehicles**: ~60 vehicles (300s ÷ 5s)
- **Route**: Counter-clockwise square (A0→B0→B1→A1→A0)
- **Route Time**: ~25 seconds per complete circuit

## How to Modify Vehicle Density

### Option 1: Edit `straight.rou.xml`
```xml
<!-- Current: 1 vehicle every 5 seconds -->
<flow id="flow0" type="vtype0" route="route0" begin="0" period="5" end="300" arrivalPos="0" />

<!-- Low density: 1 vehicle every 15 seconds -->
<flow id="flow0" type="vtype0" route="route0" begin="0" period="15" end="300" arrivalPos="0" />

<!-- High density: 1 vehicle every 2 seconds -->
<flow id="flow0" type="vtype0" route="route0" begin="0" period="2" end="300" arrivalPos="0" />

<!-- Very high density: 1 vehicle every 1 second -->
<flow id="flow0" type="vtype0" route="route0" begin="0" period="1" end="300" arrivalPos="0" />
```

### Option 2: Use Pre-configured Scenarios
The INI file includes several pre-configured scenarios:

- **Baseline**: Default settings
- **LowDensity**: Low vehicle density
- **MediumDensity**: Medium vehicle density  
- **HighDensity**: High vehicle density

## Vehicle Density Calculations

| Spawn Period | Total Vehicles | Average Concurrent | Description |
|--------------|----------------|-------------------|-------------|
| 1s | 300 | ~25 | Very high density |
| 2s | 150 | ~12 | High density |
| 5s | 60 | ~5 | Medium density (baseline) |
| 8s | 37 | ~3 | Low-medium density |
| 15s | 20 | ~2 | Low density |

*Note: Average concurrent vehicles = Route time (25s) ÷ Spawn period*

## Edge Server Positioning

Edge servers are positioned around the 100m × 100m road network:
- **3 servers**: At corners and midpoints
- **5 servers**: Extended coverage
- **8 servers**: Complete surrounding coverage

All servers are within 120m of the road network center for optimal communication.

## Experiment Recommendations

### For Service Placement Strategy Testing:
1. **Start with Baseline** (5s spawn) for initial testing
2. **Use LowDensity** (15s spawn) for stress testing strategies
3. **Use HighDensity** (2s spawn) for load testing

### For Scalability Testing:
1. **Vary edge server count**: 3, 5, 8 servers
2. **Vary vehicle density**: Low, Medium, High
3. **Compare strategies**: Threshold vs Greedy

### For Latency Analysis:
1. **Measure end-to-end latency** from vehicle request to response
2. **Compare different traffic loads** on same infrastructure
3. **Analyze edge server utilization** under different conditions 