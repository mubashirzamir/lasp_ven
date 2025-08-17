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
    EV_WARN << "[NETWORK-DEBUG] EdgeServer " << serverId << " bound to port " << localPort << " and ready to receive" << endl;
    
    // Debug: Check IP address and routing
    auto wlan = getModuleByPath("^.wlan[0]");
    if (wlan) {
        EV_WARN << "[DEBUG-ROUTE-001] EdgeServer " << serverId << ": Found wlan interface" << endl;
        
        // IPv4 module is at host level, not wlan level
        auto ipv4 = getModuleByPath("^.ipv4");
        
        if (ipv4) {
            EV_WARN << "[DEBUG-ROUTE-001] EdgeServer " << serverId << ": Found IPv4 module at host level" << endl;
            
            // Check routing table at host level
            auto routingTable = getModuleByPath("^.ipv4.routingTable");
            if (routingTable) {
                auto iroutingTable = check_and_cast<IRoutingTable*>(routingTable);
                EV_WARN << "[DEBUG-ROUTE-001] EdgeServer " << serverId << ": Routing table has " << iroutingTable->getNumRoutes() << " routes" << endl;
                EV_WARN << "[NETWORK-DEBUG] EdgeServer " << serverId << ": IPv4 network stack is properly initialized!" << endl;
                for (int i = 0; i < iroutingTable->getNumRoutes(); i++) {
                    auto route = iroutingTable->getRoute(i);
                    if (route) {
                        EV_WARN << "[DEBUG-ROUTE-001] EdgeServer " << serverId << ": Route " << i << ": " 
                                << route->getDestinationAsGeneric().str() << " -> " << route->getNextHopAsGeneric().str() << endl;
                    }
                }
            } else {
                EV_WARN << "[DEBUG-ROUTE-001] EdgeServer " << serverId << ": No routing table found" << endl;
            }
        } else {
            EV_WARN << "[DEBUG-ROUTE-001] EdgeServer " << serverId << ": No IPv4 module found at host level" << endl;
        }
    } else {
        EV_WARN << "[DEBUG-ROUTE-001] EdgeServer " << serverId << ": No wlan interface found" << endl;
    }
    
    EV_WARN << "=== EDGE SERVER STARTED ===" << endl;
}

void EdgeServerApp::handleStopOperation(inet::LifecycleOperation* operation)
{
    socket.close();
    EV_WARN << "EdgeServer " << serverId << " stopped" << endl;
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
    EV_WARN << "[FLOW-4] EDGESERVER " << serverId << " <- LASPManager: Received packet: " << packet->getName() << " at " << simTime() << endl;
    
    emit(requestsReceived, 1);
    
    // Get client address for response
    auto addressInd = packet->getTag<L3AddressInd>();
    L3Address clientAddr = addressInd->getSrcAddress();
    
    EV_WARN << "[FLOW-4] EDGESERVER " << serverId << " <- LASPManager: Client address: " << clientAddr.str() << endl;
    
    // Check packet name to determine if it's a deployment command or direct service request
    if (strcmp(packet->getName(), "ServiceDeployment") == 0) {
        EV_WARN << "[FLOW-4] EDGESERVER " << serverId << " <- LASPManager: Processing deployment command" << endl;
        // This is a deployment command from LASPManager
        handleDeploymentCommand(packet, clientAddr);
    } else {
        EV_WARN << "[FLOW-4] EDGESERVER " << serverId << " <- LASPManager: Unexpected packet type: " << packet->getName() << endl;
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
    EV_WARN << "EdgeServer " << serverId << " socket closed" << endl;
}

void EdgeServerApp::processServiceRequest(const ServiceRequest& request, const L3Address& clientAddr, int clientPort)
{
    EV_WARN << "EdgeServer " << serverId << " processing request from vehicle " << request.vehicleId << endl;
    
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
    
    EV_WARN << "EdgeServer " << serverId << " processed request, current load: " 
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
    // Extract deployment information
    auto payload = packet->peekData<ApplicationPacket>();
    int vehicleId = payload->getSequenceNumber();
    
    EV_WARN << "[FLOW-5] EDGESERVER " << serverId << " -> VEHICLE " << vehicleId << ": Processing deployment command" << endl;
    
    // Simulate service processing time
    double processingTime = uniform(0.01, 0.05); // 10-50ms processing time
    
    EV_WARN << "[FLOW-5] EDGESERVER " << serverId << " -> VEHICLE " << vehicleId << ": Processing time: " << (processingTime * 1000) << "ms" << endl;
    
    // Update server load
    updateLoad(1.0); // Add some load for this service
    
    // Create service response
    auto response = new Packet("ServiceResponse");
    auto responsePayload = makeShared<ApplicationPacket>();
    responsePayload->setChunkLength(B(300)); // 300 bytes response
    responsePayload->setSequenceNumber(vehicleId);
    response->insertAtBack(responsePayload);
    
    // Send response back to vehicle using the correct port for each vehicle
    std::string addressStr = "192.168.1." + std::to_string(10 + vehicleId);
    L3Address vehicleAddr = L3AddressResolver().resolve(addressStr.c_str());
    int vehiclePort = 5000 + vehicleId; // Each vehicle uses port 5000 + vehicleId
    
    EV_WARN << "[FLOW-5] EDGESERVER " << serverId << " -> VEHICLE " << vehicleId << ": Sending response to " << vehicleAddr.str() << ":" << vehiclePort << endl;
    
    // For now, send immediately (in real implementation, would schedule)
    socket.sendTo(response, vehicleAddr, vehiclePort);
    
    emit(requestsProcessed, 1);
    emit(serverLoadSignal, (currentLoad / computeCapacity) * 100);
    
    EV_WARN << "[FLOW-5] EDGESERVER " << serverId << " -> VEHICLE " << vehicleId << ": Response sent (load: " << (currentLoad / computeCapacity) * 100 << "%)" << endl;
}

void EdgeServerApp::handleDirectServiceRequest(Packet* packet, const L3Address& clientAddr)
{
    // Handle direct service requests (for future use)
    EV_WARN << "EdgeServer " << serverId << " received direct service request" << endl;
    
    // For now, just acknowledge
    auto response = new Packet("DirectServiceResponse");
    socket.sendTo(response, clientAddr, 5000);
}

void EdgeServerApp::finish()
{
    EV_WARN << "EdgeServer " << serverId << " finished. Final load: " 
            << (currentLoad / computeCapacity) * 100 << "%" << endl;
}

} // namespace lasp_ven_simple
