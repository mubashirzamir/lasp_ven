#include "GreedyLatencyAwareStrategy.h"
#include "../utils/ServicePlacementUtils.h"
#include <algorithm>
#include <limits>

namespace lasp_ven_simple {

ServicePlacement* GreedyLatencyAwareStrategy::placeService(const ServiceRequest& request, 
                                                         const std::map<int, EdgeServer>& edgeServers,
                                                         double loadWeight,
                                                         double latencyWeight)
{
    ServicePlacement* bestPlacement = nullptr;
    double bestScore = std::numeric_limits<double>::max();
    
    EV_WARN << "[LATENCY-AWARE-GREEDY] Processing request from vehicle " << request.vehicleId 
            << " with weights - Load: " << loadWeight << ", Latency: " << latencyWeight << endl;
    
    for (const auto& serverPair : edgeServers) {
        const EdgeServer& server = serverPair.second;
        
        // Skip inactive servers
        if (!server.isActive) {
            EV_WARN << "[LATENCY-AWARE-GREEDY] Server " << server.serverId << " is inactive, skipping" << endl;
            continue;
        }
        
        // Check if server supports the service type
        bool supportsService = false;
        for (ServiceType service : server.supportedServices) {
            if (service == request.serviceType) {
                supportsService = true;
                break;
            }
        }
        
        if (!supportsService) {
            EV_WARN << "[LATENCY-AWARE-GREEDY] Server " << server.serverId << " doesn't support service type" << endl;
            continue;
        }
        
        // Check if server has capacity
        double requiredCapacity = request.dataSize * 0.1; // Simple capacity calculation
        if ((server.currentLoad + requiredCapacity) > server.computeCapacity) {
            EV_WARN << "[LATENCY-AWARE-GREEDY] Server " << server.serverId << " insufficient capacity" << endl;
            continue;
        }
        
        // Calculate latency for this server
        double latency = ServicePlacementUtils::estimateLatency(request, server);
        
        // Calculate load utilization (0.0 = no load, 1.0 = fully loaded)
        double loadUtilization = server.currentLoad / server.computeCapacity;
        
        // Calculate combined score (lower is better)
        // Normalize both metrics to 0-1 range and apply weights
        double normalizedLatency = std::min(latency / 100.0, 1.0); // Normalize to 0-1, cap at 100ms
        double normalizedLoad = loadUtilization; // Already 0-1
        
        double combinedScore = (latencyWeight * normalizedLatency) + (loadWeight * normalizedLoad);
        
        EV_WARN << "[LATENCY-AWARE-GREEDY] Server " << server.serverId 
                << " - Latency: " << latency << "ms (norm: " << normalizedLatency 
                << "), Load: " << (loadUtilization * 100) << "% (norm: " << normalizedLoad 
                << "), Score: " << combinedScore << endl;
        
        // Greedy: choose the server with lowest combined score
        if (combinedScore < bestScore) {
            bestScore = combinedScore;
            
            if (bestPlacement) {
                delete bestPlacement;
            }
            
            bestPlacement = new ServicePlacement();
            bestPlacement->serviceId = request.vehicleId;
            bestPlacement->serverId = server.serverId;
            bestPlacement->serviceType = request.serviceType;
            bestPlacement->placementTime = simTime().dbl();
            bestPlacement->estimatedLatency = latency;
            bestPlacement->resourceUsage = requiredCapacity;
            
            EV_WARN << "[LATENCY-AWARE-GREEDY] New best server: " << server.serverId 
                    << " with score: " << combinedScore << endl;
        }
    }
    
    if (bestPlacement) {
        EV_WARN << "[LATENCY-AWARE-GREEDY] Selected server " << bestPlacement->serverId 
                << " with final score: " << bestScore << endl;
    } else {
        EV_WARN << "[LATENCY-AWARE-GREEDY] No suitable server found" << endl;
    }
    
    return bestPlacement;
}

} // namespace lasp_ven_simple
