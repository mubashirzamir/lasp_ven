#ifndef TESTAPP_H
#define TESTAPP_H

#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "LASPManager.h"

using namespace omnetpp;
using namespace inet;

namespace lasp_ven_simple {

class TestApp : public ApplicationBase, public UdpSocket::ICallback
{
private:
    // Network
    UdpSocket socket;
    L3Address laspManagerAddress;
    int laspManagerPort;
    
    // Test parameters
    int testId;
    double requestInterval;
    int requestCounter;
    int maxRequests;
    
    // Statistics
    simsignal_t requestsSent;
    simsignal_t responsesReceived;
    simsignal_t serviceLatency;
    
    // Timer for sending requests
    cMessage* requestTimer;
    
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
    
    // Test methods
    void sendTestRequest();
    ServiceType selectRandomService();
    
public:
    TestApp();
    virtual ~TestApp();
};

} // namespace lasp_ven_simple

#endif // TESTAPP_H 