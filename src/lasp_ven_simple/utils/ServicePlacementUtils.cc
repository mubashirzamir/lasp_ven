#include "ServicePlacementUtils.h"

namespace lasp_ven_simple {

double ServicePlacementUtils::calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371000; // Earth radius in meters
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = sin(dLat/2) * sin(dLat/2) + 
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) * 
               sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return R * c;
}

double ServicePlacementUtils::estimateLatency(const ServiceRequest& request, const EdgeServer& server) {
    double distance = calculateDistance(request.latitude, request.longitude, 
                                      server.latitude, server.longitude);
    
    // Simple latency model
    double propagationDelay = distance / 200000000.0 * 1000; // Speed of light in fiber
    double processingDelay = request.dataSize / (server.computeCapacity / 10.0);
    double queueingDelay = (server.currentLoad / server.computeCapacity) * 20.0;
    
    return propagationDelay + processingDelay + queueingDelay;
}

} // namespace lasp_ven_simple 