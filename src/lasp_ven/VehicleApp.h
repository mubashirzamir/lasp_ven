#ifndef VEHICLEAPP_H
#define VEHICLEAPP_H

#include "veins_inet/VeinsInetApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "LASPManager.h"

using namespace omnetpp;
using namespace inet;

namespace lasp_ven {

class VehicleApp : public veins::VeinsInetApplicationBase, public UdpSocket::ICallback
{
private:
    // Network
    UdpSocket socket;
    L3Address laspManagerAddress;
    int laspManagerPort;
    
    // Vehicle state
    int vehicleId;
    double lastRequestTime;
    double requestInterval;
    int requestCounter;
    
    // Service parameters
    std::vector<ServiceType> preferredServices;
    double currentLatitude;
    double currentLongitude;
    
    // Statistics
    simsignal_t requestsSent;
    simsignal_t responsesReceived;
    simsignal_t serviceLatency;
    
protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage* msg) override;
    virtual void finish() override;
    
    // UDP callback methods
    virtual void socketDataArrived(UdpSocket *socket, Packet *packet) override;
    virtual void socketErrorArrived(UdpSocket *socket, Indication *indication) override;
    virtual void socketClosed(UdpSocket *socket) override;
    
    // Vehicle-specific methods
    void sendServiceRequest();
    void updatePosition();
    ServiceType selectRandomService();
    
    // Veins integration
    virtual void receiveSignal(cComponent* source, simsignal_t signalID, cObject* obj, cObject* details);

public:
    VehicleApp();
    virtual ~VehicleApp();
};

} // namespace lasp_ven

#endif // VEHICLEAPP_H
