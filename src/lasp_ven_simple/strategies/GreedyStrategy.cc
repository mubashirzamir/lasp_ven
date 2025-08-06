#include "GreedyStrategy.h"
#include "../utils/ServicePlacementUtils.h"
#include <algorithm>
#include <limits>

namespace lasp_ven_simple {

ServicePlacement* GreedyStrategy::placeService(const ServiceRequest& request, 
                                             const std::map<int, EdgeServer>& edgeServers)
{
    ServicePlacement* bestPlacement = nullptr;
    double bestLatency = std::numeric_limits<double>::max();
    
    for (const auto& serverPair : edgeServers) {
        const EdgeServer& server = serverPair.second;
        
        // Skip inactive servers
        if (!server.isActive) continue;
        
        // Check if server supports the service type
        bool supportsService = false;
        for (ServiceType service : server.supportedServices) {
            if (service == request.serviceType) {
                supportsService = true;
                break;
            }
        }
        
        if (!supportsService) continue;
        
        // Check if server has capacity
        double requiredCapacity = request.dataSize * 0.1; // Simple capacity calculation
        if ((server.currentLoad + requiredCapacity) > server.computeCapacity) {
            continue;
        }
        
        // Calculate latency for this server
        double latency = ServicePlacementUtils::estimateLatency(request, server);
        
        // Greedy: choose the server with lowest latency
        if (latency < bestLatency) {
            bestLatency = latency;
            
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
        }
    }
    
    return bestPlacement;
}

} // namespace lasp_ven_simple 