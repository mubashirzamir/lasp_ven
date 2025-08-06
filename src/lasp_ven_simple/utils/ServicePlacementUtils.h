#ifndef SERVICEPLACEMENTUTILS_H
#define SERVICEPLACEMENTUTILS_H

#include "../LASPManager.h"
#include <cmath>

namespace lasp_ven_simple {

class ServicePlacementUtils {
public:
    static double calculateDistance(double lat1, double lon1, double lat2, double lon2);
    static double estimateLatency(const ServiceRequest& request, const EdgeServer& server);
};

} // namespace lasp_ven_simple

#endif // SERVICEPLACEMENTUTILS_H 