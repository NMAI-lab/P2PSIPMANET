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

package p2psipmanet.DHT4Rouring;

import oversim.common.BaseApp;
import oversim.common.ITier;


//
// Simple module of the DHT4Routing test application
//
// @author Gregoire Menuel, Ingmar Baumgart, Silas Ngozi
//
simple DHT4Routing extends BaseApp
{
    parameters:
        @class(DHT4Routing);
        double testInterval @unit(s) = default(5s);    // time interval for DHT operations (put/get/modify)
        int testTtl;      // TTL for stored test records
        bool activeNetwInitPhase = default(false);    // send messages when network is in init phase?
        bool p2pnsTraffic = default(false); // model p2pns application traffic
        string nodeHandle = default(""); // allows to specify which interface a node uses to join the overlay
        string overlayInterface = default("0"); // allows to specify which interface a node uses to join the overlay
		double DHT4RoutingtransitionTime @unit(s) = default(100s);
		double DHT4RoutingmeasurementTime @unit(s); //= default(500s);
		double stabilization @unit(s); //
}


//
// Compound module for a simple DHT test application
//
// @author Gregoire Menuel, Ingmar Baumgart, Silas Ngozi
//
module DHT4RoutingModules like ITier
{
    gates:
        input from_lowerTier;    // gate from the lower tier
        input from_upperTier;    // gate from the upper tier
        input udpIn;    // gate from the UDP layer
        input trace_in;    // gate for trace file commands
        input tcpIn;             // gate from the TCP layer
        output tcpOut;           // gate to the TCP layer
        output to_lowerTier;    // gate to the lower tier
        output to_upperTier;    // gate to the upper tier
        output udpOut;    // gate to the UDP layer

    submodules:
        dht4routing: DHT4Routing;
    connections allowunconnected:
        from_lowerTier --> dht4routing.from_lowerTier;
        to_lowerTier <-- dht4routing.to_lowerTier;
        udpIn --> dht4routing.udpIn;
        udpOut <-- dht4routing.udpOut;
//        trace_in --> dht4routing.trace_in;
        
}

