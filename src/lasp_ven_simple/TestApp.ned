package lasp_ven.lasp_ven_simple;

import inet.applications.contract.IApp;

simple TestApp like IApp
{
    parameters:
        @class(lasp_ven_simple::TestApp);
        @display("i=block/app");
        
        // Test identification
        int testId = default(ancestorIndex(0)); // Use node index as default
        
        // Communication parameters
        int laspManagerPort = default(9999);
        double requestInterval @unit(s) = default(5s);
        int maxRequests = default(10);
        
        // Statistics
        @signal[requestsSent](type=long);
        @signal[responsesReceived](type=long);
        @signal[serviceLatency](type=double);
        
        @statistic[requestsSent](title="Test Requests Sent"; record=count,sum,vector; interpolationmode=none);
        @statistic[responsesReceived](title="Test Responses Received"; record=count,sum,vector; interpolationmode=none);
        @statistic[serviceLatency](title="Service Response Latency"; record=mean,max,min,vector; interpolationmode=none);
        
    gates:
        input socketIn @labels(UdpControlInfo/up);
        output socketOut @labels(UdpControlInfo/down);
} 