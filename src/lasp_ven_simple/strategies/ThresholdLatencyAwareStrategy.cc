#include "ThresholdLatencyAwareStrategy.h"
#include "../utils/ServicePlacementUtils.h"
#include <algorithm>
#include <limits>
#include <vector>

namespace lasp_ven_simple {

ServicePlacement* ThresholdLatencyAwareStrategy::placeService(
    const ServiceRequest& request,
    const std::map<int, EdgeServer>& edgeServers,
    double loadThreshold,
    double loadWeight,
    double latencyWeight) {
    
    ServicePlacement* bestPlacement = nullptr;
    double bestScore = std::numeric_limits<double>::max();
    
    EV_WARN << "[LATENCY-AWARE-THRESHOLD] Processing request from vehicle " << request.vehicleId 
            << " with threshold: " << loadThreshold 
            << ", weights - Load: " << loadWeight << ", Latency: " << latencyWeight << endl;
    
    // First pass: collect servers under threshold
    std::vector<std::pair<int, const EdgeServer*>> eligibleServers;
    
    for (const auto& serverPair : edgeServers) {
        int serverId = serverPair.first;
        const EdgeServer& server = serverPair.second;
        
        // Check basic eligibility
        if (!server.isActive) {
            EV_WARN << "[LATENCY-AWARE-THRESHOLD] Server " << serverId << " is inactive, skipping" << endl;
            continue;
        }
        
        // Calculate current utilization
        double utilization = server.currentLoad / server.computeCapacity;
        if (utilization > loadThreshold) {
            EV_WARN << "[LATENCY-AWARE-THRESHOLD] Server " << serverId 
                    << " above threshold: " << (utilization * 100) << "% > " << (loadThreshold * 100) << "%" << endl;
            continue;
        }
        
        // Check if server supports the requested service
        auto serviceIt = std::find(server.supportedServices.begin(), 
                                 server.supportedServices.end(), 
                                 request.serviceType);
        if (serviceIt == server.supportedServices.end()) {
            EV_WARN << "[LATENCY-AWARE-THRESHOLD] Server " << serverId << " doesn't support service type" << endl;
            continue;
        }
        
        // Check resource availability
        if (server.currentLoad + request.dataSize > server.computeCapacity) {
            EV_WARN << "[LATENCY-AWARE-THRESHOLD] Server " << serverId << " insufficient capacity" << endl;
            continue;
        }
        
        eligibleServers.push_back({serverId, &server});
        EV_WARN << "[LATENCY-AWARE-THRESHOLD] Server " << serverId << " is eligible (util: " 
                << (utilization * 100) << "%)" << endl;
    }
    
    // If no servers under threshold, fall back to greedy approach with all servers
    if (eligibleServers.empty()) {
        EV_WARN << "[LATENCY-AWARE-THRESHOLD] No servers under threshold, falling back to greedy approach" << endl;
        
        for (const auto& serverPair : edgeServers) {
            int serverId = serverPair.first;
            const EdgeServer& server = serverPair.second;
            
            if (!server.isActive) continue;
            
            // Check service support and capacity
            auto serviceIt = std::find(server.supportedServices.begin(), 
                                     server.supportedServices.end(), 
                                     request.serviceType);
            if (serviceIt == server.supportedServices.end()) continue;
            
            if (server.currentLoad + request.dataSize > server.computeCapacity) continue;
            
            eligibleServers.push_back({serverId, &server});
        }
    }
    
    // Second pass: evaluate eligible servers using combined metrics
    for (const auto& serverInfo : eligibleServers) {
        int serverId = serverInfo.first;
        const EdgeServer& server = *serverInfo.second;
        
        // Calculate latency for this server
        double latency = ServicePlacementUtils::estimateLatency(request, server);
        
        // Calculate load utilization
        double loadUtilization = server.currentLoad / server.computeCapacity;
        
        // Calculate combined score (lower is better)
        // Normalize both metrics to 0-1 range and apply weights
        double normalizedLatency = std::min(latency / 100.0, 1.0); // Normalize to 0-1, cap at 100ms
        double normalizedLoad = loadUtilization; // Already 0-1
        
        double combinedScore = (latencyWeight * normalizedLatency) + (loadWeight * normalizedLoad);
        
        EV_WARN << "[LATENCY-AWARE-THRESHOLD] Server " << serverId 
                << " - Latency: " << latency << "ms (norm: " << normalizedLatency 
                << "), Load: " << (loadUtilization * 100) << "% (norm: " << normalizedLoad 
                << "), Score: " << combinedScore << endl;
        
        if (combinedScore < bestScore) {
            bestScore = combinedScore;
            
            if (bestPlacement) delete bestPlacement;
            bestPlacement = new ServicePlacement();
            bestPlacement->serviceId = request.vehicleId;
            bestPlacement->serverId = serverId;
            bestPlacement->serviceType = request.serviceType;
            bestPlacement->placementTime = simTime().dbl();
            bestPlacement->estimatedLatency = latency;
            bestPlacement->resourceUsage = request.dataSize;
            
            EV_WARN << "[LATENCY-AWARE-THRESHOLD] New best server: " << serverId 
                    << " with score: " << combinedScore << endl;
        }
    }
    
    if (bestPlacement) {
        EV_WARN << "[LATENCY-AWARE-THRESHOLD] Selected server " << bestPlacement->serverId 
                << " with final score: " << bestScore << endl;
    } else {
        EV_WARN << "[LATENCY-AWARE-THRESHOLD] No suitable server found" << endl;
    }
    
    return bestPlacement;
}

} // namespace lasp_ven_simple
