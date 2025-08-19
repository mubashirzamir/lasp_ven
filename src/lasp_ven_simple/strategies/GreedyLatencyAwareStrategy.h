#ifndef GREEDYLATENCYAWAReSTRATEGY_H
#define GREEDYLATENCYAWAReSTRATEGY_H

#include "../LASPManager.h"
#include <map>

namespace lasp_ven_simple {

class GreedyLatencyAwareStrategy {
public:
    static ServicePlacement* placeService(const ServiceRequest& request, 
                                        const std::map<int, EdgeServer>& edgeServers,
                                        double loadWeight = 0.5,
                                        double latencyWeight = 0.5);
};

} // namespace lasp_ven_simple

#endif // GREEDYLATENCYAWAReSTRATEGY_H
