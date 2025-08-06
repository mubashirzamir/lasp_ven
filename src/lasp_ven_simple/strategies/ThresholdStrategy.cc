#include "ThresholdStrategy.h"
#include "../utils/ServicePlacementUtils.h"
#include <algorithm>
#include <limits>
#include <cmath>

namespace lasp_ven_simple {

ServicePlacement* ThresholdStrategy::placeService(
    const ServiceRequest& request,
    const std::map<int, EdgeServer>& edgeServers,
    double loadThreshold) {
    
    ServicePlacement* bestPlacement = nullptr;
    double bestScore = std::numeric_limits<double>::max();
    
    for (const auto& serverPair : edgeServers) {
        int serverId = serverPair.first;
        const EdgeServer& server = serverPair.second;
        
        // Check basic eligibility
        if (!server.isActive) continue;
        
        // Calculate current utilization
        double utilization = server.currentLoad / server.computeCapacity;
        if (utilization > loadThreshold) continue;
        
        // Check if server supports the requested service
        auto serviceIt = std::find(server.supportedServices.begin(), 
                                 server.supportedServices.end(), 
                                 request.serviceType);
        if (serviceIt == server.supportedServices.end()) continue;
        
        // Check resource availability
        if (server.currentLoad + request.dataSize > server.computeCapacity) continue;
        
        // Calculate placement score (lower is better)
        double latency = ServicePlacementUtils::estimateLatency(request, server);
        double loadPenalty = utilization * 100.0; // Penalty for higher load
        double priorityBonus = (5.0 - request.priority) * 10.0; // Higher priority = lower score
        
        double score = latency + loadPenalty - priorityBonus;
        
        if (score < bestScore) {
            bestScore = score;
            
            if (bestPlacement) delete bestPlacement;
            bestPlacement = new ServicePlacement();
            bestPlacement->serviceId = request.vehicleId;
            bestPlacement->serverId = serverId;
            bestPlacement->serviceType = request.serviceType;
            bestPlacement->placementTime = simTime().dbl();
            bestPlacement->estimatedLatency = latency;
            bestPlacement->resourceUsage = request.dataSize;
        }
    }
    
    return bestPlacement;
}

} // namespace lasp_ven_simple 