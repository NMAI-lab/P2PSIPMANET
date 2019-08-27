def hostRoutes(minHosts, maxHosts, minDests, maxDests, gateway, interface):
  """ Writes the static routes for the host nodes. 
  Does not work fully since there is no way of knowing if the route required is
  a multihop route.

  Keyword arguments:
  minHosts -- Minimum number of hosts
  maxHosts -- Maximum number of hosts
  minDests -- Minimum number of destinations for other host nodes
  maxDests -- Maximum number of destinations for other host nodes
  gateway -- Associated to send out found packet through
  interface -- Associated interface to send packet to gateway through
  """
  # Create/open xml file
  with open("StaticRoute.xml", "a") as f:
    # Route for every host
    for i in range(minDests, maxDests):
      f.write("<route hosts='host[%i..%i]' destination='host[%i]' gateway='%s' netmask = '255.255.255.255' interface='%s'/>\n" %(minHosts,maxHosts, i, gateway, interface));

  return;

def gatewayRoutes(minDests, maxDests, clusters, interface):
  """ Writes the static routes for the gateway nodes.

  Keyword arguments:
  minDests -- Minimum number of destinations for other host nodes
  maxDests -- Maximum number of destinations for other host nodes
  clusters -- A two dimensional array of numbers holding the indices for each host node in their respective cluster index
  interface -- a String representing the interface parameter for the route
  """
  # Create/open xml file
  with open("StaticRoute.xml", "a") as f:
    foundIndex = -1;
    for gatewayNum in range(0, len(clusters)):
      for i in range(minDests, maxDests):

        # Find which cluster the current index host is in
        for j in range(0, len(clusters)):
          if i in clusters[j]:
            foundIndex = j;
            break;

        # Write a route if the host is not in the current cluster index (gatewayNum)
        if i not in clusters[gatewayNum]:
          # Check if the destination is below or above the current gateway number
          gatewayRoute = gatewayNum + 1 if gatewayNum < foundIndex else gatewayNum - 1;
          f.write("<route hosts='gw[%i]' destination='host[%i]' interface='%s' gateway='gw[%i]' netmask='255.255.255.255' />\n" %(gatewayNum, i, interface, gatewayRoute));
        
  return;

cluster0 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
cluster1 = []
cluster2 = [20, 21, 22, 23, 24, 25, 26, 27, 28, 29]
cluster3 = []
cluster4 = [10, 11, 12, 13, 14, 15, 16, 17, 18, 19]

minHosts = 0;
maxHosts = len(cluster0) + len(cluster1) + len(cluster2) + len(cluster3) + len(cluster4);

clusters = [cluster0, cluster1, cluster2, cluster3, cluster4];

gatewayRoutes(minHosts, maxHosts, clusters, "wlan1");
