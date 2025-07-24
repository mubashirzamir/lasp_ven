#include "LASPManager.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include <cmath>
#include <algorithm>
#include <random>

using namespace omnetpp;
using namespace inet;

namespace lasp_ven {

Define_Module(LASPManager);

LASPManager::LASPManager()
{
    localPort = 9999; // Default port
}

LASPManager::~LASPManager()
{
}

void LASPManager::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        // Read parameters
        currentStrategy = par("strategy").stdstringValue();
        evaluationInterval = par("evaluationInterval").doubleValue();
        maxServiceLatency = par("maxServiceLatency").doubleValue();
        numEdgeServers = par("numEdgeServers").intValue();
        localPort = par("localPort").intValue();
        
        // Initialize statistics
        requestsReceived = registerSignal("requestsReceived");
        requestsServed = registerSignal("requestsServed");
        averageLatency = registerSignal("averageLatency");
        serverUtilization = registerSignal("serverUtilization");
        
        EV_INFO << "LASPManager initialized with strategy: " << currentStrategy << endl;
    }
    else if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        // Setup UDP socket
        socket.setOutputGate(gate("socketOut"));
        socket.bind(localPort);
        socket.setCallback(this);
        
        // Initialize edge servers
        initializeEdgeServers();
        
        // Schedule periodic evaluation
        cMessage *evalTimer = new cMessage("evaluationTimer");
        scheduleAt(simTime() + evaluationInterval, evalTimer);
        
        EV_INFO << "LASPManager network setup complete. Listening on port " << localPort << endl;
    }
}

void LASPManager::initializeEdgeServers()
{
    // Create edge servers with random positions for demonstration
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> latDist(37.0, 38.0); // Sample coordinates
    std::uniform_real_distribution<> lonDist(-122.5, -121.5);
    std::uniform_real_distribution<> capacityDist(50.0, 200.0); // GFLOPS
    std::uniform_real_distribution<> storageDist(100.0, 1000.0); // GB
    
    for (int i = 0; i < numEdgeServers; i++) {
        EdgeServer server;
        server.serverId = i;
        server.latitude = latDist(gen);
        server.longitude = lonDist(gen);
        server.computeCapacity = capacityDist(gen);
        server.storageCapacity = storageDist(gen);
        server.currentLoad = 0.0;
        server.isActive = true;
        
        // All servers support all services for now
        server.supportedServices = {TRAFFIC_INFO, EMERGENCY_ALERT, INFOTAINMENT, NAVIGATION};
        
        edgeServers[i] = server;
        
        EV_INFO << "Created edge server " << i << " at (" << server.latitude 
                << ", " << server.longitude << ") with capacity " << server.computeCapacity << " GFLOPS" << endl;
    }
}

void LASPManager::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (strcmp(msg->getName(), "evaluationTimer") == 0) {
            evaluateCurrentPlacements();
            updateServerLoad();
            
            // Reschedule
            scheduleAt(simTime() + evaluationInterval, msg);
            return;
        }
    }
    
    // Handle network messages
    if (msg->getArrivalGate() && strcmp(msg->getArrivalGate()->getName(), "socketIn") == 0) {
        // This will be handled by socketDataArrived callback
        return;
    }
    
    delete msg;
}

void LASPManager::socketDataArrived(UdpSocket *socket, Packet *packet)
{
    emit(requestsReceived, 1);
    
    // Extract service request from packet
    // For now, create a dummy request
    ServiceRequest request;
    request.vehicleId = intrand(1000); // Random vehicle ID for demo
    request.serviceType = static_cast<ServiceType>(intrand(4) + 1);
    request.timestamp = simTime().dbl();
    request.latitude = 37.5 + dblrand() * 0.5; // Random position
    request.longitude = -122.0 + dblrand() * 0.5;
    request.priority = intrand(5) + 1;
    request.deadline = simTime().dbl() + uniform(1.0, 10.0);
    request.dataSize = uniform(0.1, 5.0); // MB
    
    EV_INFO << "Received service request from vehicle " << request.vehicleId 
            << " for service type " << request.serviceType << endl;
    
    processServiceRequest(request);
    delete packet;
}

void LASPManager::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    EV_WARN << "Socket error: " << indication->str() << endl;
    delete indication;
}

void LASPManager::socketClosed(UdpSocket *socket)
{
    EV_INFO << "Socket closed" << endl;
}

void LASPManager::processServiceRequest(const ServiceRequest& request)
{
    ServicePlacement* placement = findBestPlacement(request);
    
    if (placement) {
        activePlacements.push_back(*placement);
        edgeServers[placement->serverId].currentLoad += request.dataSize;
        
        emit(requestsServed, 1);
        emit(averageLatency, placement->estimatedLatency);
        
        EV_INFO << "Service request " << request.vehicleId << " placed on server " 
                << placement->serverId << " with latency " << placement->estimatedLatency << "ms" << endl;
        
        delete placement;
    } else {
        EV_WARN << "Could not find suitable placement for request from vehicle " << request.vehicleId << endl;
        pendingRequests.push_back(request);
    }
}

ServicePlacement* LASPManager::findBestPlacement(const ServiceRequest& request)
{
    if (currentStrategy == "threshold") {
        return thresholdBasedPlacement(request);
    } else if (currentStrategy == "greedy") {
        return greedyPlacement(request);
    } else {
        // Default to greedy
        return greedyPlacement(request);
    }
}

