#include "TestApp.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include <random>

using namespace omnetpp;
using namespace inet;

namespace lasp_ven_simple {

Define_Module(TestApp);

TestApp::TestApp()
{
    testId = 0;
    requestInterval = 5.0; // Default 5 seconds
    requestCounter = 0;
    maxRequests = 10; // Default max requests
    laspManagerPort = 9999;
    requestTimer = nullptr;
}

TestApp::~TestApp()
{
    if (requestTimer) {
        cancelAndDelete(requestTimer);
    }
}

void TestApp::initialize(int stage)
{
    ApplicationBase::initialize(stage);
    
    if (stage == inet::INITSTAGE_LOCAL) {
        // Read parameters
        testId = par("testId").intValue();
        requestInterval = par("requestInterval").doubleValue();
        maxRequests = par("maxRequests").intValue();
        laspManagerPort = par("laspManagerPort").intValue();
        
        // Initialize statistics
        requestsSent = registerSignal("requestsSent");
        responsesReceived = registerSignal("responsesReceived");
        serviceLatency = registerSignal("serviceLatency");
        
        EV_INFO << "TestApp " << testId << " initialized" << endl;
    }
}

void TestApp::handleStartOperation(inet::LifecycleOperation* operation)
{
    // Application is starting
    EV_INFO << "TestApp " << testId << " starting..." << endl;
    
    // Setup UDP socket
    socket.setOutputGate(gate("socketOut"));
    socket.bind(L3Address(), 0); // Bind to any available port
    socket.setCallback(this);
    
    // Resolve LASP Manager address
    L3AddressResolver().tryResolve("laspManager", laspManagerAddress);
    if (laspManagerAddress.isUnspecified()) {
        EV_ERROR << "Could not resolve LASP Manager address" << endl;
        return;
    }
    
    // Schedule first request
    requestTimer = new cMessage("requestTimer");
    scheduleAt(simTime() + requestInterval, requestTimer);
    
    EV_INFO << "TestApp " << testId << " started. Will send requests to " 
            << laspManagerAddress.str() << ":" << laspManagerPort << endl;
}

void TestApp::handleStopOperation(inet::LifecycleOperation* operation)
{
    // Application is stopping
    EV_INFO << "TestApp " << testId << " stopping..." << endl;
    
    if (requestTimer) {
        cancelAndDelete(requestTimer);
        requestTimer = nullptr;
    }
    
    socket.close();
    
    EV_INFO << "TestApp " << testId << " stopped." << endl;
}

void TestApp::handleCrashOperation(inet::LifecycleOperation* operation)
{
    // Application crashed
    EV_WARN << "TestApp " << testId << " crashed!" << endl;
    
    if (requestTimer) {
        cancelAndDelete(requestTimer);
        requestTimer = nullptr;
    }
    
    socket.destroy();
}

void TestApp::handleMessage(cMessage *msg)
{
    // Delegate to handleMessageWhenUp for proper lifecycle handling
    handleMessageWhenUp(msg);
}

void TestApp::handleMessageWhenUp(cMessage* msg)
{
    // Handle messages when application is up
    if (msg == requestTimer) {
        sendTestRequest();
        
        // Schedule next request if we haven't reached the limit
        if (requestCounter < maxRequests) {
            scheduleAt(simTime() + requestInterval, requestTimer);
        }
    }
    else {
        socket.processMessage(msg);
    }
}

void TestApp::refreshDisplay() const
{
    // Update display string
    char buf[100];
    sprintf(buf, "Test %d: %d/%d", testId, requestCounter, maxRequests);
    getDisplayString().setTagArg("t", 0, buf);
}

void TestApp::sendTestRequest()
{
    if (requestCounter >= maxRequests) {
        EV_INFO << "TestApp " << testId << " reached max requests limit" << endl;
        return;
    }
    
    // Create a test packet
    auto packet = new Packet("testRequest");
    auto payload = makeShared<ApplicationPacket>();
    payload->setChunkLength(B(100)); // 100 bytes
    payload->setSequenceNumber(requestCounter);
    packet->insertAtBack(payload);
    
    // Send to LASP Manager
    socket.sendTo(packet, laspManagerAddress, laspManagerPort);
    
    emit(requestsSent, 1);
    requestCounter++;
    
    EV_INFO << "TestApp " << testId << " sent request " << requestCounter 
            << " with service " << selectRandomService() << endl;
}

ServiceType TestApp::selectRandomService()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1, 4);
    
    return static_cast<ServiceType>(dis(gen));
}

void TestApp::socketDataArrived(UdpSocket *socket, Packet *packet)
{
    emit(responsesReceived, 1);
    
    // For now, just log the response
    EV_INFO << "TestApp " << testId << " received response" << endl;
    
    delete packet;
}

void TestApp::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    EV_WARN << "UDP error: " << indication->getName() << endl;
    delete indication;
}

void TestApp::socketClosed(UdpSocket *socket)
{
    EV_INFO << "UDP socket closed" << endl;
}

void TestApp::finish()
{
    ApplicationBase::finish();
    
    EV_INFO << "TestApp " << testId << " finished. Sent " << requestCounter 
            << " requests, received " << responsesReceived << " responses" << endl;
}

} // namespace lasp_ven_simple 