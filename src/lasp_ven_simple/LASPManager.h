#ifndef LASPMANAGER_H
#define LASPMANAGER_H

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include <map>
#include <vector>
#include <string>

using namespace omnetpp;
using namespace inet;

namespace lasp_ven_simple {

enum ServiceType {
    TRAFFIC_INFO = 1,
    EMERGENCY_ALERT = 2,
    INFOTAINMENT = 3,
    NAVIGATION = 4
};

struct ServiceRequest {
    int vehicleId;
    ServiceType serviceType;
    double timestamp;
    double latitude;
    double longitude;
    int priority;
    double deadline;
    double dataSize; // in MB
};

struct EdgeServer {
    int serverId;
    double latitude;
    double longitude;
    double computeCapacity; // in GFLOPS
    double storageCapacity; // in GB
    double currentLoad;
    std::vector<ServiceType> supportedServices;
    bool isActive;
};

struct ServicePlacement {
    int serviceId;
    int serverId;
    ServiceType serviceType;
    double placementTime;
    double estimatedLatency;
    double resourceUsage;
};

class LASPManager : public ApplicationBase, public UdpSocket::ICallback
{
private:
    // Network
    UdpSocket socket;
    int localPort;
    
    // Edge servers management
    std::map<int, EdgeServer> edgeServers;
    std::vector<ServiceRequest> pendingRequests;
    std::vector<ServicePlacement> activePlacements;
    
    // Strategy selection
    std::string currentStrategy;
    double loadThreshold;
    
    // Statistics
    simsignal_t requestsReceived;
    simsignal_t requestsServed;
    simsignal_t averageLatency;
    simsignal_t serverUtilization;
    
    // QoS Metrics
    simsignal_t requestSuccessRate;
    simsignal_t requestRejectionRate;
    simsignal_t serviceCompletionTime;
    simsignal_t loadBalancingEfficiency;
    
    // Parameters
    double evaluationInterval;
    int numEdgeServers;
    
    // Statistics tracking
    int totalRequestsReceived;
    int totalRequestsServed;
    
    // Timer for periodic evaluation
    cMessage* evaluationTimer;
    
protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    
    // ApplicationBase pure virtual functions
    virtual void handleStartOperation(inet::LifecycleOperation* operation) override;
    virtual void handleStopOperation(inet::LifecycleOperation* operation) override;
    virtual void handleCrashOperation(inet::LifecycleOperation* operation) override;
    virtual void handleMessageWhenUp(cMessage* msg) override;
    virtual void refreshDisplay() const override;
    
    // UDP callback methods
    virtual void socketDataArrived(UdpSocket *socket, Packet *packet) override;
    virtual void socketErrorArrived(UdpSocket *socket, Indication *indication) override;
    virtual void socketClosed(UdpSocket *socket) override;
    
    // Service placement methods
    void initializeEdgeServers();
    void processServiceRequest(const ServiceRequest& request);
    ServicePlacement* findBestPlacement(const ServiceRequest& request);
    void updateServerLoad();
    void evaluateCurrentPlacements();
    
    // Strategy methods
    ServicePlacement* thresholdBasedPlacement(const ServiceRequest& request);
    ServicePlacement* greedyPlacement(const ServiceRequest& request);
    
    // Utility methods
    bool canServerHandleRequest(const EdgeServer& server, const ServiceRequest& request);
    
    // Timer handling
    void scheduleEvaluationTimer();
    void handleEvaluationTimer();
    
    // Edge server communication
    void sendDeploymentCommand(const ServicePlacement& placement, const ServiceRequest& request);
    
public:
    LASPManager();
    virtual ~LASPManager();
    
    // Public interface for vehicles
    void submitServiceRequest(const ServiceRequest& request);
    void removeServiceRequest(int requestId);
    
    // Getters for analysis
    const std::map<int, EdgeServer>& getEdgeServers() const { return edgeServers; }
    const std::vector<ServicePlacement>& getActivePlacements() const { return activePlacements; }
};

} // namespace lasp_ven_simple

#endif // LASPMANAGER_H 
