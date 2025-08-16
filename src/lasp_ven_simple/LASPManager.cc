#include "LASPManager.h"
#include "strategies/ThresholdStrategy.h"
#include "strategies/GreedyStrategy.h"
#include "utils/ServicePlacementUtils.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include <cmath>
#include <algorithm>
#include <random>

using namespace omnetpp;
using namespace inet;

namespace lasp_ven_simple {

Define_Module(LASPManager);

LASPManager::LASPManager()
{
    localPort = 9999; // Default port
    evaluationTimer = nullptr;
    EV_WARN << "=== LASP MANAGER CONSTRUCTOR CALLED ===" << endl;
}

LASPManager::~LASPManager()
{
    if (evaluationTimer) {
        cancelAndDelete(evaluationTimer);
    }
}

void LASPManager::initialize(int stage)
{
    EV_WARN << "=== LASP MANAGER INITIALIZING ===" << endl;
    EV_WARN << "Stage: " << stage << endl;
    
    ApplicationBase::initialize(stage);
    
    if (stage == inet::INITSTAGE_LOCAL) {
        // Read parameters
        currentStrategy = par("strategy").stdstringValue();
        evaluationInterval = par("evaluationInterval").doubleValue();
        loadThreshold = par("loadThreshold").doubleValue();
        numEdgeServers = par("numEdgeServers").intValue();
        localPort = par("localPort").intValue();
        
        EV_WARN << "LASPManager parameters loaded:" << endl;
        EV_WARN << "  strategy: " << currentStrategy << endl;
        EV_WARN << "  evaluationInterval: " << evaluationInterval << endl;
        EV_WARN << "  loadThreshold: " << loadThreshold << endl;
        EV_WARN << "  numEdgeServers: " << numEdgeServers << endl;
        EV_WARN << "  localPort: " << localPort << endl;
        
        // Initialize statistics
        requestsReceived = registerSignal("requestsReceived");
        requestsServed = registerSignal("requestsServed");
        averageLatency = registerSignal("averageLatency");
        serverUtilization = registerSignal("serverUtilization");
        
            EV_WARN << "Statistics signals registered successfully" << endl;
    EV_WARN << "=== LASP MANAGER INITIALIZED ===" << endl;
    
    // LASPManager is now starting properly via normal lifecycle
    // No need for forced start anymore
    }
}

void LASPManager::initializeEdgeServers()
{
    EV_WARN << "=== INITIALIZING EDGE SERVERS ===" << endl;
    EV_WARN << "Creating " << numEdgeServers << " edge servers" << endl;
    
    // Create edge servers at strategic locations within road network (0-100m)
    for (int i = 0; i < numEdgeServers; i++) {
        EdgeServer server;
        server.serverId = i;
        
        // Position servers strategically across the road network
        // Road network spans 0-100m in both X and Y directions
        if (numEdgeServers <= 3) {
            // For 3 servers: distribute along main road
            server.latitude = 20.0 + (i * 30.0);  // X: 20, 50, 80 meters
            server.longitude = 50.0;               // Y: center of road network
        } else if (numEdgeServers <= 5) {
            // For 5 servers: grid pattern
            server.latitude = 10.0 + (i % 3) * 40.0;     // X: 10, 50, 90, 10, 50
            server.longitude = 25.0 + (i / 3) * 50.0;    // Y: 25, 25, 25, 75, 75
        } else {
            // For 8 servers: dense grid coverage
            server.latitude = 12.5 + (i % 4) * 25.0;     // X: every 25m
            server.longitude = 25.0 + (i / 4) * 50.0;    // Y: 25m and 75m rows
        }
        server.computeCapacity = 100.0; // 100 GFLOPS
        server.storageCapacity = 1000.0; // 1 TB
        server.currentLoad = 0.0;
        server.isActive = true;
        
        // All servers support all services for simplicity
        server.supportedServices = {TRAFFIC_INFO, EMERGENCY_ALERT, INFOTAINMENT, NAVIGATION};
        
        edgeServers[i] = server;
        EV_WARN << "Edge server " << i << " initialized at (" 
                << server.latitude << ", " << server.longitude << ") meters in road network" << endl;
    }
    
    EV_WARN << "=== EDGE SERVERS INITIALIZED ===" << endl;
}

void LASPManager::handleMessage(cMessage *msg)
{
    if (msg == evaluationTimer) {
        handleEvaluationTimer();
    }
    else {
        socket.processMessage(msg);
    }
}

void LASPManager::handleStartOperation(inet::LifecycleOperation* operation)
{
    EV_WARN << "=== LASP MANAGER STARTING ===" << endl;
    EV_WARN << "LASPManager handleStartOperation called" << endl;
    
    // Application is starting
    EV_WARN << "LASPManager starting..." << endl;
    
    // Setup UDP socket
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort);
    socket.setCallback(this);
    
