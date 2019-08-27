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

package p2psipmanet.simulations.RyanStatic;

import p2psipmanet.simulations.RyanStatic.StaticGateway;
import inet.nodes.inet.AdhocHost;
import inet.networklayer.autorouting.ipv4.IPv4NetworkConfigurator;
import inet.world.radio.ChannelControl;
import inet.util.ThruputMeteringChannel;
import oversim.common.GlobalObserver;
import p2psipmanet.DummyUnderlayConfigurator;
import p2psipmanet.ZeroChurn;

network StaticHierarchical {
    parameters:

        @display("bgb=801,595");
        double hosts;
        double gw;

    gates:
        input radioIn @directIn;


    types:
        channel C extends ThruputMeteringChannel
        {
            delay = 0.1us;
            datarate = 100Mbps;
            thruputDisplayFormat = "#N";
        }
    submodules:
        channelControl: ChannelControl {
            @display("p=42,37");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=47,98");
        }
        globalObserver: GlobalObserver {
            @display("p=47,156");
        }
        underlayConfigurator: DummyUnderlayConfigurator {
            @display("p=47,228");
        }
        churnGenerator: ZeroChurn {
            @display("p=47,301");
        }
        gw[gw]: StaticGateway;
        host[hosts]: AdhocHost;
    connections allowunconnected:
} 
