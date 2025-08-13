#include "VehicleServiceApp.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/TimeTag_m.h"
#include <random>

using namespace omnetpp;
using namespace inet;

namespace lasp_ven_simple {

Define_Module(VehicleServiceApp);

VehicleServiceApp::VehicleServiceApp()
{
    laspManagerPort = 15000; // Use port 15000 to avoid conflicts
    serviceRequestInterval = 10.0; // Default 10 seconds
    requestCounter = 0;
    maxRequests = 5; // Limit requests per vehicle
    requestSize = 200; // Default 200 bytes
}

VehicleServiceApp::~VehicleServiceApp()
{
    // Timer cleanup handled by Veins TimerManager
}

bool VehicleServiceApp::startApplication()
{
    EV_WARN << "=== VEHICLE SERVICE APP STARTING ===" << endl;
    EV_WARN << "Vehicle index: " << getParentModule()->getIndex() << endl;
    
    // Don't call parent's startApplication() to avoid accident simulation
    // Instead, call the grandparent (VeinsInetApplicationBase) directly
    bool result = VeinsInetApplicationBase::startApplication();
    
    EV_WARN << "Parent startApplication() result: " << result << endl;
    
    // Initialize service request functionality
    serviceRequestsSent = registerSignal("serviceRequestsSent");
    serviceResponsesReceived = registerSignal("serviceResponsesReceived");
    serviceLatency = registerSignal("serviceLatency");
    
    EV_WARN << "Signals registered successfully" << endl;
    
    // Get configurable parameters
    requestSize = par("requestSize");
    maxRequests = par("maxRequests");
    serviceRequestInterval = par("serviceRequestInterval");
    
    EV_WARN << "Parameters loaded - requestSize: " << requestSize 
            << ", maxRequests: " << maxRequests 
            << ", serviceRequestInterval: " << serviceRequestInterval << endl;
    
    // Setup service socket
    serviceSocket.setOutputGate(gate("socketOut"));
    serviceSocket.bind(5000); // Bind to port 5000 for EdgeServer responses
    serviceSocket.setCallback(this);
    
    EV_WARN << "Service socket setup complete" << endl;
    
    // Resolve LASP Manager address
    laspManagerAddress = L3AddressResolver().resolve("192.168.1.100");
    
    EV_WARN << "LASP Manager address resolved: " << laspManagerAddress.str() << endl;
    
    // Schedule first service request using Veins timer system
    scheduleNextServiceRequest();
    
    EV_WARN << "=== VEHICLE SERVICE APP STARTED SUCCESSFULLY ===" << endl;
    
    return result;
}

void VehicleServiceApp::processPacket(std::shared_ptr<inet::Packet> pk)
{
    // Ignore accident messages and other VeinsInetSampleApplication packets
    // Our vehicles should only process service response packets via socketDataArrived
    EV_INFO << "Vehicle " << getParentModule()->getIndex() 
            << " ignoring VeinsInetSampleApplication packet for VEN testing" << endl;
    
    // Delete the packet without processing
    delete pk.get();
}

bool VehicleServiceApp::stopApplication()
{
    serviceSocket.close();
    
    // Call parent cleanup (TimerManager cleanup handled automatically)
    return VeinsInetSampleApplication::stopApplication();
}

void VehicleServiceApp::scheduleNextServiceRequest()
{
    EV_WARN << "Vehicle " << getParentModule()->getIndex() 
            << " scheduleNextServiceRequest() called, requestCounter: " << requestCounter 
            << ", maxRequests: " << maxRequests << endl;
    
    if (requestCounter < maxRequests) {
        // Use Veins timer system instead of OMNeT++ timers
        auto callback = [this]() {
            EV_WARN << "Vehicle " << getParentModule()->getIndex() 
                    << " timer callback triggered, calling sendServiceRequest()" << endl;
            sendServiceRequest();
            scheduleNextServiceRequest(); // Schedule next one
        };
        
        double delay = uniform(1.0, 3.0) + 5.0; // Reduced delay for testing
        timerManager.create(veins::TimerSpecification(callback).oneshotIn(SimTime(delay, SIMTIME_S)));
        
        EV_WARN << "Vehicle " << getParentModule()->getIndex() 
                << " scheduled next service request in " << delay << "s" << endl;
    } else {
        EV_WARN << "Vehicle " << getParentModule()->getIndex() 
                << " max requests reached, not scheduling more" << endl;
    }
}

void VehicleServiceApp::sendServiceRequest()
{
    EV_WARN << "=== VEHICLE SENDING SERVICE REQUEST ===" << endl;
    EV_WARN << "Vehicle " << getParentModule()->getIndex() 
            << " sendServiceRequest() called, requestCounter: " << requestCounter 
            << ", maxRequests: " << maxRequests << endl;
    
    if (requestCounter >= maxRequests) {
        EV_WARN << "Vehicle " << getParentModule()->getIndex() 
                << " max requests reached, not sending" << endl;
        return;
    }
    
    // Create service request packet
    auto packet = new Packet("VehicleServiceRequest");
    auto payload = makeShared<ApplicationPacket>();
    payload->setChunkLength(B(requestSize)); // Configurable request size
    payload->setSequenceNumber(getParentModule()->getIndex()); // Use vehicle index as ID
    packet->insertAtBack(payload);
    
    // Add request metadata (in real implementation, would use proper message format)
    packet->addTag<CreationTimeTag>()->setCreationTime(simTime());
    
    // Track request for latency measurement  
    int vehicleId = getParentModule()->getIndex();
    pendingRequests[vehicleId] = simTime();
    
    EV_WARN << "Vehicle " << getParentModule()->getIndex() 
            << " sending packet to LASP Manager at " << laspManagerAddress.str() 
            << ":" << laspManagerPort << endl;
    
    // Send to LASP Manager
    serviceSocket.sendTo(packet, laspManagerAddress, laspManagerPort);
    
    emit(serviceRequestsSent, 1);
    requestCounter++;
    
    ServiceType service = selectServiceBasedOnContext();
    EV_WARN << "Vehicle " << getParentModule()->getIndex() 
            << " sent service request " << requestCounter 
            << " for service type " << service << endl;
    EV_WARN << "=== SERVICE REQUEST SENT SUCCESSFULLY ===" << endl;
}

ServiceType VehicleServiceApp::selectServiceBasedOnContext()
{
    // Simple context-based service selection
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1, 4);
    