    EV_WARN << "LASPManager socket setup complete on port " << localPort << endl;
    EV_WARN << "[NETWORK-DEBUG] LASPManager bound to port " << localPort << " and ready to receive" << endl;
    
    // Debug: Check our actual IP address
    EV_WARN << "LASPManager checking network interfaces..." << endl;
    
    // Check for different possible interface names
    auto wlan = getModuleByPath("^.wlan[0]");
    auto eth = getModuleByPath("^.eth");
    auto ppp = getModuleByPath("^.ppp");
    
    if (wlan) {
        EV_WARN << "[DEBUG-ROUTE-001] LASPManager: Found wlan interface" << endl;
        
        // IPv4 module is at host level, not wlan level
        auto ipv4 = getModuleByPath("^.ipv4");
        if (ipv4) {
            EV_WARN << "[DEBUG-ROUTE-001] LASPManager: Found IPv4 module at host level" << endl;
            
            // Check routing table at host level
            auto routingTable = getModuleByPath("^.ipv4.routingTable");
            if (routingTable) {
                auto iroutingTable = check_and_cast<IRoutingTable*>(routingTable);
                EV_WARN << "[DEBUG-ROUTE-001] LASPManager: Routing table has " << iroutingTable->getNumRoutes() << " routes" << endl;
                EV_WARN << "[NETWORK-DEBUG] LASPManager: IPv4 network stack is properly initialized!" << endl;
                for (int i = 0; i < iroutingTable->getNumRoutes(); i++) {
                    auto route = iroutingTable->getRoute(i);
                    if (route) {
                        EV_WARN << "[DEBUG-ROUTE-001] LASPManager: Route " << i << ": " 
                                << route->getDestinationAsGeneric().str() << " -> " << route->getNextHopAsGeneric().str() << endl;
                    }
                }
            } else {
                EV_WARN << "[DEBUG-ROUTE-001] LASPManager: No routing table found" << endl;
            }
        } else {
            EV_WARN << "[DEBUG-ROUTE-001] LASPManager: No IPv4 module found at host level" << endl;
        }
    } else if (eth) {
        EV_WARN << "[DEBUG-ROUTE-001] LASPManager: Found eth interface" << endl;
        auto ipv4 = eth->getSubmodule("ipv4");
        if (ipv4) {
            std::string address = ipv4->par("address").stdstringValue();
            EV_WARN << "[DEBUG-ROUTE-001] LASPManager: Actual IP address: " << address << endl;
        }
    } else if (ppp) {
        EV_WARN << "[DEBUG-ROUTE-001] LASPManager: Found ppp interface" << endl;
        auto ipv4 = ppp->getSubmodule("ipv4");
        if (ipv4) {
            std::string address = ipv4->par("address").stdstringValue();
            EV_WARN << "[DEBUG-ROUTE-001] LASPManager: Actual IP address: " << address << endl;
        }
    } else {
        EV_WARN << "[DEBUG-ROUTE-001] LASPManager: No network interfaces found" << endl;
    }
    
    // Initialize edge servers
    initializeEdgeServers();
    
    // Schedule periodic evaluation
    evaluationTimer = new cMessage("evaluationTimer");
    scheduleAt(simTime() + evaluationInterval, evaluationTimer);
    
    EV_WARN << "Evaluation timer scheduled for " << evaluationInterval << "s" << endl;
    EV_WARN << "=== LASP MANAGER STARTED SUCCESSFULLY ===" << endl;
}

void LASPManager::handleStopOperation(inet::LifecycleOperation* operation)
{
    // Application is stopping
    EV_WARN << "LASPManager stopping..." << endl;
    
    if (evaluationTimer) {
        cancelAndDelete(evaluationTimer);
        evaluationTimer = nullptr;
    }
    
    socket.close();
    
    EV_WARN << "LASPManager stopped." << endl;
}

