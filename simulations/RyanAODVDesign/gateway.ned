package p2psipmanet.simulations.RyanAODVDesign;

import inet.networklayer.IManetRouting;
import p2psipmanet.simulations.RyanAODVDesign.StaticGateway;

module gateway extends StaticGateway
{
    parameters:
        @node;
        @display("i=device/cellphone;bgb=665,477");
        string routingProtocol @enum("AODVUU","DYMOUM","DYMO","DSRUU","OLSR","OLSR_ETX","DSDV_2","Batman") = default("OLSR");  // Protocol for cluster nodes
        string backboneRoutingProtocol @enum("AODVUU","DYMOUM","DYMO","DSRUU","OLSR","OLSR_ETX","DSDV_2","Batman") = default("OLSR"); // Protocol for cluster nodes
    submodules:
        // Routing for clusters
        manetrouting: <routingProtocol> like IManetRouting if routingProtocol != "" {
            @display("p=522,307");
        }

        // Routing protocol for backbone
        backbonemanetrouting: <backboneRoutingProtocol> like IManetRouting {
            @display("p=558,338");
        }
        
    connections allowunconnected:
        // Connect cluster routing protocol
        networkLayer.transportOut++ --> manetrouting.from_ip if routingProtocol != "";
        networkLayer.transportIn++ <-- manetrouting.to_ip if routingProtocol != "";

        // Connect Backbone Routing protocol to Network Layer
        backbonemanetrouting.to_ip --> networkLayer.transportIn++;
        backbonemanetrouting.from_ip <-- networkLayer.transportOut++;
}
