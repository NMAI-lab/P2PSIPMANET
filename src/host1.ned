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
import inet.applications.IPingApp;
import inet.linklayer.ieee80211.Ieee80211Nic;
import inet.nodes.inet.AdhocHost;
import inet.networklayer.IManetRouting;
import inet.world.powercontrol.PowerControlManager;


//
// Host that participates in the overlay over a MANET
//
// @author Markus Mauch, Bernhard Heep, Silas Ngozi
//
module host1 extends AdhocHost
{
    parameters:
        string backboneRoutingProtocol @enum("AODVUU","DYMOUM","DYMO","DSRUU","OLSR","OLSR_ETX","DSDV_2","Batman", " ") = default("inet.networklayer.manetrouting.OLSR");
        string mgmtType @enum("Ieee80211MgmtSTASimplified", "Ieee80211MgmtSTA", "Ieee80211MgmtAP", "Ieee80211MgmtAPSimplified", "Ieee80211MgmtAdhoc") = default("Ieee80211MgmtAdhoc");  // use adhoc management
//        int numPingTestApps = default(0);
        @node;
        @display("bgb=660,521;i=device/cellphone");

    submodules:
        backbonemanetrouting: <backboneRoutingProtocol> like IManetRouting {
            @display("p=547,233");
        }

//        pingTestApp[numPingTestApps]: <default("PingApp")> like IPingApp {
//            @display("p=605,289");
//        }
    connections allowunconnected:

        backbonemanetrouting.to_ip --> networkLayer.transportIn++;
        backbonemanetrouting.from_ip <-- networkLayer.transportOut++;

//        for i=0..numPingTestApps-1 {
//            networkLayer.pingOut++ --> pingTestApp[i].pingIn;
//            networkLayer.pingIn++ <-- pingTestApp[i].pingOut;
//        }


}

