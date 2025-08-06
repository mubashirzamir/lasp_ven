package lasp_ven.simulations.lasp_ven_simple_example;

import lasp_ven.veins_inet.VeinsInetManager;
import lasp_ven.veins_inet.VeinsInetCar;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.node.inet.StandardHost;
import lasp_ven.lasp_ven_simple.LASPManager;
import lasp_ven.lasp_ven_simple.TestApp;
import inet.visualizer.integrated.IntegratedVisualizer;
import inet.environment.common.PhysicalEnvironment;
import org.car2x.veins.visualizer.roads.RoadsCanvasVisualizer;
import org.car2x.veins.visualizer.roads.RoadsOsgVisualizer;

network LaspVenSimpleSimulation
{
    parameters:
        double playgroundSizeX @unit(m) = default(2500m);
        double playgroundSizeY @unit(m) = default(2500m);
        double playgroundSizeZ @unit(m) = default(50m);
        bool useOsg = default(false);

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

        visualizer: IntegratedVisualizer {
            @display("p=64,320");
        }

        roadsCanvasVisualizer: RoadsCanvasVisualizer {
            @display("p=64,416");
        }

        roadsOsgVisualizer: RoadsOsgVisualizer if useOsg {
            @display("p=192,416");
        }

        // LASP Manager (Edge Computing Infrastructure)
        laspManager: StandardHost {            
            @display("p=500,100;i=device/server");
            
            // Application configuration
            numApps = 1;
            app[0].typename = "lasp_ven.lasp_ven_simple.LASPManager";
        }

        // Test Application (for testing LASP Manager)
        testApp: StandardHost {
            @display("p=500,200;i=device/laptop");
            
            // Application configuration
            numApps = 1;
            app[0].typename = "lasp_ven.lasp_ven_simple.TestApp";
        }

        // Road Side Units (optional, for extended scenarios)
        rsu[0]: StandardHost {
            @display("p=329.31198,347.384;i=device/antennatower");
        }

    connections allowunconnected:
} 