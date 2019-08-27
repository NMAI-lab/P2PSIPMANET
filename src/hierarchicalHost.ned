//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package p2psipmanet;

import inet.applications.pingapp.PingTestApp;
import inet.linklayer.ieee80211.Ieee80211Nic;
import inet.nodes.inet.NetworkLayer;
import inet.nodes.inet.NodeBase;
import inet.networklayer.IManetRouting;
import inet.networklayer.manetrouting.OLSR;
import inet.networklayer.manetrouting.OLSR_ETX;
import inet.transport.udp.UDP;
import inet.transport.tcp.TCP;
import inet.applications.IPingApp;
import inet.world.powercontrol.PowerControlManager;
import oversim.common.BootstrapList;
import oversim.common.CryptoModule;
import oversim.common.IOverlay;
import oversim.common.ITier;
import oversim.common.NeighborCache;
import oversim.underlay.inetunderlay.InetOverlayHost;
import oversim.underlay.inetunderlay.InetOverlayHostAdHoc;


//
// Host that participates in the overlay over a MANET
//
// @author Markus Mauch, Bernhard Heep, Silas Ngozi
//
module hierarchicalHost extends host
{
    parameters:
        @node;
        @display("bgb=613,521;i=device/cellphone");

    gates:

    submodules:

        networkLayer1: NetworkLayer {
            @display("p=447,285");
        }
        
        
    connections allowunconnected:
//        networkLayer.ifOut++ --> lo0.upperLayerIn;
//        lo0.upperLayerOut --> networkLayer.ifIn++;

	for i=0..sizeof(ipOut)-1{
        udp.ipOut --> networkLayer1.transportIn++;
        udp.ipIn <-- networkLayer1.transportOut++;
	}

	for i=0..sizeof(ipOut)-1{
        tcp.ipOut --> networkLayer1.transportIn++;
        tcp.ipIn <-- networkLayer1.transportOut++;
	}
	for i=0..sizeof(to_ip)-1{
        backbonemanetrouting.to_ip --> networkLayer1.transportIn++;
        backbonemanetrouting.from_ip <-- networkLayer1.transportOut++;
	}
	
	//networkLayer.ifOut++ --> networkLayer1.ifIn;
	//networkLayer.ifIn --> networkLayer1.ifOut++;
        // connections to network outside
    for i=0..sizeof(ifOut)-1{
		networkLayer.ifOut++ --> lo0.upperLayerIn;
        lo0.upperLayerOut --> networkLayer.ifIn++;
    }

//        for i=2..sizeof(radioIn)+3 {
//            radioIn[i] --> { @display("m=s"); } --> wlan[i].radioIn;
//            wlan[i].upperLayerOut --> networkLayer1.ifIn++;
//            wlan[i].upperLayerIn <-- networkLayer1.ifOut++;
//        }

//
//        for i=0..numExtInterfaces{
//            ext[i].upperLayerOut --> networkLayer1.ifIn++;
//            ext[i].upperLayerIn <-- networkLayer1.ifOut++;
//        }

//        for i=0..numPingApps-1 {
//            networkLayer1.pingOut++ --> pingApp[i].pingIn;
//            networkLayer1.pingIn++ <-- pingApp[i].pingOut;
//        }
//        for i=0..numPingApps-1 {
//            networkLayer1.pingOut++ --> pingTestApp[i].pingIn;
//            networkLayer1.pingIn++ <-- pingTestApp[i].pingOut;
//        }
}
