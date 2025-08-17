package lasp_ven.simulations.lasp_ven_simple_example;

import lasp_ven.veins_inet.VeinsInetManager;
import lasp_ven.veins_inet.VeinsInetCar;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.physicallayer.unitdisk.UnitDiskRadioMedium;
import inet.node.inet.StandardHost;
import lasp_ven.lasp_ven_simple.LASPManager;
import lasp_ven.lasp_ven_simple.EdgeServerApp;
import lasp_ven.lasp_ven_simple.VehicleServiceApp;
import inet.transportlayer.udp.Udp;
import inet.visualizer.integrated.IntegratedVisualizer;
import inet.environment.common.PhysicalEnvironment;
import org.car2x.veins.visualizer.roads.RoadsCanvasVisualizer;
import org.car2x.veins.visualizer.roads.RoadsOsgVisualizer;

network LaspVenSimpleSimulation
{
    parameters:
        double playgroundSizeX @unit(m) = default(200m);
        double playgroundSizeY @unit(m) = default(200m);
        double playgroundSizeZ @unit(m) = default(50m);
        bool useOsg = default(false);
        int numEdgeServers = default(3);

        @display("bgb=1000,1000");

    submodules:
        radioMedium: UnitDiskRadioMedium {
            @display("p=168.672,96.383995");
        }

        configurator: Ipv4NetworkConfigurator {
            @display("p=716.85596,692.76");
        }

        veinsManager: VeinsInetManager {
            @display("p=375.496,692.76");
        }

        visualizer: IntegratedVisualizer {
            @display("p=84.336,867.456");
        }

        roadsCanvasVisualizer: RoadsCanvasVisualizer {
            @display("p=375.496,867.456");
        }

        roadsOsgVisualizer: RoadsOsgVisualizer if useOsg {
            @display("p=716.85596,867.456");
        }

        // LASP Manager (Regional Edge Computing Orchestrator)
        laspManager: StandardHost {
            @display("p=100,100;i=device/server");

            // Application configuration
            numApps = 1;
            app[0].typename = "lasp_ven.lasp_ven_simple.LASPManager";
            
            // Network stack configuration
            numWlanInterfaces = 1;
            
            // Ensure proper network initialization
            hasIpv4 = true;
            hasUdp = true;
        }

        // Physical Edge Servers
        edgeServer[numEdgeServers]: StandardHost {
            @display("p=162.648,339.352;i=device/server2");  // Add position
            numApps = 1;
            app[0].typename = "lasp_ven.lasp_ven_simple.EdgeServerApp";
        }

    connections allowunconnected:
}
