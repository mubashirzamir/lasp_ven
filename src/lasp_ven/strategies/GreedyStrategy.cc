/*
 * GreedyStrategy.cc
 *
 * Greedy service placement strategy
 * Always selects the server with minimum estimated latency
 */

#include "../LASPManager.h"
#include <algorithm>
#include <limits>

namespace lasp_ven {

/**
 * Greedy placement strategy implementation
 *
 * This strategy always attempts to minimize latency by selecting
 * the edge server that can provide the lowest estimated response time,
 * regardless of server load or other factors.
 *
 * Key characteristics:
 * - Latency-optimal: Always chooses minimum latency option
 * - Simple: Easy to implement and understand
 * - Aggressive: May cause server overloading
 * - Myopic: Doesn't consider long-term implications
 *
 * Use cases:
 * - Real-time applications requiring minimal latency
 * - Emergency services where speed is critical
 * - Low-load scenarios where overloading is not a concern
 */
class GreedyStrategy {
public:
    static ServicePlacement* placeService(
        const ServiceRequest& request,
        const std::map<int, EdgeServer>& edgeServers,
        double maxLatency = 1000.0) { // Max latency in ms
        
        ServicePlacement* bestPlacement = nullptr;
        double bestLatency = std::numeric_limits<double>::max();
        
        for (const auto& [serverId, server] : edgeServers) {
            // Check basic eligibility
            if (!server.isActive) continue;

            // Check if server supports the requested service
            auto serviceIt = std::find(server.supportedServices.begin(),
                                     server.supportedServices.end(),
                                     request.serviceType);
            if (serviceIt == server.supportedServices.end()) continue;

            // Check basic resource availability (allow some overloading)
            if (server.currentLoad + request.dataSize > server.computeCapacity * 1.2) continue;

            // Calculate estimated latency
            double latency = estimateLatency(request, server);

            // Skip if latency exceeds maximum allowed
            if (latency > maxLatency) continue;
            
            // Select server with minimum latency
            if (latency < bestLatency) {
                bestLatency = latency;

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
        // Distance-based latency calculation
        double distance = calculateDistance(request.latitude, request.longitude,
                                          server.latitude, server.longitude);
        
        // Simple latency model components
        double propagationDelay = distance / 200000000.0 * 1000; // Speed of light in fiber (ms)
        double processingDelay = request.dataSize / (server.computeCapacity / 10.0); // Processing time
        
        // Greedy strategy uses minimal queueing delay consideration
        double queueingDelay = (server.currentLoad / server.computeCapacity) * 5.0; // Light penalty
        
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