ServicePlacement* LASPManager::thresholdBasedPlacement(const ServiceRequest& request)
{
    double loadThreshold = 0.8; // 80% load threshold
    
    ServicePlacement* bestPlacement = nullptr;
    double bestLatency = std::numeric_limits<double>::max();
    
    for (auto& [serverId, server] : edgeServers) {
        if (!server.isActive) continue;
        if (server.currentLoad / server.computeCapacity > loadThreshold) continue;
        if (!canServerHandleRequest(server, request)) continue;
        
        double latency = estimateLatency(request, server);
        if (latency < bestLatency && latency <= maxServiceLatency) {
            bestLatency = latency;
            
            if (bestPlacement) delete bestPlacement;
            bestPlacement = new ServicePlacement();
            bestPlacement->serviceId = request.vehicleId;
            bestPlacement->serverId = serverId;
            bestPlacement->serviceType = request.serviceType;
            bestPlacement->placementTime = simTime().dbl();
            bestPlacement->estimatedLatency = latency;
            bestPlacement->resourceUsage = request.dataSize;
        }
    }
    
    return bestPlacement;
}

ServicePlacement* LASPManager::greedyPlacement(const ServiceRequest& request)
{
    ServicePlacement* bestPlacement = nullptr;
    double bestLatency = std::numeric_limits<double>::max();
    
    for (auto& [serverId, server] : edgeServers) {
        if (!server.isActive) continue;
        if (!canServerHandleRequest(server, request)) continue;
        
        double latency = estimateLatency(request, server);
        if (latency < bestLatency && latency <= maxServiceLatency) {
            bestLatency = latency;
            
            if (bestPlacement) delete bestPlacement;
            bestPlacement = new ServicePlacement();
            bestPlacement->serviceId = request.vehicleId;
            bestPlacement->serverId = serverId;
            bestPlacement->serviceType = request.serviceType;
            bestPlacement->placementTime = simTime().dbl();
            bestPlacement->estimatedLatency = latency;
            bestPlacement->resourceUsage = request.dataSize;
        }
    }
    
    return bestPlacement;
}

double LASPManager::calculateDistance(double lat1, double lon1, double lat2, double lon2)
{
    const double R = 6371000; // Earth radius in meters
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = sin(dLat/2) * sin(dLat/2) + cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) * sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return R * c;
}

double LASPManager::estimateLatency(const ServiceRequest& request, const EdgeServer& server)
{
    double distance = calculateDistance(request.latitude, request.longitude, server.latitude, server.longitude);
    
    // Simple latency model: propagation delay + processing delay + queueing delay
    double propagationDelay = distance / 200000000.0 * 1000; // Speed of light in fiber, convert to ms
    double processingDelay = request.dataSize / (server.computeCapacity / 10.0); // Simplified processing time
    double queueingDelay = server.currentLoad / server.computeCapacity * 10.0; // Load-based queueing
    
    return propagationDelay + processingDelay + queueingDelay;
}

bool LASPManager::canServerHandleRequest(const EdgeServer& server, const ServiceRequest& request)
{
    // Check if server supports the service type
    auto it = std::find(server.supportedServices.begin(), server.supportedServices.end(), request.serviceType);
    if (it == server.supportedServices.end()) {
        return false;
    }
    
    // Check if server has enough resources
    if (server.currentLoad + request.dataSize > server.computeCapacity) {
        return false;
    }
    
    return true;
}

void LASPManager::updateServerLoad()
{
    // Decay server loads over time (services completing)
    for (auto& [serverId, server] : edgeServers) {
        server.currentLoad = std::max(0.0, server.currentLoad * 0.95); // 5% decay per evaluation
        emit(serverUtilization, server.currentLoad / server.computeCapacity);
    }
    
    // Remove old placements
    auto now = simTime().dbl();
    activePlacements.erase(
        std::remove_if(activePlacements.begin(), activePlacements.end(),
            [now](const ServicePlacement& p) { return now - p.placementTime > 10.0; }),
        activePlacements.end());
}

void LASPManager::evaluateCurrentPlacements()
{
    EV_INFO << "Evaluating current placements. Active: " << activePlacements.size() 
            << ", Pending: " << pendingRequests.size() << endl;
    
    // Try to place pending requests
    auto it = pendingRequests.begin();
    while (it != pendingRequests.end()) {
        ServicePlacement* placement = findBestPlacement(*it);
        if (placement) {
            activePlacements.push_back(*placement);
            edgeServers[placement->serverId].currentLoad += it->dataSize;
            emit(requestsServed, 1);
            emit(averageLatency, placement->estimatedLatency);
            delete placement;
            it = pendingRequests.erase(it);
        } else {
            ++it;
        }
    }
}

void LASPManager::submitServiceRequest(const ServiceRequest& request)
{
    processServiceRequest(request);
}

void LASPManager::removeServiceRequest(int requestId)
{
    // Remove from pending requests
    pendingRequests.erase(
        std::remove_if(pendingRequests.begin(), pendingRequests.end(),
            [requestId](const ServiceRequest& r) { return r.vehicleId == requestId; }),
        pendingRequests.end());
}

void LASPManager::finish()
{
    EV_INFO << "LASPManager simulation finished" << endl;
    EV_INFO << "Total active placements: " << activePlacements.size() << endl;
    EV_INFO << "Total pending requests: " << pendingRequests.size() << endl;
    
    // Calculate final statistics
    double totalUtilization = 0.0;
    for (const auto& [serverId, server] : edgeServers) {
        totalUtilization += server.currentLoad / server.computeCapacity;
    }
    double avgUtilization = totalUtilization / edgeServers.size();
    
    recordScalar("Final Average Server Utilization", avgUtilization);
    recordScalar("Final Active Placements", (double)activePlacements.size());
    recordScalar("Final Pending Requests", (double)pendingRequests.size());
}

} // namespace lasp_ven
