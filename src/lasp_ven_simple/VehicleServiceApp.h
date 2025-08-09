#pragma once

#include "../veins_inet/VeinsInetSampleApplication.h"
#include "LASPManager.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

using namespace omnetpp;
using namespace inet;

namespace lasp_ven_simple {

class VehicleServiceApp : public VeinsInetSampleApplication
{
private:
    // Service request functionality
    UdpSocket serviceSocket;
    L3Address laspManagerAddress;
    int laspManagerPort;
    double serviceRequestInterval;
    int requestCounter;
    int maxRequests;
    
    // Statistics
    simsignal_t serviceRequestsSent;
    simsignal_t serviceResponsesReceived;
    simsignal_t serviceLatency;
    
    // Request tracking for latency measurement
    std::map<int, simtime_t> pendingRequests;
    
protected:
    virtual bool startApplication() override;
    virtual bool stopApplication() override;
    
    // UdpSocket::ICallback interface (inherited from VeinsInetApplicationBase)
    virtual void socketDataArrived(UdpSocket *socket, Packet *packet) override;
    virtual void socketErrorArrived(UdpSocket *socket, Indication *indication) override;
    virtual void socketClosed(UdpSocket *socket) override;
    
    // Service request functionality
    virtual void sendServiceRequest();
    virtual ServiceType selectServiceBasedOnContext();

    // Service request using Veins timer system
    virtual void scheduleNextServiceRequest();

public:
    VehicleServiceApp();
    virtual ~VehicleServiceApp();
};

} // namespace lasp_ven_simple