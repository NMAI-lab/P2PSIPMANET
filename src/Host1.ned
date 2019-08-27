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
import inet.networklayer.IManetRouting;
import inet.networklayer.ipv4.RoutingTable;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.manetrouting.OLSR;
import inet.linklayer.ppp.PPPInterface;
import inet.linklayer.IExternalNic;
import inet.linklayer.IWirelessNic;
import inet.linklayer.loopback.LoopbackInterface;
import inet.base.NotificationBoard;
import inet.nodes.inet.NetworkLayer;
import inet.transport.udp.UDP;
import inet.transport.tcp.TCP;
import inet.mobility.contract.IMobility;
import inet.battery.IBattery;
import inet.status.NodeStatus;
import inet.util.PcapRecorder;
import inet.applications.IPingApp;
import inet.world.powercontrol.PowerControlManager;
import oversim.common.BootstrapList;
import oversim.common.CryptoModule;
import oversim.common.IOverlay;
import oversim.common.ITier;
import oversim.common.NeighborCache;

//
// Host that participates in the overlay over a MANET
//
// @author Markus Mauch, Bernhard Heep, Silas Ngozi
//
module Host1
{
    parameters:
        string overlayType; // overlay protocol compound module to use
        string tier1Type; // tier 1 application to use
        string tier2Type; // tier 2 module to use
        string tier3Type; // tier 3 module to use
        string routingFile = default("");
        string batteryType = default("");
        string routingProtocol @enum("AODVUU","DYMOUM","DYMO","DSRUU","OLSR","OLSR_ETX","DSDV_2","Batman", " ") = default("inet.networklayer.manetrouting.OLSR");
        string mobilityType = default(numRadios > 0 ? "StationaryMobility" : "");
        int numTiers; // number of tiers
        int numExtInterfaces = default(0);
        int numPcapRecorders = default(0); // no of PcapRecorders.
        int numPingApps = default(0);  // no of PING apps. Specify the app types in INI file with pingApp[0..1].typename="PingApp" syntax
		int numRadios = default(1);
        bool hasStatus = default(true);
        bool IPForward = default(true);
        bool forwardMulticast = default(false);
        wlan[*].mgmtType = default("Ieee80211MgmtAdhoc");  // use adhoc management
        //string routingProtocol = default("inet.networklayer.manetrouting.OLSR");
        @node;
        @display("bgb=665,521;i=device/cellphone");
        //networkLayer.configurator.networkConfiguratorModule = ""; // parameter for IPv4NetworkLayer (the default value of networkLayer)
    gates:
        input radioIn[numRadios] @directIn;
        inout pppg[] @labels(PPPFrame-conn); // gates from router
        input overlayNeighborArrowIn[]; // incoming gate for visualizing overlay neighborship with connection arrows
        output overlayNeighborArrowOut[]; // incoming gate for visualizing overlay neighborship with connection arrows

    submodules:
        manetrouting: <routingProtocol> like IManetRouting {
            @display("p=519,305");
        }
        notificationBoard: NotificationBoard {
            @display("p=46,246");
        }
        interfaceTable: InterfaceTable {
            parameters:
                @display("p=46,311");
        }
        // optional mobility module. Required only if wireless cards are present
        mobility: <mobilityType> like IMobility if mobilityType != "" {
            parameters:
                @display("p=46,187");
        }

        routingTable: RoutingTable {
            parameters:
                IPForward = IPForward; // true if ip packets should be forwarded
                routerId = ""; // id of the router
                routingFile = routingFile; // use predefined routing file if given
                @display("p=46,382");
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
        networkLayer: NetworkLayer {
            parameters:
                proxyARP = false; // sets proxy \ARP mode (replying to \ARP requests for the addresses for which a routing table entry exists)
                @display("p=374,332;q=queue");
            gates:
                ifIn[sizeof(pppg)+sizeof(ethg)];
                ifOut[sizeof(pppg)+sizeof(ethg)];
        }
        lo0: LoopbackInterface {
            @display("p=289,436");
        }
        ppp[sizeof(pppg)]: PPPInterface {
            parameters:
                @display("p=496,436,row,90;q=txQueue");
        }
        wlan[numRadios]: <default("Ieee80211Nic")> like IWirelessNic {
            parameters:
                @display("p=381,436,row,60;q=queue");
        }
        ext[numExtInterfaces]: <default("ExtInterface")> like IExternalNic {
            parameters:
                @display("p=197,444,row,10;q=txQueue;i=block/ifcard");
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

        battery: <batteryType> like IBattery if batteryType != "" {
            parameters:
                @display("p=46,135;i=block/plug;is=s");
        }

        pcapRecorder[numPcapRecorders]: PcapRecorder {
            @display("p=46,436,r,10");
        }

        status: NodeStatus if hasStatus {
            @display("p=46,83");
        }

        pingApp[numPingApps]: <default("PingApp")> like IPingApp {
            @display("p=572,218");
        }

        pingTestApp[numPingApps]: <default("PingApp")> like IPingApp {
            @display("p=572,135");
        }
    connections allowunconnected:
        networkLayer.ifOut++ --> lo0.upperLayerIn;
        lo0.upperLayerOut --> networkLayer.ifIn++;

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

        for i=0..sizeof(radioIn)-1 {
            radioIn[i] --> { @display("m=s"); } --> wlan[i].radioIn;
            wlan[i].upperLayerOut --> networkLayer.ifIn++;
            wlan[i].upperLayerIn <-- networkLayer.ifOut++;
        }
        // connections to network outside
        for i=0..sizeof(pppg)-1 {
            pppg[i] <--> ppp[i].phys;
            ppp[i].upperLayerOut --> networkLayer.ifIn++;
            ppp[i].upperLayerIn <-- networkLayer.ifOut++;
        }

        for i=0..numExtInterfaces-1 {
            ext[i].upperLayerOut --> networkLayer.ifIn++;
            ext[i].upperLayerIn <-- networkLayer.ifOut++;
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

