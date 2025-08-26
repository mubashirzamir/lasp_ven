package lasp_ven.lasp_ven_simple;

import inet.applications.contract.IApp;

simple LASPManager like IApp
{
    parameters:
        @class(lasp_ven_simple::LASPManager);
        @display("i=block/server");
        
        // Strategy parameters
        string strategy @enum("threshold", "greedy", "greedyLatencyAware", "thresholdLatencyAware") = default("threshold");
        double loadThreshold = default(0.8);
        double loadWeight = default(0.5);
        double latencyWeight = default(0.5);
        double evaluationInterval @unit(s) = default(2s);
        int numEdgeServers = default(4);
        int localPort = default(9999);
        
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