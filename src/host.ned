//
//  Copyright (C) 2006,2007 Christoph Sommer <christoph.sommer@informatik.uni-erlangen.de>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
module host extends NodeBase
{
    parameters:
        string overlayType; // overlay protocol compound module to use
        string tier1Type; // tier 1 application to use
        string tier2Type; // tier 2 module to use
        string tier3Type; // tier 3 module to use
        string routingProtocol @enum("AODVUU","DYMOUM","DYMO","DSRUU","OLSR","OLSR_ETX","DSDV_2","Batman", " ") = default("inet.networklayer.manetrouting.OLSR");
        string backboneRoutingProtocol @enum("AODVUU","DYMOUM","DYMO","DSRUU","OLSR","OLSR_ETX","DSDV_2","Batman", " ") = default("inet.networklayer.manetrouting.OLSR");
        string mgmtType @enum("Ieee80211MgmtSTASimplified", "Ieee80211MgmtSTA", "Ieee80211MgmtAP", "Ieee80211MgmtAPSimplified", "Ieee80211MgmtAdhoc") = default("Ieee80211MgmtAdhoc");  // use adhoc management
        int numTiers; // number of tiers
        int numPingApps = default(0);  // no of PING apps. Specify the app types in INI file with pingApp[0..1].typename="PingApp" syntax
       	@node;
        @display("bgb=613,521;i=device/cellphone");

    gates:
        input overlayNeighborArrowIn[]; // incoming gate for visualizing overlay neighborship with connection arrows
        output overlayNeighborArrowOut[]; // incoming gate for visualizing overlay neighborship with connection arrows

    submodules:
        manetrouting: <routingProtocol> like IManetRouting {
            @display("p=549,207");
        }
        backbonemanetrouting: <backboneRoutingProtocol> like IManetRouting {
            @display("p=558,338");
        }
        tier3: <tier3Type> like ITier {
            parameters:
                @display("p=96,44");
        }
        tier2: <tier2Type> like ITier {
            parameters:
                @display("p=197,65");
        }
        tier1: <tier1Type> like ITier {
            parameters:
                @display("p=289,104");
        }
        overlay: <overlayType> like IOverlay {
            parameters:
                @display("p=374,150");
        }
        udp: UDP {
            parameters:
                @display("p=374,246");
        }
        tcp: TCP {
            parameters:
                @display("p=269,246");
        }
        neighborCache: NeighborCache {
            parameters:
                @display("p=135,227");
        }
        bootstrapList: BootstrapList {
            parameters:
                @display("p=143,311");
        }
        cryptoModule: CryptoModule {
            parameters:
                @display("p=143,382");
        }
        pingApp[numPingApps]: <default("PingApp")> like IPingApp {
            @display("p=538,272");
        }
        pingTestApp[numPingApps]:<default("PingTestApp")> like IPingApp {
            @display("p=503,150");
        }
    connections allowunconnected:
//        networkLayer.ifOut++ --> lo0.upperLayerIn;
//        lo0.upperLayerOut --> networkLayer.ifIn++;

        tier1.to_lowerTier --> overlay.appIn if numTiers>0;
        tier1.from_lowerTier <-- overlay.appOut if numTiers>0;
        tier1.udpOut --> udp.appIn++ if numTiers>0;
        udp.appOut++ --> tier1.udpIn if numTiers>0;
        tier1.tcpOut --> tcp.appIn++ if numTiers>0;
        tcp.appOut++ --> tier1.tcpIn if numTiers>0;

        tier2.to_lowerTier --> tier1.from_upperTier if numTiers > 1;
        tier2.from_lowerTier <-- tier1.to_upperTier if numTiers > 1;
        tier2.udpOut --> udp.appIn++ if numTiers>1;
        udp.appOut++ --> tier2.udpIn if numTiers>1;
        tier2.tcpOut --> tcp.appIn++ if numTiers>1;
        tcp.appOut++ --> tier2.tcpIn if numTiers>1;

        tier3.to_lowerTier --> tier2.from_upperTier if numTiers > 2;
        tier3.from_lowerTier <-- tier2.to_upperTier if numTiers > 2;
        tier3.udpOut --> udp.appIn++ if numTiers>2;
        udp.appOut++ --> tier3.udpIn if numTiers>2;
        tier3.tcpOut --> tcp.appIn++ if numTiers>2;
        tcp.appOut++ --> tier3.tcpIn if numTiers>2;

        overlay.udpOut --> udp.appIn++;
        overlay.udpIn <-- udp.appOut++;

        overlay.tcpOut --> tcp.appIn++;
        overlay.tcpIn <-- tcp.appOut++;

        bootstrapList.udpOut --> udp.appIn++;
        bootstrapList.udpIn <-- udp.appOut++;

        udp.ipOut --> networkLayer.transportIn++;
        udp.ipIn <-- networkLayer.transportOut++;

        tcp.ipOut --> networkLayer.transportIn++;
        tcp.ipIn <-- networkLayer.transportOut++;

        manetrouting.to_ip --> networkLayer.transportIn++;
        manetrouting.from_ip <-- networkLayer.transportOut++;
        backbonemanetrouting.to_ip --> networkLayer.transportIn++;
        backbonemanetrouting.from_ip <-- networkLayer.transportOut++;

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

