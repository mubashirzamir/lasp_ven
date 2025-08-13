#include "EdgeServerApp.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/applications/base/ApplicationPacket_m.h"

using namespace omnetpp;
using namespace inet;

namespace lasp_ven_simple {

Define_Module(EdgeServerApp);

EdgeServerApp::EdgeServerApp()
{
    EV_WARN << "EdgeServerApp constructor called" << endl;
}

void EdgeServerApp::initialize(int stage)
{
    EV_WARN << "=== EDGE SERVER APP INITIALIZING ===" << endl;
    EV_WARN << "Stage: " << stage << endl;
    
    ApplicationBase::initialize(stage);
    
    if (stage == INITSTAGE_LOCAL) {
        // Read parameters
        serverId = par("serverId");
        computeCapacity = par("computeCapacity");
        storageCapacity = par("storageCapacity");
        localPort = par("localPort");
        currentLoad = 0.0;
        
        EV_WARN << "EdgeServerApp parameters loaded:" << endl;
        EV_WARN << "  serverId: " << serverId << endl;
        EV_WARN << "  computeCapacity: " << computeCapacity << endl;
        EV_WARN << "  storageCapacity: " << storageCapacity << endl;
        EV_WARN << "  localPort: " << localPort << endl;
        
        // Initialize supported services (all services for simplicity)
        supportedServices = {TRAFFIC_INFO, EMERGENCY_ALERT, INFOTAINMENT, NAVIGATION};
        
        // Initialize statistics
        requestsReceived = registerSignal("requestsReceived");
        requestsProcessed = registerSignal("requestsProcessed");
        serverLoadSignal = registerSignal("serverLoad");
        
        EV_WARN << "Statistics signals registered successfully" << endl;
        EV_WARN << "=== EDGE SERVER APP INITIALIZED ===" << endl;
    }
}

void EdgeServerApp::handleStartOperation(inet::LifecycleOperation* operation)
{
    EV_WARN << "=== EDGE SERVER STARTING ===" << endl;
    EV_WARN << "EdgeServer " << serverId << " handleStartOperation called" << endl;
    EV_WARN << "EdgeServer " << serverId << " starting on port " << localPort << endl;
    
    // Setup UDP socket
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort);
    socket.setCallback(this);
    
    EV_WARN << "EdgeServer " << serverId << " socket setup complete" << endl;
    EV_WARN << "=== EDGE SERVER STARTED ===" << endl;
}

void EdgeServerApp::handleStopOperation(inet::LifecycleOperation* operation)
{
    socket.close();
    EV_INFO << "EdgeServer " << serverId << " stopped" << endl;
}

void EdgeServerApp::handleCrashOperation(inet::LifecycleOperation* operation)
{
    socket.destroy();
    EV_WARN << "EdgeServer " << serverId << " crashed!" << endl;
}

void EdgeServerApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        // Handle self-messages (timers, etc.)
        delete msg;
    } else {
        socket.processMessage(msg);
    }
}

void EdgeServerApp::handleMessageWhenUp(cMessage* msg)
{
    handleMessage(msg);
}

void EdgeServerApp::refreshDisplay() const
{
    char buf[100];
    sprintf(buf, "Server %d\\nLoad: %.1f%%", serverId, (currentLoad / computeCapacity) * 100);
    getDisplayString().setTagArg("t", 0, buf);
}

void EdgeServerApp::socketDataArrived(UdpSocket *socket, Packet *packet)
{
    EV_WARN << "=== EDGE SERVER RECEIVED PACKET ===" << endl;
    EV_WARN << "EdgeServer " << serverId << " received packet: " << packet->getName() << endl;
    EV_WARN << "Packet received at time: " << simTime() << endl;
    
    emit(requestsReceived, 1);
    
    // Get client address for response
    auto addressInd = packet->getTag<L3AddressInd>();
    L3Address clientAddr = addressInd->getSrcAddress();
    
    EV_WARN << "Client address: " << clientAddr.str() << endl;
    
    // Check packet name to determine if it's a deployment command or direct service request
    if (strcmp(packet->getName(), "ServiceDeployment") == 0) {
        EV_WARN << "Processing deployment command from LASPManager" << endl;
        // This is a deployment command from LASPManager
        handleDeploymentCommand(packet, clientAddr);
    } else {
        EV_WARN << "Processing direct service request" << endl;
        // This is a direct service request (shouldn't happen in our current flow)
        handleDirectServiceRequest(packet, clientAddr);
    }
    
    delete packet;
}

void EdgeServerApp::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    EV_WARN << "EdgeServer " << serverId << " UDP error: " << indication->getName() << endl;
    delete indication;
}

void EdgeServerApp::socketClosed(UdpSocket *socket)
{
    EV_INFO << "EdgeServer " << serverId << " socket closed" << endl;
}

