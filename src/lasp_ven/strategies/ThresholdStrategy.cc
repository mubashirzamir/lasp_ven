/*
 * ThresholdStrategy.cc
 * 
 * Threshold-based service placement strategy
 * Places services only on servers with load below a specified threshold
 */

#include "../LASPManager.h"
#include <algorithm>
#include <limits>

namespace lasp_ven {

/**
 * Threshold-based placement strategy implementation
 * 
 * This strategy only considers edge servers that have current load
 * below a predefined threshold. Among eligible servers, it selects
 * the one with minimum estimated latency.
 * 
 * Key characteristics:
 * - Load balancing: Prevents overloading of servers
 * - Proactive: Maintains headroom for future requests
 * - Conservative: May reject requests when servers are near capacity
 * 
 * Parameters:
 * - loadThreshold: Maximum allowed server utilization (0.0-1.0)
 * - priorityWeight: How much to weight request priority in decisions
 */
class ThresholdStrategy {
public:
    static ServicePlacement* placeService(
        const ServiceRequest& request,
        const std::map<int, EdgeServer>& edgeServers,
        double loadThreshold = 0.8,
        double priorityWeight = 0.1) {
        
        ServicePlacement* bestPlacement = nullptr;
        double bestScore = std::numeric_limits<double>::max();
        
        for (std::map<int, EdgeServer>::const_iterator it = edgeServers.begin(); 
             it != edgeServers.end(); ++it) {
            int serverId = it->first;
            const EdgeServer& server = it->second;
            
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
            double latency = estimateLatency(request, server);
            double loadPenalty = utilization * 100.0; // Penalty for higher load
            double priorityBonus = (5.0 - request.priority) * priorityWeight * 10.0;
            
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

private:
    static double estimateLatency(const ServiceRequest& request, const EdgeServer& server) {
        // Simple distance-based latency model
        double distance = calculateDistance(request.latitude, request.longitude, 
                                          server.latitude, server.longitude);
        
        // Propagation delay + processing delay + queueing delay
        double propagationDelay = distance / 200000000.0 * 1000; // Speed of light in fiber
        double processingDelay = request.dataSize / (server.computeCapacity / 10.0);
        double queueingDelay = (server.currentLoad / server.computeCapacity) * 20.0;
        
        return propagationDelay + processingDelay + queueingDelay;
    }
    
    static double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
        const double R = 6371000; // Earth radius in meters
        double dLat = (lat2 - lat1) * M_PI / 180.0;
        double dLon = (lon2 - lon1) * M_PI / 180.0;
        double a = sin(dLat/2) * sin(dLat/2) + 
                   cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) * 
                   sin(dLon/2) * sin(dLon/2);
        double c = 2 * atan2(sqrt(a), sqrt(1-a));
        return R * c;
    }
};

} // namespace lasp_ven