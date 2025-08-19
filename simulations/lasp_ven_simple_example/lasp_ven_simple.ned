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
        double playgroundSizeX @unit(m) = default(100m);
        double playgroundSizeY @unit(m) = default(100m);
        double playgroundSizeZ @unit(m) = default(50m);
        bool useOsg = default(true);
        int numEdgeServers = default(4);

        @display("bgb=1000,1000");

    submodules:
        radioMedium: UnitDiskRadioMedium {
            @display("p=889.54395,70.28");
        }

        configurator: Ipv4NetworkConfigurator {
            @display("p=889.54395,248.99199");
        }

        veinsManager: VeinsInetManager {
            @display("p=662.63995,72.287994");
        }

        visualizer: IntegratedVisualizer {
            @display("p=662.63995,251");
        }

        roadsCanvasVisualizer: RoadsCanvasVisualizer {
            @display("p=895.568,676.696");
        }

        roadsOsgVisualizer: RoadsOsgVisualizer if useOsg {
            @display("p=889.54395,451.8");
        }

        // LASP Manager (Regional Edge Computing Orchestrator) - Center of square road
        laspManager: StandardHost {
            @display("p=250,250;i=device/server");

            // Application configuration
            numApps = 1;
            app[0].typename = "lasp_ven.lasp_ven_simple.LASPManager";

            // Network stack configuration
            numWlanInterfaces = 1;

            // Ensure proper network initialization
            hasIpv4 = true;
            hasUdp = true;
        }

        // Physical Edge Servers - Fixed to exactly 4 servers at square corners
        edgeServer[4]: StandardHost {
            numApps = 1;
            app[0].typename = "lasp_ven.lasp_ven_simple.EdgeServerApp";

            // Network stack configuration
            numWlanInterfaces = 1;
            hasIpv4 = true;
            hasUdp = true;
            @display("p=248.99199,437.744");
        }

    connections allowunconnected:
}
