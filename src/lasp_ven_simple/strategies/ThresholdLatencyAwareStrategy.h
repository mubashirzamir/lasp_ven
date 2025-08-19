#ifndef THRESHOLDLATENCYAWAReSTRATEGY_H
#define THRESHOLDLATENCYAWAReSTRATEGY_H

#include "../LASPManager.h"
#include <map>

namespace lasp_ven_simple {

class ThresholdLatencyAwareStrategy {
public:
    static ServicePlacement* placeService(
        const ServiceRequest& request,
        const std::map<int, EdgeServer>& edgeServers,
        double loadThreshold = 0.8,
        double loadWeight = 0.5,
        double latencyWeight = 0.5);
};

} // namespace lasp_ven_simple

#endif // THRESHOLDLATENCYAWAReSTRATEGY_H