void LASPManager::handleCrashOperation(inet::LifecycleOperation* operation)
{
    // Application crashed
    EV_WARN << "LASPManager crashed!" << endl;
    
    if (evaluationTimer) {
        cancelAndDelete(evaluationTimer);
        evaluationTimer = nullptr;
    }
    
    socket.destroy();
}

void LASPManager::handleMessageWhenUp(cMessage* msg)
{
    // Handle messages when application is up
    if (msg == evaluationTimer) {
        handleEvaluationTimer();
    }
    else {
        socket.processMessage(msg);
    }
}

void LASPManager::refreshDisplay() const
{
    // Update display string
    char buf[100];
    sprintf(buf, "Strategy: %s", currentStrategy.c_str());
    getDisplayString().setTagArg("t", 0, buf);
}

void LASPManager::socketDataArrived(UdpSocket *socket, Packet *packet)
{
    EV_WARN << "[FLOW-2] LASPManager <- VEHICLE: Received packet at " << simTime() << endl;
    
    // Extract vehicle service request from packet
    try {
        auto payload = packet->peekData<ApplicationPacket>();
        int vehicleId = payload->getSequenceNumber();
        
        EV_WARN << "[FLOW-2] LASPManager <- VEHICLE: Parsed packet from vehicle " << vehicleId << endl;
        
        // Create service request from received packet
        ServiceRequest request;
        request.vehicleId = vehicleId;
        request.serviceType = TRAFFIC_INFO; // Could be extracted from packet in future
        request.timestamp = simTime().dbl();
        request.latitude = 52.5200; // Could be extracted from vehicle position
        request.longitude = 13.4050;
        request.priority = 1;
        request.deadline = simTime().dbl() + 10.0; // 10 seconds deadline
        request.dataSize = 1.0; // 1 MB
        
        EV_WARN << "[FLOW-2] LASPManager <- VEHICLE: Request from vehicle " << vehicleId << " processed" << endl;
        
        emit(requestsReceived, 1);
        processServiceRequest(request);
        
    } catch (const std::exception& e) {
        EV_WARN << "[FLOW-2] LASPManager <- VEHICLE: Failed to parse packet: " << e.what() << endl;
    }
    
    delete packet;
}

void LASPManager::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    EV_WARN << "UDP error: " << indication->getName() << endl;
    delete indication;
}

void LASPManager::socketClosed(UdpSocket *socket)
{
    EV_WARN << "UDP socket closed" << endl;
}

void LASPManager::processServiceRequest(const ServiceRequest& request)
{
            EV_WARN << "[FLOW-3] LASPManager -> EDGESERVER: Processing request from vehicle " << request.vehicleId << endl;
    
    ServicePlacement* placement = findBestPlacement(request);
    if (placement) {
        EV_WARN << "[FLOW-3] LASPManager -> EDGESERVER: Found placement on server " << placement->serverId << " (latency: " << placement->estimatedLatency << "ms)" << endl;
        
        activePlacements.push_back(*placement);
        emit(requestsServed, 1);
        emit(averageLatency, placement->estimatedLatency);
        
        // Send deployment command to selected edge server
        EV_WARN << "[FLOW-3] LASPManager -> EDGESERVER: Sending deployment command to server " << placement->serverId << endl;
        sendDeploymentCommand(*placement, request);
        
        EV_WARN << "[FLOW-3] LASPManager -> EDGESERVER: Deployment command sent to server " << placement->serverId << endl;
    }
    else {
        EV_WARN << "[FLOW-3] LASPManager -> EDGESERVER: No suitable server found for vehicle " << request.vehicleId << endl;
    }
}

ServicePlacement* LASPManager::findBestPlacement(const ServiceRequest& request)
{
    if (currentStrategy == "threshold") {
        return ThresholdStrategy::placeService(request, edgeServers, loadThreshold);
    }
    else if (currentStrategy == "greedy") {
        return GreedyStrategy::placeService(request, edgeServers);
    }
    else {
        EV_ERROR << "Unknown strategy: " << currentStrategy << endl;
        return nullptr;
    }
}

void LASPManager::updateServerLoad()
{
    // Update server loads based on active placements
    for (auto& server : edgeServers) {
        server.second.currentLoad = 0.0;
    }
    
    for (const auto& placement : activePlacements) {
        if (edgeServers.find(placement.serverId) != edgeServers.end()) {
            edgeServers[placement.serverId].currentLoad += placement.resourceUsage;
        }
    }
}

