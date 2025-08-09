package lasp_ven.lasp_ven_simple;

import lasp_ven.veins_inet.VeinsInetApplicationBase;

simple VehicleServiceApp extends VeinsInetApplicationBase
{
    parameters:
        @class(lasp_ven_simple::VehicleServiceApp);
        @display("i=block/app");
        
        // Service request parameters
        double serviceRequestInterval @unit(s) = default(10s);
        int maxRequests = default(5);
        int laspManagerPort = default(9999);
        
        // Statistics
        @signal[serviceRequestsSent](type=long);
        @signal[serviceResponsesReceived](type=long);
        @signal[serviceLatency](type=double);
        
        @statistic[serviceRequestsSent](title="Service Requests Sent"; record=count,sum,vector);
        @statistic[serviceResponsesReceived](title="Service Responses Received"; record=count,sum,vector);
        @statistic[serviceLatency](title="Service Response Latency"; record=mean,max,min,vector; unit=s);
}