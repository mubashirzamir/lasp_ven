# LASP VEN - Simplified Project Structure

This document describes the simplified version of the LASP VEN project that uses existing OMNeT++/INET/Veins modules to reduce complexity while maintaining the core objective of evaluating service placement strategies in Vehicular Edge Networks (VENs).

## Project Structure

```
lasp_ven/
├── src/
│   ├── lasp_ven_simple/           # Simplified source code
│   │   ├── LASPManager.h/.cc/.ned # Service placement manager
│   │   ├── TestApp.h/.cc/.ned     # Test application
│   │   ├── strategies/            # Placement strategies
│   │   │   ├── ThresholdStrategy.h/.cc
│   │   │   └── GreedyStrategy.h/.cc
│   │   └── utils/                 # Utility functions
│   │       └── ServicePlacementUtils.h/.cc
│   └── veins_inet/               # Copied from Veins project
├── simulations/
│   └── lasp_ven_simple_example/  # Simplified simulation
│       ├── lasp_ven_simple.ini   # Configuration
│       ├── lasp_ven_simple.ned   # Network definition
│       └── sumo/                 # SUMO traffic files
└── README_SIMPLE.md              # This file
```

## Key Simplifications

### 1. **Uses Existing Modules**
- **Vehicles**: Uses `org.car2x.veinsinets.VeinsInetCar` from veins_inet
- **RSUs**: Uses `org.car2x.veinsinets.VeinsInetRSU` from veins_inet
- **Network Stack**: Leverages existing INET networking modules
- **Mobility**: Uses existing VeinsInetMobility
- **Application Base**: Uses `inet::ApplicationBase` for fixed infrastructure applications

### 2. **Custom Components Only**
- **LASPManager**: Custom application for service placement logic
- **Strategies**: Custom C++ classes for placement algorithms
- **TestApp**: Simple test application to generate service requests

### 3. **Removed Complexity**
- No custom vehicle/RSU modules
- No complex networking code
- Simplified UDP communication
- Focus on core service placement logic

## Service Placement Strategies

### Threshold Strategy
- Places services only on servers with load below a threshold
- Considers latency, load, and priority
- Conservative approach to prevent overloading

### Greedy Strategy
- Places services on the server with lowest estimated latency
- Simple and fast placement decision
- May lead to uneven load distribution

## Usage

### Building
```bash
# Build the simplified project
make
```

### Running Simulation
```bash
# Run with threshold strategy
./lasp-ven -u Cmdenv -c Baseline -r 0 simulations/lasp_ven_simple_example/lasp_ven_simple.ini

# Run with greedy strategy
./lasp-ven -u Cmdenv -c CompareStrategies -r 0 simulations/lasp_ven_simple_example/lasp_ven_simple.ini
```

### Configuration Parameters
- `strategy`: "threshold" or "greedy"
- `loadThreshold`: Maximum server utilization (0.0-1.0)
- `numEdgeServers`: Number of edge servers to deploy
- `evaluationInterval`: How often to evaluate placements

## Statistics Collected
- `requestsReceived`: Number of service requests received
- `requestsServed`: Number of requests successfully placed
- `averageLatency`: Average service placement latency
- `serverUtilization`: Average server utilization
- `requestsSent`: Number of test requests sent
- `responsesReceived`: Number of responses received

## Advantages of Simplified Structure

1. **Reduced Complexity**: Fewer custom modules to maintain
2. **Better Compatibility**: Uses well-tested existing modules
3. **Easier Debugging**: Less custom code means fewer bugs
4. **Faster Development**: Focus on strategy logic, not infrastructure
5. **Better Performance**: Leverages optimized existing modules
6. **Proper Base Class**: Uses `inet::ApplicationBase` for fixed infrastructure

## Extending the Project

### Adding New Strategies
1. Create new strategy class in `src/lasp_ven_simple/strategies/`
2. Implement `placeService()` method
3. Add strategy to `LASPManager::findBestPlacement()`
4. Update configuration parameters

### Adding New Metrics
1. Add signals in `LASPManager::initialize()`
2. Emit signals in relevant methods
3. Configure recording in `.ini` file

### Integrating with Real Vehicles
1. Modify `TestApp` to interface with vehicle modules
2. Use OMNeT++ signals for communication
3. Add vehicle-specific parameters

## Comparison with Original

| Aspect | Original | Simplified |
|--------|----------|------------|
| Custom Vehicle Modules | Yes | No (uses veins_inet) |
| Custom RSU Modules | Yes | No (uses veins_inet) |
| Complex Networking | Yes | No (uses INET) |
| Strategy Implementation | Complex | Simple C++ classes |
| Application Base | Custom | inet::ApplicationBase |
| Maintenance Overhead | High | Low |
| Development Speed | Slow | Fast |
| Compatibility | Custom | Standard |

This simplified structure maintains the core objective of evaluating service placement strategies while significantly reducing complexity and potential for errors. The use of `inet::ApplicationBase` ensures proper integration for fixed infrastructure applications. 