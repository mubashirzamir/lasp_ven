# Latency-Aware Service Placement Strategies in Vehicular Edge Networks (LASP VEN)

This repository contains the implementation and experimental framework for research on latency-aware service placement strategies in vehicular edge networks. The project provides a configurable simulation environment using OMNeT++/INET/Veins with SUMO integration for realistic vehicular mobility modeling.

## Overview

This research project implements and evaluates two service placement strategies for vehicular edge networks:

1. **Greedy Strategy**: Pure latency optimization approach
2. **Greedy-LASP**: Enhanced latency-aware strategy with load balancing

The framework supports configurable traffic scenarios, strategy parameters, and comprehensive QoS metrics evaluation.

### Required Software
1. **OMNeT++ 5.6.2** - Discrete event simulation platform
2. **INET Framework 4.2.2** - Network simulation framework
3. **Veins 5.2** - Vehicular network simulation middleware
4. **SUMO 1.8.0** - Traffic simulation suite

## 📁 Project Structure

```
lasp_ven/
├── src/
│   ├── lasp_ven_simple/           # Main simulation modules
│   │   ├── LASPManager.cc         # Central placement controller
│   │   ├── VehicleServiceApp.cc   # Vehicle-side application
│   │   ├── EdgeServerApp.cc       # Server-side application
│   │   └── strategies/            # Service placement strategies
│   │       ├── GreedyStrategy.cc
│   │       └── GreedyLatencyAwareStrategy.cc
│   └── veins_inet/               # Veins-INET integration
├── simulations/
│   └── lasp_ven_simple_example/  # Main simulation scenario
│       ├── lasp_ven_simple.ini   # Configuration file
│       ├── lasp_ven_simple.ned   # Network topology
│       └── results/              # Simulation results
└── README.md                     # This file
```

## Running Experiments

### Basic Experiment Execution

#### Method 1: OMNeT++ IDE
1. Open OMNeT++ IDE
2. Import the project: `File → Import → Existing Projects into Workspace`
3. Select the `lasp_ven` directory
4. Right-click on `lasp_ven_simple.ini` → `Run As → OMNeT++ Simulation`

## Configuration Options

### Main Configuration File: `lasp_ven_simple.ini`

#### Network Topology Configuration
```ini
# Playground size (must match SUMO road network)
*.playgroundSizeX = 100m
*.playgroundSizeY = 100m
*.playgroundSizeZ = 50m

# Number of edge servers
*.numEdgeServers = 4

# Edge server positioning
*.edgeServer[0].mobility.initialX = 10m
*.edgeServer[0].mobility.initialY = 10m
*.edgeServer[1].mobility.initialX = 90m
*.edgeServer[1].mobility.initialY = 10m
# ... (positions for all 4 servers)
```

#### Strategy Configuration
```ini
# Strategy selection
*.laspManager.app[0].strategy = ${strategy="greedy","greedyLatencyAware"}

# Load threshold (for threshold-based strategies)
*.laspManager.app[0].loadThreshold = 0.8

# Weight configuration for Greedy-LASP
*.laspManager.app[0].loadWeight = ${loadWeight=0.5}
*.laspManager.app[0].latencyWeight = ${latencyWeight=0.5}
```

#### Server Configuration
```ini
# Server capacity
*.edgeServer[*].app[0].computeCapacity = 100GFLOPS
*.edgeServer[*].app[0].storageCapacity = 1000GB

# Server ports (must be unique)
*.edgeServer[0].app[0].localPort = 8000
*.edgeServer[1].app[0].localPort = 8001
*.edgeServer[2].app[0].localPort = 8002
*.edgeServer[3].app[0].localPort = 8003
```

#### Vehicle Configuration
```ini
# Service request parameters
*.vehicle[*].app[0].serviceRequestInterval = 5s
*.vehicle[*].app[0].maxRequests = 8
*.vehicle[*].app[0].requestSize = 500B
```

## Traffic Scenarios

### Pre-configured Scenarios

#### Light Traffic Scenario
- **Vehicles**: 5
- **Spawn Period**: 15 seconds
- **Characteristics**: Low-density urban traffic
- **Use Case**: Early morning/late night conditions

#### Medium Traffic Scenario
- **Vehicles**: 10
- **Spawn Period**: 8 seconds
- **Characteristics**: Moderate urban traffic
- **Use Case**: Typical daytime conditions

#### Heavy Traffic Scenario
- **Vehicles**: 50
- **Spawn Period**: 3 seconds
- **Characteristics**: High-density traffic
- **Use Case**: Peak hour or special event conditions

### Custom Traffic Scenarios

To create custom traffic scenarios, modify the SUMO configuration files:

#### Vehicle Flow Configuration (`straight.rou.xml`)
```xml
<flow id="vehicleFlow" begin="0" end="200" vehsPerHour="20" route="route_0">
    <vType id="car" accel="2.6" decel="4.5" sigma="0.5" length="5" minGap="2.5" maxSpeed="16.67" guiShape="passenger"/>
</flow>
```