void EdgeServerApp::processServiceRequest(const ServiceRequest& request, const L3Address& clientAddr, int clientPort)
{
    EV_INFO << "EdgeServer " << serverId << " processing request from vehicle " << request.vehicleId << endl;
    
    if (!canHandleRequest(request)) {
        EV_WARN << "EdgeServer " << serverId << " cannot handle request - insufficient capacity or unsupported service" << endl;
        return;
    }
    
    // Process the request
    double processingLoad = request.dataSize * 0.1; // Simple load calculation
    updateLoad(processingLoad);
    
    emit(requestsProcessed, 1);
    emit(serverLoadSignal, (currentLoad / computeCapacity) * 100);
    
    // Send response back to client (simplified)
    auto response = new Packet("ServiceResponse");
    socket.sendTo(response, clientAddr, clientPort);
    
    EV_INFO << "EdgeServer " << serverId << " processed request, current load: " 
            << (currentLoad / computeCapacity) * 100 << "%" << endl;
}

bool EdgeServerApp::canHandleRequest(const ServiceRequest& request)
{
    // Check if service is supported
    if (!isServiceSupported(request.serviceType)) {
        return false;
    }
    
    // Check if there's sufficient capacity
    double requiredCapacity = request.dataSize * 0.1;
    if ((currentLoad + requiredCapacity) > computeCapacity) {
        return false;
    }
    
    return true;
}

bool EdgeServerApp::isServiceSupported(ServiceType serviceType) const
{
    for (ServiceType service : supportedServices) {
        if (service == serviceType) {
            return true;
        }
    }
    return false;
}

void EdgeServerApp::updateLoad(double additionalLoad)
{
    currentLoad += additionalLoad;
    
    // Simple load decay over time (services complete)
    if (simTime() > 0) {
        currentLoad *= 0.95; // 5% decay per update
    }
    
    if (currentLoad < 0.01) {
        currentLoad = 0.0;
    }
}

void EdgeServerApp::handleDeploymentCommand(Packet* packet, const L3Address& laspManagerAddr)
{
    EV_WARN << "=== EDGE SERVER PROCESSING DEPLOYMENT COMMAND ===" << endl;
    
    // Extract deployment information
    auto payload = packet->peekData<ApplicationPacket>();
    int vehicleId = payload->getSequenceNumber();
    
    EV_WARN << "EdgeServer " << serverId << " received deployment command for vehicle " << vehicleId << endl;
    
    // Simulate service processing time
    double processingTime = uniform(0.01, 0.05); // 10-50ms processing time
    
    EV_WARN << "Processing time: " << processingTime * 1000 << "ms" << endl;
    
    // Update server load
    updateLoad(1.0); // Add some load for this service
    
    // Create service response
    auto response = new Packet("ServiceResponse");
    auto responsePayload = makeShared<ApplicationPacket>();
    responsePayload->setChunkLength(B(300)); // 300 bytes response
    responsePayload->setSequenceNumber(vehicleId);
    response->insertAtBack(responsePayload);
    
    // Send response back to vehicle (simplified addressing)
    std::string addressStr = "192.168.1." + std::to_string(10 + vehicleId);
    L3Address vehicleAddr = L3AddressResolver().resolve(addressStr.c_str());
    int vehiclePort = 5000; // Assume vehicles listen on port 5000
    
    EV_WARN << "Sending response to vehicle at " << vehicleAddr.str() << ":" << vehiclePort << endl;
    
    // Schedule response after processing time
    scheduleAt(simTime() + processingTime, new cMessage("sendResponse"));
    
    // For now, send immediately (in real implementation, would schedule)
    socket.sendTo(response, vehicleAddr, vehiclePort);
    
    emit(requestsProcessed, 1);
    emit(serverLoadSignal, (currentLoad / computeCapacity) * 100);
    
    EV_WARN << "EdgeServer " << serverId << " processed service for vehicle " << vehicleId 
            << ", current load: " << (currentLoad / computeCapacity) * 100 << "%" << endl;
    EV_WARN << "=== DEPLOYMENT COMMAND PROCESSED SUCCESSFULLY ===" << endl;
}

void EdgeServerApp::handleDirectServiceRequest(Packet* packet, const L3Address& clientAddr)
{
    // Handle direct service requests (for future use)
    EV_INFO << "EdgeServer " << serverId << " received direct service request" << endl;
    
    // For now, just acknowledge
    auto response = new Packet("DirectServiceResponse");
    socket.sendTo(response, clientAddr, 5000);
}

void EdgeServerApp::finish()
{
    EV_INFO << "EdgeServer " << serverId << " finished. Final load: " 
            << (currentLoad / computeCapacity) * 100 << "%" << endl;
}

} // namespace lasp_ven_simple
