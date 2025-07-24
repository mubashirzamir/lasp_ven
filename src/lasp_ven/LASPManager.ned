//
// LASPManager - Location-Aware Service Placement Manager for VENs
//
package lasp_ven.lasp_ven;

import inet.applications.contract.IApp;

simple LASPManager like IApp
{
    parameters:
        @class(lasp_ven::LASPManager);
        @display("i=block/control");
        
        // Strategy configuration
        string strategy = default("greedy"); // "greedy", "threshold", "optimal"
        
        // Network parameters
        int localPort = default(9999);
        
        // Service placement parameters
        double evaluationInterval @unit(s) = default(1s);
        double maxServiceLatency @unit(s) = default(0.1s); // 100ms max latency
        int numEdgeServers = default(5);
        
        // Service parameters
        double serviceTimeout @unit(s) = default(10s);
        
        // Statistics
        @signal[requestsReceived](type=long);
        @signal[requestsServed](type=long);
        @signal[averageLatency](type=double);
        @signal[serverUtilization](type=double);
        
        @statistic[requestsReceived](title="Service Requests Received"; record=count,sum,vector; interpolationmode=none);
        @statistic[requestsServed](title="Service Requests Served"; record=count,sum,vector; interpolationmode=none);
        @statistic[averageLatency](title="Average Service Latency"; record=mean,max,min,vector; interpolationmode=none);
        @statistic[serverUtilization](title="Server Utilization"; record=mean,max,min,vector; interpolationmode=none);
        
    gates:
        input socketIn @labels(UdpControlInfo/up);
        output socketOut @labels(UdpControlInfo/down);
}