    // In a real scenario, this could be based on:
    // - Vehicle speed (high speed = navigation, low speed = infotainment)
    // - Time of day (rush hour = traffic info)
    // - Location (accident area = emergency alerts)
    
    return static_cast<ServiceType>(dis(gen));
}

void VehicleServiceApp::socketDataArrived(UdpSocket *socket, Packet *packet)
{
    if (socket == &serviceSocket) {
        // This is a service response from EdgeServer
        try {
            auto payload = packet->peekData<ApplicationPacket>();
            int sequenceNumber = payload->getSequenceNumber();
            
            // Calculate latency if we have the original request time
            if (pendingRequests.find(sequenceNumber) != pendingRequests.end()) {
                simtime_t latency = simTime() - pendingRequests[sequenceNumber];
                emit(serviceLatency, latency.dbl());
                pendingRequests.erase(sequenceNumber);
                
                EV_INFO << "Vehicle " << getParentModule()->getIndex() 
                        << " received service response with latency " 
                        << latency.dbl() * 1000 << "ms" << endl;
                        
                emit(serviceResponsesReceived, 1);
            }
        } catch (const std::exception& e) {
            EV_WARN << "Failed to parse service response packet: " << e.what() << endl;
        }
        delete packet;
    } else {
        // This is the parent's socket - delegate to parent class
        VeinsInetSampleApplication::socketDataArrived(socket, packet);
    }
}

void VehicleServiceApp::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    if (socket == &serviceSocket) {
        EV_WARN << "Vehicle " << getParentModule()->getIndex() 
                << " service socket error: " << indication->getName() << endl;
        delete indication;
    } else {
        VeinsInetSampleApplication::socketErrorArrived(socket, indication);
    }
}

void VehicleServiceApp::socketClosed(UdpSocket *socket)
{
    if (socket == &serviceSocket) {
        EV_INFO << "Vehicle " << getParentModule()->getIndex() 
                << " service socket closed" << endl;
    } else {
        VeinsInetSampleApplication::socketClosed(socket);
    }
}

} // namespace lasp_ven_simple