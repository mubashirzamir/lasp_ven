#ifndef THRESHOLDSTRATEGY_H
#define THRESHOLDSTRATEGY_H

#include "../LASPManager.h"
#include <map>

namespace lasp_ven_simple {

class ThresholdStrategy {
public:
    static ServicePlacement* placeService(
        const ServiceRequest& request,
        const std::map<int, EdgeServer>& edgeServers,
        double loadThreshold = 0.8);
};

} // namespace lasp_ven_simple

#endif // THRESHOLDSTRATEGY_H 