To configure a static network the following actions should be taken with the files:
--------------------------------------------------------------------------------------
1. Put StaticGateway.ned in the "src" folder
2. Put the staticRoutes folder in the "src" folder
3. Put the DHT-basedStatic.ini in the simulations folder
4. Put the StaticHierarchical.ned in the simulations folder
5. Run the DHT-basedStatic.ini
6. Select and simulate TwoClustersStatic, ThreeClustersStatic, FourClustersStatic, FiveClustersStatic configurations.

To simulate a new static network:
--------------------------------------
1. Change the seed in the corresponding configuration in the DHT-basedStatic.ini
2. Remove all <route /> XML entries from the corresponding XML file in the staticRoutes folder
3. Edit the RouteHardcoder.py to the appropiate cluster information
	a) In the main section of the script, change the cluster0, cluster1, cluster2, cluster3, cluster4 arrays with integers representing the host nodes in each cluster
4. Run the RouteHardcoder.py using terminal:
	a) change directories to the folder
	b) execute command: python ./RouteHardcoder
5. Open StaticRoute.xml in the same folder
6. Copy the entries from the StaticRoute.xml to the corresponding XML configuration
7. Manually write <route /> entries from each host to their destination
	a) For intracluster routing
		i) If hosts are within range of each other, no routing entry is needed in the XML config
		ii) If they are not within range add appropriate routing entries with gateway (next-hop) parameter
	b) For intercluster routing
		i) Add routing entry from host node to the gateway (may have to perform multi-hop routing entries)
		ii) Add routing entries from associated cluster gatway to the host in its cluster using local interface (wlan0)
		iii) Add routing path on the reverse. Ex. If the initial ping is A -> B, the same routing entries must be made for B -> A on the reverse path
8. Run the edited configuration.
9. Validate that the paths work (should have a non-zero success rate in pings)
