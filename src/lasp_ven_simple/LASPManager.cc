#include "LASPManager.h"
#include "strategies/ThresholdStrategy.h"
#include "strategies/GreedyStrategy.h"
#include "utils/ServicePlacementUtils.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/applications/base/ApplicationPacket_m.h"
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
}

LASPManager::~LASPManager()
{
    if (evaluationTimer) {
        cancelAndDelete(evaluationTimer);
    }
}

void LASPManager::initialize(int stage)
{
    ApplicationBase::initialize(stage);
    
    if (stage == inet::INITSTAGE_LOCAL) {
        // Read parameters
        currentStrategy = par("strategy").stdstringValue();
        evaluationInterval = par("evaluationInterval").doubleValue();
        loadThreshold = par("loadThreshold").doubleValue();
        numEdgeServers = par("numEdgeServers").intValue();
        localPort = par("localPort").intValue();
        
        // Initialize statistics
        requestsReceived = registerSignal("requestsReceived");
        requestsServed = registerSignal("requestsServed");
        averageLatency = registerSignal("averageLatency");
        serverUtilization = registerSignal("serverUtilization");
        
        EV_INFO << "LASPManager initialized with strategy: " << currentStrategy << endl;
    }
}

void LASPManager::initializeEdgeServers()
{
    // Create edge servers at fixed locations
    for (int i = 0; i < numEdgeServers; i++) {
        EdgeServer server;
        server.serverId = i;
        server.latitude = 52.5200 + (i * 0.01); // Berlin area
        server.longitude = 13.4050 + (i * 0.01);
        server.computeCapacity = 100.0; // 100 GFLOPS
        server.storageCapacity = 1000.0; // 1 TB
        server.currentLoad = 0.0;
        server.isActive = true;
        
        // All servers support all services for simplicity
        server.supportedServices = {TRAFFIC_INFO, EMERGENCY_ALERT, INFOTAINMENT, NAVIGATION};
        
        edgeServers[i] = server;
        EV_INFO << "Edge server " << i << " initialized at (" 
                << server.latitude << ", " << server.longitude << ")" << endl;
    }
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
    // Application is starting
    EV_INFO << "LASPManager starting..." << endl;
    
    // Setup UDP socket
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort);
    socket.setCallback(this);
    
    // Initialize edge servers
    initializeEdgeServers();
    
    // Schedule periodic evaluation
    evaluationTimer = new cMessage("evaluationTimer");
    scheduleAt(simTime() + evaluationInterval, evaluationTimer);
    
    EV_INFO << "LASPManager started. Listening on port " << localPort << endl;
}

void LASPManager::handleStopOperation(inet::LifecycleOperation* operation)
{
    // Application is stopping
    EV_INFO << "LASPManager stopping..." << endl;
    
    if (evaluationTimer) {
        cancelAndDelete(evaluationTimer);
        evaluationTimer = nullptr;
    }
    
    socket.close();
    
    EV_INFO << "LASPManager stopped." << endl;
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
    // Process incoming service requests
    // For now, we'll create a simple service request
    ServiceRequest request;
    request.vehicleId = 1; // Default vehicle ID
    request.serviceType = TRAFFIC_INFO; // Default service
    request.timestamp = simTime().dbl();
    request.latitude = 52.5200; // Default location
    request.longitude = 13.4050;
    request.priority = 1;
    request.deadline = simTime().dbl() + 10.0; // 10 seconds deadline
    request.dataSize = 1.0; // 1 MB
    
    emit(requestsReceived, 1);
    processServiceRequest(request);
    
    delete packet;
}

void LASPManager::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    EV_WARN << "UDP error: " << indication->getName() << endl;
    delete indication;
}

void LASPManager::socketClosed(UdpSocket *socket)
{
    EV_INFO << "UDP socket closed" << endl;
}

void LASPManager::processServiceRequest(const ServiceRequest& request)
{
    EV_INFO << "Processing service request from vehicle " << request.vehicleId << endl;
    
    ServicePlacement* placement = findBestPlacement(request);
    if (placement) {
        activePlacements.push_back(*placement);
        emit(requestsServed, 1);
        emit(averageLatency, placement->estimatedLatency);
        
        EV_INFO << "Service placed on server " << placement->serverId 
                << " with latency " << placement->estimatedLatency << "ms" << endl;
    }
    else {
        EV_WARN << "No suitable server found for request from vehicle " << request.vehicleId << endl;
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
    
    EV_INFO << "Current server utilization: " << (avgUtilization * 100) << "%" << endl;
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

void LASPManager::finish()
{
    ApplicationBase::finish();
    
    EV_INFO << "LASPManager finished. Total requests served: " 
            << activePlacements.size() << endl;
}

} // namespace lasp_ven_simple 