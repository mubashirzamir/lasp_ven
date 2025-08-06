#ifndef GREEDYSTRATEGY_H
#define GREEDYSTRATEGY_H

#include "../LASPManager.h"
#include <map>

namespace lasp_ven_simple {

class GreedyStrategy {
public:
    static ServicePlacement* placeService(const ServiceRequest& request, 
                                        const std::map<int, EdgeServer>& edgeServers);
};

} // namespace lasp_ven_simple

#endif // GREEDYSTRATEGY_H 