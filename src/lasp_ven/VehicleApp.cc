#include "VehicleApp.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "veins_inet/VeinsInetMobility.h"

using namespace omnetpp;
using namespace inet;

namespace lasp_ven {

Define_Module(VehicleApp);

VehicleApp::VehicleApp()
{
    vehicleId = 0;
    lastRequestTime = 0.0;
    requestInterval = 5.0; // Default 5 seconds
    requestCounter = 0;
    laspManagerPort = 9999;
    currentLatitude = 0.0;
    currentLongitude = 0.0;
}

VehicleApp::~VehicleApp()
{
}

void VehicleApp::initialize(int stage)
{
    VeinsInetApplicationBase::initialize(stage);
    
    if (stage == inet::INITSTAGE_LOCAL) {
        // Read parameters
        vehicleId = par("vehicleId").intValue();
        requestInterval = par("requestInterval").doubleValue();
        laspManagerPort = par("laspManagerPort").intValue();
        
        // Initialize preferred services (random for demo)
        preferredServices.push_back(TRAFFIC_INFO);
        preferredServices.push_back(NAVIGATION);
        if (dblrand() > 0.5) preferredServices.push_back(INFOTAINMENT);
        if (dblrand() > 0.8) preferredServices.push_back(EMERGENCY_ALERT);
        
        // Initialize statistics
        requestsSent = registerSignal("requestsSent");
        responsesReceived = registerSignal("responsesReceived");
        serviceLatency = registerSignal("serviceLatency");
        
        EV_INFO << "VehicleApp " << vehicleId << " initialized" << endl;
    }
    else if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        // Setup UDP socket
        socket.setOutputGate(gate("socketOut"));
        socket.bind(L3Address(), 0); // Bind to any available port
        
        // Come back to this.
//        socket.setCallback(this);

        // Find LASP Manager
        try {
            laspManagerAddress = L3AddressResolver().resolve("laspManager");
        } catch (const std::exception& e) {
            // Fallback to localhost for testing
            laspManagerAddress = L3AddressResolver().resolve("127.0.0.1");
        }
        
        // Schedule first service request
        cMessage *requestTimer = new cMessage("serviceRequestTimer");
        scheduleAt(simTime() + uniform(1.0, requestInterval), requestTimer);
        
        EV_INFO << "VehicleApp " << vehicleId << " network setup complete" << endl;
    }
}

void VehicleApp::handleMessage(cMessage* msg)
{
    if (msg->isSelfMessage()) {
        if (strcmp(msg->getName(), "serviceRequestTimer") == 0) {
            updatePosition();
            sendServiceRequest();
            
            // Schedule next request
            scheduleAt(simTime() + exponential(requestInterval), msg);
            return;
        }
    }
    
    // Handle network messages (UDP callback will be called)
    VeinsInetApplicationBase::handleMessage(msg);
}

void VehicleApp::updatePosition()
{
    // Get current position from mobility module
    auto mobility = check_and_cast<veins::VeinsInetMobility*>(getParentModule()->getSubmodule("mobility"));
    if (mobility) {
//        auto coord = mobility->getPositionAt(simTime());
        auto coord = mobility->getCurrentPosition();
        currentLatitude = coord.x / 100000.0 + 37.0; // Convert to realistic coordinates
        currentLongitude = coord.y / 100000.0 - 122.0;
        
        EV_DEBUG << "Vehicle " << vehicleId << " position updated to (" 
                 << currentLatitude << ", " << currentLongitude << ")" << endl;
    } else {
        // Fallback: use random movement for testing
        currentLatitude += uniform(-0.001, 0.001);
        currentLongitude += uniform(-0.001, 0.001);
    }
}

void VehicleApp::sendServiceRequest()
{
    if (!socket.isOpen()) {
        EV_WARN << "Socket not open, cannot send request" << endl;
        return;
    }
    
    requestCounter++;
    
    // Create service request packet
    auto packet = new Packet("ServiceRequest");
    auto payload = makeShared<ApplicationPacket>();
    
    // Simple payload with vehicle info (in real implementation, use proper serialization)
    std::string requestData = "VehicleID:" + std::to_string(vehicleId) + 
                             ",Service:" + std::to_string(selectRandomService()) +
                             ",Lat:" + std::to_string(currentLatitude) +
                             ",Lon:" + std::to_string(currentLongitude) +
                             ",Time:" + std::to_string(simTime().dbl()) +
                             ",Counter:" + std::to_string(requestCounter);
    
//    payload->setPayload(requestData.c_str());
    payload->setChunkLength(B(requestData.length()));
    packet->insertAtBack(payload);
    
    // Send to LASP Manager
    socket.sendTo(packet, laspManagerAddress, laspManagerPort);
    
    emit(requestsSent, 1);
    lastRequestTime = simTime().dbl();
    
    EV_INFO << "Vehicle " << vehicleId << " sent service request #" << requestCounter 
            << " to LASP Manager" << endl;
}

ServiceType VehicleApp::selectRandomService()
{
    if (preferredServices.empty()) {
        return static_cast<ServiceType>(intrand(4) + 1);
    }
    
    int index = intrand(preferredServices.size());
    return preferredServices[index];
}

void VehicleApp::socketDataArrived(UdpSocket *socket, Packet *packet)
{
    emit(responsesReceived, 1);
    
    double latency = simTime().dbl() - lastRequestTime;
    emit(serviceLatency, latency);
    
    EV_INFO << "Vehicle " << vehicleId << " received service response with latency " 
            << latency * 1000 << "ms" << endl;
    
    delete packet;
}

void VehicleApp::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    EV_WARN << "Vehicle " << vehicleId << " socket error: " << indication->str() << endl;
    delete indication;
}

void VehicleApp::socketClosed(UdpSocket *socket)
{
    EV_INFO << "Vehicle " << vehicleId << " socket closed" << endl;
}

void VehicleApp::receiveSignal(cComponent* source, simsignal_t signalID, cObject* obj, cObject* details)
{
//    VeinsInetApplicationBase::receiveSignal(source, signalID, obj, details);
//
//    // Handle Veins signals (position updates, etc.)
//    if (signalID == mobilityStateChangedSignal) {
//        updatePosition();
//    }
}

void VehicleApp::finish()
{
    VeinsInetApplicationBase::finish();
    
    EV_INFO << "VehicleApp " << vehicleId << " finished" << endl;
    EV_INFO << "Total requests sent: " << requestCounter << endl;
    
    recordScalar("Total Requests Sent", (double)requestCounter);
}

} // namespace lasp_ven
