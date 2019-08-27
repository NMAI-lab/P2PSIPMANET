# P2PSIPMANET

This is a project exploring routing solutions for hierarchical MANETs.

The code in this repository is for the OMNET++ simulator and has dependencies on the projects Oversim and inetmanet.

To install and use:
- install OMNET++ version 4.6 (see omnetpp.org where the software and install guide can be found).
- clone projects inetmanet-2.0 and oversim in your working directory (github repos also available from NMAI-lab).
- clone project P2PSIPMANET into your working directory.
Import all three projects (file>import>general>existing projects into workspace) from the OMNET++ IDE.
- compile inetmanet (project > build project) then oversim, then MANETSIP (warning: it's slow, particularly for oversim).

- Yay. At this point there is one simulation that can be set up without error. However, the Global Nodelist is not properly updated so as soon as the nodes try to set up a DHT there is an error because the bootstrap mechanism doesn't work.
