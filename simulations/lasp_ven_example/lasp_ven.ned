//
// LASP VEN Simulation Network
//
package lasp_ven.simulations.lasp_ven_example;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.inet.StandardHost;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import lasp_ven.veins_inet.VeinsInetManager;

network LaspVenSimulation
{
    parameters:
        double playgroundSizeX @unit(m) = default(2500m);
        double playgroundSizeY @unit(m) = default(2500m);
        double playgroundSizeZ @unit(m) = default(50m);

        @display("bgb=1000,1000");

    submodules:
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=168.672,96.383995");
        }

        configurator: Ipv4NetworkConfigurator {
            @display("p=568.264,590.352");
        }

        veinsManager: VeinsInetManager {
            @display("p=162.648,614.448");
        }

        // LASP Manager (Edge Computing Infrastructure)
        laspManager: StandardHost {
            @display("p=500,100;i=device/server");

            applications[0].typename = "LASPManager";
        }

        // Road Side Units (optional, for extended scenarios)
        rsu[0]: StandardHost {
            @display("p=329.31198,347.384;i=device/antennatower");
        }

    connections allowunconnected:
}

//
// Vehicle node definition for LASP VEN
//
import inet.applications.contract.IApp;
import inet.linklayer.ieee80211.Ieee80211Interface;
import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.Ipv4NetworkLayer;
import inet.transportlayer.udp.Udp;
import lasp_ven.veins_inet.VeinsInetMobility;

module VehicleNode
{
    parameters:
        @networkNode();
        @labels(node,ethernet-node,wireless-node);
        @display("i=device/car");

        // Applications
        int numApps = default(1);
        
        *.interfaceTableModule = "^.interfaceTable";

    gates:
        input radioIn @directIn;
        
    submodules:
        interfaceTable: InterfaceTable {
            @display("p=50,300");
        }

        mobility: VeinsInetMobility {
            @display("p=50,200");
        }

        app[numApps]: <> like IApp {
            @display("p=200,100,row,60");
        }

        udp: Udp {
            @display("p=200,200");
            interfaceTableModule = "^.interfaceTable";
        }

        networkLayer: Ipv4NetworkLayer {
            @display("p=200,300");
            interfaceTableModule = "^.interfaceTable";
        }

        wlan: Ieee80211Interface {
            @display("p=200,400");
            radio.antenna.mobilityModule = "^.^.^.mobility";
        }

    connections:
        // Application to transport layer
        for i=0..numApps-1 {
            app[i].socketOut --> udp.appIn;
            app[i].socketIn <-- udp.appOut;
        }

        // Transport to network layer
        udp.ipOut --> networkLayer.transportIn;
        udp.ipIn <-- networkLayer.transportOut;

        // Network to link layer
        networkLayer.ifOut --> wlan.upperLayerIn;
        networkLayer.ifIn <-- wlan.upperLayerOut;

        // Physical connection
        wlan.radioIn <-- radioIn;
}
