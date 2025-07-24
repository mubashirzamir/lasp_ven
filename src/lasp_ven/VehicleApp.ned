//
// VehicleApp - Application running on vehicles for service requests
//
package lasp_ven.lasp_ven;

import lasp_ven.veins_inet.VeinsInetApplicationBase;

simple VehicleApp extends VeinsInetApplicationBase
{
    parameters:
        @class(lasp_ven::VehicleApp);
        @display("i=block/app");
        
        // Vehicle identification
        int vehicleId = default(ancestorIndex(0)); // Use node index as default
        
        // Communication parameters
        int laspManagerPort = default(9999);
        double requestInterval @unit(s) = default(5s);
        
        // Service preferences (can be extended)
        string preferredServices = default("TRAFFIC_INFO,NAVIGATION");
        
        // Statistics
        @signal[requestsSent](type=long);
        @signal[responsesReceived](type=long);
        @signal[serviceLatency](type=double);
        
        @statistic[requestsSent](title="Service Requests Sent"; record=count,sum,vector; interpolationmode=none);
        @statistic[responsesReceived](title="Service Responses Received"; record=count,sum,vector; interpolationmode=none);
        @statistic[serviceLatency](title="Service Response Latency"; record=mean,max,min,vector; interpolationmode=none);
}