void LASPManager::evaluateCurrentPlacements()
{
    updateServerLoad();
    
    // Calculate average server utilization
    double totalUtilization = 0.0;
    for (const auto& server : edgeServers) {
        totalUtilization += server.second.currentLoad / server.second.computeCapacity;
    }
    double avgUtilization = totalUtilization / edgeServers.size();
    emit(serverUtilization, avgUtilization);
    
    // Reduced frequency logging to save tokens
    static int utilizationLogCounter = 0;
    if (++utilizationLogCounter % 20 == 0) {  // Log every 20th evaluation
        EV_WARN << "[UTIL] Server utilization: " << (avgUtilization * 100) << "%" << endl;
    }
}

void LASPManager::handleEvaluationTimer()
{
    evaluateCurrentPlacements();
    
    // Reschedule timer
    scheduleAt(simTime() + evaluationInterval, evaluationTimer);
}

ServicePlacement* LASPManager::thresholdBasedPlacement(const ServiceRequest& request)
{
    return ThresholdStrategy::placeService(request, edgeServers, loadThreshold);
}

ServicePlacement* LASPManager::greedyPlacement(const ServiceRequest& request)
{
    return GreedyStrategy::placeService(request, edgeServers);
}

bool LASPManager::canServerHandleRequest(const EdgeServer& server, const ServiceRequest& request)
{
    // Check if server supports the service type
    bool supportsService = false;
    for (ServiceType service : server.supportedServices) {
        if (service == request.serviceType) {
            supportsService = true;
            break;
        }
    }
    
    if (!supportsService) return false;
    
    // Check if server has capacity
    double requiredCapacity = request.dataSize * 0.1; // Simple capacity calculation
    return (server.currentLoad + requiredCapacity) <= server.computeCapacity;
}

void LASPManager::submitServiceRequest(const ServiceRequest& request)
{
    pendingRequests.push_back(request);
    processServiceRequest(request);
}

void LASPManager::removeServiceRequest(int requestId)
{
    // Remove from pending requests
    pendingRequests.erase(
        std::remove_if(pendingRequests.begin(), pendingRequests.end(),
            [requestId](const ServiceRequest& req) { return req.vehicleId == requestId; }),
        pendingRequests.end()
    );
    
    // Remove from active placements
    activePlacements.erase(
        std::remove_if(activePlacements.begin(), activePlacements.end(),
            [requestId](const ServicePlacement& placement) { return placement.serviceId == requestId; }),
        activePlacements.end()
    );
}

void LASPManager::sendDeploymentCommand(const ServicePlacement& placement, const ServiceRequest& request)
{
    EV_WARN << "=== SENDING DEPLOYMENT COMMAND ===" << endl;
    EV_WARN << "Sending deployment command to EdgeServer " << placement.serverId 
            << " for vehicle " << request.vehicleId << endl;
    
    // Create deployment command packet
    auto packet = new Packet("ServiceDeployment");
    auto payload = makeShared<ApplicationPacket>();
    payload->setChunkLength(B(150)); // 150 bytes
    payload->setSequenceNumber(request.vehicleId);
    packet->insertAtBack(payload);
    
    // Add placement information as tags (in real implementation, would use proper message format)
    packet->addTag<CreationTimeTag>()->setCreationTime(simTime());
    
    // Send to selected edge server  
    std::string addressStr = "192.168.1." + std::to_string(200 + placement.serverId);
    L3Address edgeServerAddress = L3AddressResolver().resolve(addressStr.c_str());
    int edgeServerPort = 8000 + placement.serverId; // Each server has unique port
    
    EV_WARN << "EdgeServer address: " << edgeServerAddress.str() << ":" << edgeServerPort << endl;
    
    socket.sendTo(packet, edgeServerAddress, edgeServerPort);
    
    EV_WARN << "Deployment command sent successfully" << endl;
    EV_WARN << "=== DEPLOYMENT COMMAND SENT ===" << endl;
}

void LASPManager::finish()
{
    ApplicationBase::finish();
    
    EV_WARN << "LASPManager finished. Total requests served: " 
            << activePlacements.size() << endl;
}

} // namespace lasp_ven_simple 