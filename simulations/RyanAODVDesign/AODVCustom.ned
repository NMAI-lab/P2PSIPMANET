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

package p2psipmanet.simulations.RyanAODVDesign;

import inet.base.LifecycleController;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.world.scenario.ScenarioManager;
import inet.networklayer.autorouting.ipv4.IPv4NetworkConfigurator;
import inet.nodes.inet.AdhocHost;
import inet.world.radio.ChannelControl;
import inet.util.ThruputMeteringChannel;
import p2psipmanet.simulations.RyanAODVDesign.gateway;

network AODVNetworkCustom
{
    parameters:
        // Parameters to set to configure the number of
        // Hosts and gateways. Should be set via ini file
        int numHosts;
        int numGateway;
        
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
        // Allows for node initialization
        channelControl: ChannelControl {
            parameters:
                @display("p=50,50");
        }
        
        // Routes the nodes
        configurator: IPv4NetworkConfigurator {
            @display("p=47,98");
        }
        
        routingTableRecorder: RoutingTableRecorder {
            parameters:
                @display("p=50,150");
        }
        
        lifecycleController: LifecycleController {
            parameters:
                @display("p=50,200");
        }
        
        scenarioManager: ScenarioManager {
            parameters:
                script = default(xml("<scenario/>"));
                @display("p=50,250");
        }
        
        // Specify the hosts
        host[numHosts]: AdhocHost {            
            parameters:
                @display("i=device/pocketpc_s;r=,,#707070");
        }; 
        
        // Specify the gateways
        gw[numGateway]: gateway;
        
    connections allowunconnected:
}
