package p2psipmanet.simulations.RyanStatic;

import inet.networklayer.IManetRouting;
import inet.nodes.inet.NodeBase;
import inet.transport.udp.UDP;
import inet.transport.tcp.TCP;
import inet.applications.IPingApp;

module StaticGateway extends NodeBase
{
    parameters:
        @node;
        @display("i=device/cellphone;bgb=665,477");
        wlan[*].mgmtType = default("Ieee80211MgmtAdhoc");  // use adhoc management
        IPForward = default(true);
        int numPingApps = default(0);  // no of PING apps. Specify the app types in INI file with pingApp[0..1].typename="PingApp" syntax
    submodules:
    	udp: UDP {
            parameters:
                @display("p=374,246");
        }
        tcp: TCP {
            parameters:
                @display("p=269,246");
        }

        pingApp[numPingApps]: <default("PingApp")> like IPingApp {
            @display("p=538,272");
        }
        
        pingTestApp[numPingApps]:<default("PingTestApp")> like IPingApp {
            @display("p=503,150");
        }
        
    connections allowunconnected:
        udp.ipOut --> networkLayer.transportIn++;
        udp.ipIn <-- networkLayer.transportOut++;

        tcp.ipOut --> networkLayer.transportIn++;
        tcp.ipIn <-- networkLayer.transportOut++;
        
        // connections to network outside
        for i=0..sizeof(pppg)-1 {
            pppg[i] <--> ppp[i].phys;
            ppp[i].upperLayerOut --> networkLayer.ifIn++;
            ppp[i].upperLayerIn <-- networkLayer.ifOut++;
        }
        
        for i=0..numPingApps-1 {
            networkLayer.pingOut++ --> pingApp[i].pingIn;
            networkLayer.pingIn++ <-- pingApp[i].pingOut;
        }        
        for i=0..numPingApps-1 {
            networkLayer.pingOut++ --> pingTestApp[i].pingIn;
            networkLayer.pingIn++ <-- pingTestApp[i].pingOut;
        }
}
