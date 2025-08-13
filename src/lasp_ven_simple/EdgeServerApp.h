#ifndef EDGESERVERAPP_H
#define EDGESERVERAPP_H

#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "LASPManager.h"

using namespace omnetpp;
using namespace inet;

namespace lasp_ven_simple {

class EdgeServerApp : public ApplicationBase, public UdpSocket::ICallback
{
private:
    // Network
    UdpSocket socket;
    int localPort;
    int serverId;
    
    // Server properties
    double computeCapacity;
    double storageCapacity;
    double currentLoad;
    std::vector<ServiceType> supportedServices;
    
    // Statistics
    simsignal_t requestsReceived;
    simsignal_t requestsProcessed;
    simsignal_t serverLoadSignal;
    
protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    
    // ApplicationBase lifecycle
    virtual void handleStartOperation(inet::LifecycleOperation* operation) override;
    virtual void handleStopOperation(inet::LifecycleOperation* operation) override;
    virtual void handleCrashOperation(inet::LifecycleOperation* operation) override;
    virtual void handleMessageWhenUp(cMessage* msg) override;
    virtual void refreshDisplay() const override;
    
    // UdpSocket::ICallback interface
    virtual void socketDataArrived(UdpSocket *socket, Packet *packet) override;
    virtual void socketErrorArrived(UdpSocket *socket, Indication *indication) override;
    virtual void socketClosed(UdpSocket *socket) override;
    
    // Service processing
    virtual void processServiceRequest(const ServiceRequest& request, const L3Address& clientAddr, int clientPort);
    virtual bool canHandleRequest(const ServiceRequest& request);
    virtual void updateLoad(double additionalLoad);
    
    // New deployment handling methods
    virtual void handleDeploymentCommand(Packet* packet, const L3Address& laspManagerAddr);
    virtual void handleDirectServiceRequest(Packet* packet, const L3Address& clientAddr);

public:
    EdgeServerApp();
    virtual ~EdgeServerApp() {}
    
    // Public interface for LASPManager
    double getCurrentLoad() const { return currentLoad; }
    double getComputeCapacity() const { return computeCapacity; }
    bool isServiceSupported(ServiceType serviceType) const;
};

} // namespace lasp_ven_simple

#endif