package lasp_ven.lasp_ven_simple;

import inet.applications.contract.IApp;

//
// Edge Server Application for handling service requests
// This provides a physical representation of edge servers in the network
//
simple EdgeServerApp like IApp
{
    parameters:
        @class(lasp_ven_simple::EdgeServerApp);    
        int serverId = default(0);
        double computeCapacity @unit(GFLOPS) = default(100GFLOPS);
        double storageCapacity @unit(GB) = default(1000GB);
        int localPort = default(8000 + serverId);
        
        // Statistics
        @signal[requestsReceived](type=long);
        @signal[requestsProcessed](type=long);
        @signal[serverLoad](type=double);
        @statistic[requestsReceived](title="Requests Received"; record=count,sum,vector);
        @statistic[requestsProcessed](title="Requests Processed"; record=count,sum,vector);
        @statistic[serverLoad](title="Server Load"; record=mean,max,vector);
        
        @display("i=device/server2");
        
    gates:
        input socketIn;
        output socketOut;
}