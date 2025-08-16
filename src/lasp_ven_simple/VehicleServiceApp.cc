#include "VehicleServiceApp.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/contract/IRoutingTable.h"
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
    
    // Setup service socket (but don't bind yet - wait for IP assignment)
    serviceSocket.setOutputGate(gate("socketOut"));
    serviceSocket.setCallback(this);
    
    EV_WARN << "Service socket setup complete (binding delayed until first request)" << endl;
    
    // Assign IP address to vehicle programmatically
    int vehicleId = getParentModule()->getIndex();
    vehicleIP = "192.168.1." + std::to_string(10 + vehicleId);
    
    EV_WARN << "=== VEHICLE " << vehicleId << " IP ASSIGNMENT ATTEMPT ===" << endl;
    EV_WARN << "Attempting to assign IP: " << vehicleIP << endl;
    
    // Get the interface table and find the wlan interface
    auto interfaceTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    if (interfaceTable) {
        EV_WARN << "Vehicle " << vehicleId << " found interface table with " << interfaceTable->getNumInterfaces() << " interfaces" << endl;
        
        bool ipAssigned = false;
        for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
            auto interface = interfaceTable->getInterface(i);
            if (interface) {
                EV_WARN << "Vehicle " << vehicleId << " checking interface " << i << ": " << interface->getInterfaceName() << endl;
                
                if (strstr(interface->getInterfaceName(), "wlan") != nullptr) {
                    EV_WARN << "Vehicle " << vehicleId << " found wlan interface: " << interface->getInterfaceName() << endl;
                    
                                           auto ipv4Data = interface->getProtocolData<Ipv4InterfaceData>();
                       if (ipv4Data) {
                           EV_WARN << "Vehicle " << vehicleId << " current IP before assignment: " << ipv4Data->getIPAddress().str() << endl;
                           
                           L3Address ipAddr(vehicleIP.c_str());
                           L3Address netmaskAddr("255.255.255.0");
                           ipv4Data->setIPAddress(ipAddr.toIpv4());
                           ipv4Data->setNetmask(netmaskAddr.toIpv4());
                           
                           EV_WARN << "✓ Vehicle " << vehicleId << " IP successfully assigned: " << vehicleIP << " to interface " << interface->getInterfaceName() << endl;
                           EV_WARN << "Vehicle " << vehicleId << " IP after assignment: " << ipv4Data->getIPAddress().str() << endl;
                           EV_WARN << "Vehicle " << vehicleId << " Netmask after assignment: " << ipv4Data->getNetmask().str() << endl;
                           ipAssigned = true;
                           break;
                    } else {
                        EV_WARN << "✗ Vehicle " << vehicleId << " failed to get IPv4 interface data for " << interface->getInterfaceName() << endl;
                    }
                }
            }
        }
        
        if (!ipAssigned) {
            EV_WARN << "✗ Vehicle " << vehicleId << " no suitable wlan interface found for IP assignment" << endl;
        }
    } else {
        EV_WARN << "✗ Vehicle " << vehicleId << " failed to get interface table" << endl;
    }
    
    EV_WARN << "=== VEHICLE " << vehicleId << " IP ASSIGNMENT COMPLETE ===" << endl;
    
    // Add a small delay to ensure IP assignment propagates through network stack
    EV_WARN << "Vehicle " << vehicleId << " waiting 0.1s for IP assignment to propagate..." << endl;
    scheduleAt(simTime() + 0.1, new cMessage("ipPropagationDelay"));
    
    // Resolve LASP Manager address
    laspManagerAddress = L3AddressResolver().resolve("192.168.1.100");
    
    EV_WARN << "LASP Manager address resolved: " << laspManagerAddress.str() << endl;
    
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
    int vehicleId = getParentModule()->getIndex();
    EV_WARN << "[FLOW-1] VEHICLE " << vehicleId << " → LASPManager: Sending service request #" << (requestCounter + 1) << endl;
    
    if (requestCounter >= maxRequests) {
        EV_WARN << "[FLOW-1] VEHICLE " << vehicleId << " → LASPManager: Max requests reached, not sending" << endl;
        return;
    }
    
    // Bind socket on first request (after IP assignment is complete)
    if (requestCounter == 0) {
        EV_WARN << "[FLOW-1] VEHICLE " << vehicleId << " → LASPManager: About to bind socket to port 5000" << endl;
        
        // Check current interface state before binding
        auto interfaceTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        if (interfaceTable) {
            for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
                auto interface = interfaceTable->getInterface(i);
                if (interface && strstr(interface->getInterfaceName(), "wlan") != nullptr) {
                    auto ipv4Data = interface->getProtocolData<Ipv4InterfaceData>();
                    if (ipv4Data) {
                        EV_WARN << "[FLOW-1] VEHICLE " << vehicleId << " → LASPManager: Interface " << interface->getInterfaceName() 
                                << " has IP: " << ipv4Data->getIPAddress().str() << endl;
                    }
                }
            }
        }
        
        // Bind socket to the assigned IP address and port
        EV_WARN << "[FLOW-1] VEHICLE " << vehicleId << " -> LASPManager: Binding socket to " << vehicleIP << ":5000" << endl;
        serviceSocket.bind(L3Address(vehicleIP.c_str()), 5000);
        EV_WARN << "[FLOW-1] VEHICLE " << vehicleId << " -> LASPManager: Socket bound to " << vehicleIP << ":5000" << endl;
        
        // Debug: IP assignment successful, routing will be handled automatically
        EV_WARN << "[FLOW-1] VEHICLE " << vehicleId << " -> LASPManager: IP assignment complete, ready for communication" << endl;
    }
    
    // Create service request packet
    auto packet = new Packet("VehicleServiceRequest");
    auto payload = makeShared<ApplicationPacket>();
    payload->setChunkLength(B(requestSize));
    payload->setSequenceNumber(vehicleId); // Use vehicle index as ID
    packet->insertAtBack(payload);
    
    // Add request metadata
    packet->addTag<CreationTimeTag>()->setCreationTime(simTime());
    
    // Track request for latency measurement  
    pendingRequests[vehicleId] = simTime();
    
    EV_WARN << "[FLOW-1] VEHICLE " << vehicleId << " -> LASPManager: Packet created, sending to " << laspManagerAddress.str() << ":" << laspManagerPort << endl;
    
    // Send to LASP Manager
    serviceSocket.sendTo(packet, laspManagerAddress, laspManagerPort);
    
    emit(serviceRequestsSent, 1);
    requestCounter++;
    
    ServiceType service = selectServiceBasedOnContext();
    EV_WARN << "[FLOW-1] VEHICLE " << vehicleId << " -> LASPManager: Request #" << requestCounter << " sent (service type " << service << ")" << endl;
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
        int vehicleId = getParentModule()->getIndex();
        EV_WARN << "[FLOW-6] VEHICLE " << vehicleId << " <- EDGESERVER: Received response packet: " << packet->getName() << endl;
        
        try {
            auto payload = packet->peekData<ApplicationPacket>();
            int sequenceNumber = payload->getSequenceNumber();
            
            // Calculate latency if we have the original request time
            if (pendingRequests.find(sequenceNumber) != pendingRequests.end()) {
                simtime_t latency = simTime() - pendingRequests[sequenceNumber];
                emit(serviceLatency, latency.dbl());
                pendingRequests.erase(sequenceNumber);
                
                EV_WARN << "[FLOW-6] VEHICLE " << vehicleId << " <- EDGESERVER: Response received with latency " << (latency.dbl() * 1000) << "ms" << endl;
                        
                emit(serviceResponsesReceived, 1);
            } else {
                EV_WARN << "[FLOW-6] VEHICLE " << vehicleId << " <- EDGESERVER: No pending request found for sequence " << sequenceNumber << endl;
            }
        } catch (const std::exception& e) {
            EV_WARN << "[FLOW-6] VEHICLE " << vehicleId << " <- EDGESERVER: Failed to parse response: " << e.what() << endl;
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

void VehicleServiceApp::handleMessage(cMessage* msg)
{
    if (strcmp(msg->getName(), "ipPropagationDelay") == 0) {
        int vehicleId = getParentModule()->getIndex();
        EV_WARN << "Vehicle " << vehicleId << " IP propagation delay completed, scheduling first service request" << endl;
        
        // Verify IP is still assigned after delay
        auto interfaceTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        if (interfaceTable) {
            for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
                auto interface = interfaceTable->getInterface(i);
                if (interface && strstr(interface->getInterfaceName(), "wlan") != nullptr) {
                    auto ipv4Data = interface->getProtocolData<Ipv4InterfaceData>();
                    if (ipv4Data) {
                        EV_WARN << "Vehicle " << vehicleId << " IP verification after delay: " << ipv4Data->getIPAddress().str() << endl;
                    }
                }
            }
        }
        
        delete msg;
        
        // Now schedule the first service request after IP assignment has propagated
        scheduleNextServiceRequest();
    } else {
        // Handle VeinsInetSampleApplication messages (accident messages, etc.)
        VeinsInetSampleApplication::handleMessage(msg);
    }
}

} // namespace lasp_ven_simple