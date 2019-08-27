/*
 * DHT4Routing.cc
 *
 *  Created on: Aug 22, 2017
 *      Author: silas
 */

#include <IPvXAddressResolver.h>
#include <GlobalNodeListAccess.h>
#include <GlobalNodeList.h>
#include <GlobalStatisticsAccess.h>
#include <UnderlayConfiguratorAccess.h>
#include <RpcMacros.h>
#include "IPv4Route.h"
#include "IPv4.h"
#include "IPv4Datagram.h"
#include "IPv4ControlInfo.h"
#include "NotificationBoard.h"
#include "CommonMessages_m.h"
#include "NotifierConsts.h"
#include "SimpleInfo.h"
#include "BaseOverlay.h"
#include "InterfaceTableAccess.h"
#include "RoutingTableAccess.h"
#include "OLSR_rtable.h"
#include "IRoutingTable.h"

#include <GlobalDhtTestMap.h>
#include "DHT4Routing.h"

Define_Module(DHT4Routing);

using namespace std;
bool inserted = false;

DHT4Routing::~DHT4Routing() {
    cancelAndDelete(PUT_timer);
    cancelAndDelete(GET_timer);
    cancelAndDelete(DHT_checker);
}

DHT4Routing::DHT4Routing() {
    PUT_timer = NULL;
    GET_timer = NULL;
    DHT_checker = NULL;
    notificationBoard = NULL;
}

void DHT4Routing::initializeApp(int stage) {
    if (stage != MIN_STAGE_APP)
        return;

    // fetch parameters
    debugOutput = par("debugOutput");
    activeNetwInitPhase = par("activeNetwInitPhase");

    mean = par("testInterval");
    p2pnsTraffic = par("p2pnsTraffic");
    deviation = mean / 10;

    cout<< "mean : "<<mean << " ; " <<"deviation : "<<deviation <<endl;

    if (p2pnsTraffic) {
        ttl = 3600 * 24 * 365;
    } else {
        ttl = par("testTtl");
    }

    globalNodeList = GlobalNodeListAccess().get();
    underlayConfigurator = UnderlayConfiguratorAccess().get();
    globalStatistics = GlobalStatisticsAccess().get();

    globalDhtTestMap =
            dynamic_cast<GlobalDhtTestMap*>(simulation.getModuleByPath(
                    "globalObserver.globalFunctions[0].function"));

    if (globalDhtTestMap == NULL) {
        throw cRuntimeError("DHT4Routing::initializeApp(): "
                "GlobalDhtTestMap module not found!");
    }

    // statistics
    numSent = 0;
    numGetSent = 0;
    numGetError = 0;
    numGetSuccess = 0;
    numPutSent = 0;
    numPutError = 0;
    numPutSuccess = 0;

    //initRpcs();
    WATCH(numSent);
    WATCH(numGetSent);
    WATCH(numGetError);
    WATCH(numGetSuccess);
    WATCH(numPutSent);
    WATCH(numPutError);
    WATCH(numPutSuccess);

    nodeIsLeavingSoon = false;

    // subscribe to NotificationBoard
    nb = NotificationBoardAccess().getIfExists();
    notificationBoard->subscribe(this, NF_IPv4_ROUTE_ADDED);
    notificationBoard->subscribe(this, NF_IPv4_ROUTE_DELETED);
    notificationBoard->subscribe(this, NF_IPv4_ROUTE_REMOVED);
    notificationBoard->subscribe(this, NF_IPv4_ROUTE_CHANGED);

    notificationBoard->subscribe(this, NF_IPv4_MROUTE_ADDED);
    notificationBoard->subscribe(this, NF_IPv4_MROUTE_DELETED);
    notificationBoard->subscribe(this, NF_IPv4_MROUTE_CHANGED);
    notificationBoard->subscribe(this, GET_DESTINATION_IP_FROM_BACKBONE);
    notificationBoard->subscribe(this, GET_RESPONSE_FROM_BACKBONE);

}

void DHT4Routing::GetScheduler(cMessage *msg) {

    scheduleAt(simTime() + (truncnormal(mean, deviation)/4), msg);

}

void DHT4Routing::PutScheduler(cMessage *msg) {

    scheduleAt(simTime() + truncnormal(mean, deviation), msg);

}

void DHT4Routing::DHT_checkScheduler(cMessage *msg) {

    scheduleAt(simTime() + truncnormal(mean/2.2, deviation/3), msg);

}

void DHT4Routing::handleRpcResponse(BaseResponseMessage* msg,
        const RpcState& state, simtime_t rtt) {
    RPC_SWITCH_START(msg)
                                                    RPC_ON_RESPONSE( DHTputCAPI)
                                                    {
        handlePutResponse(_DHTputCAPIResponse,
                check_and_cast<DHTStatsContext*>(state.getContext()));
        EV << "[DHT4Routing::handleRpcResponse()]\n"
                << "    DHT Put RPC Response received: id="
                << state.getId() << " msg=" << *_DHTputCAPIResponse
                << " rtt=" << rtt << endl;
        break;
                                                    }
    RPC_ON_RESPONSE(DHTgetCAPI)
    {
        handleGetResponse(_DHTgetCAPIResponse,
                check_and_cast<DHTStatsContext*>(state.getContext()));
        EV << "[DHT4Routing::handleRpcResponse()]\n"
                << "    DHT Get RPC Response received: id="
                << state.getId() << " msg=" << *_DHTgetCAPIResponse
                << " rtt=" << rtt << endl;
        break;
    }RPC_SWITCH_END()
}

void DHT4Routing::handlePutResponse(DHTputCAPIResponse* msg,
        DHTStatsContext* context) {
    DHTEntry entry = { context->value, simTime() + ttl, simTime() };
    putControl *putCtrl;

    try{
        if(check_and_cast<putControl *>(context->msg->getControlInfo()) != NULL){
            putCtrl = check_and_cast< putControl *>(context->msg->getControlInfo());
        }

    }catch(...){}

    if (context->measurementPhase == false) {
        // don't count response, if the request was not sent
        // in the measurement phase

        PUT_timer = new cMessage("DHT_PUT_timer");
        putControl *ctrl = new putControl();
        ctrl->key_ = context->key;
        ctrl->value_ = context->value;
        ctrl->gateway_ = putCtrl->gateway_;
        ctrl->host_ = putCtrl->host_;
        PUT_timer->setKind(DHTPut);
        PUT_timer->setControlInfo(ctrl);

        scheduleAt(simTime() + truncnormal(mean + 2 * mean / 3, deviation),
                PUT_timer);
        delete context->msg->removeControlInfo();
        delete context->msg;
        delete context;
        return;
    }

    if (msg->getIsSuccess()) {

        //only insert key into testmap if it was successfully put.
        globalDhtTestMap->insertEntry(context->key, entry);

        RECORD_STATS(numPutSuccess++);
        RECORD_STATS(
                globalStatistics->addStdDev("DHT4Routing: PUT Latency (s)", SIMTIME_DBL(simTime() - context->requestTime)));

        cout << "DHT4Routing: PUT Success [t=" << simTime() << "]" << "for: "
                << putCtrl->gateway_ << " ; " << context->key << " ; " << putCtrl->host_ <<endl;


        for (auto it = item().begin(); it != item().end();) {

            //cache all successful PUTS for tracking against DHT entries

            Cache prevItem = *it;
            if ((prevItem.dest_addr() == putCtrl->host_)) {
                Cache newItem;
                newItem.setDest_addr(prevItem.dest_addr());
                newItem.setGw_addr(prevItem.gw_addr());
                newItem.setKey(prevItem.key());
                newItem.setSeq(true);
                newItem.setHopCount(prevItem.hopCount());
                newItem.setTimeCreated(CURRENT_TIME);
                newItem.setInDHT(true);
                it = item().erase(it);
                insert_Cache(newItem);
                return;
            }else{
                ++it;
            }
        }

        delete context->msg->removeControlInfo();
        delete context->msg;
        delete context;
        return;

    } else {
        //do this if and only if the PUT was not successful

        PUT_timer = new cMessage("DHT_PUT_timer");
        putControl *ctrl = new putControl();
        ctrl->key_ = context->key;
        ctrl->value_ = context->value;
        ctrl->gateway_ = putCtrl->gateway_;
        ctrl->host_ = putCtrl->host_;
        PUT_timer->setKind(addRoute);
        PUT_timer->setControlInfo(ctrl);
        PutScheduler(PUT_timer);
        cout<< "DHT4Routing: PUT failed [t=" << simTime() << "]" << "for: "
                << putCtrl->gateway_ << " ; " << context->key <<endl;

        RECORD_STATS(numPutError++);
        delete context->msg->removeControlInfo();
        delete context->msg;
        delete context;
        return;
    }
    delete context->msg->removeControlInfo();
    delete context->msg;
    delete context;
}

void DHT4Routing::handleGetResponse(DHTgetCAPIResponse* msg,
        DHTStatsContext* context) {

    if (!(context->measurementPhase)) {
        // don't count response, if the request was not sent
        // in the measurement phase
        delete context->msg->removeControlInfo();
        delete context->msg;
        delete context;
        return;
    }
    //


    RECORD_STATS(
            globalStatistics->addStdDev("DHT4Routing: GET Latency (s)", SIMTIME_DBL(simTime() - context->requestTime)));

    if (!(msg->getIsSuccess())) {

        //If the GET was not successful for any of the reasons stated below, do the following :

        getControl *getCtrl;
        GET_timer = new cMessage("DHT_GET_timer");

        /* if this was a GET preceding a route advertisement in the DHT, a route request from the DHT,
         or a route delete from the DHT, then create the control information as follows :  */

        if((context->msg->getKind() == addRoute) || (context->msg->getKind() == DHTGet)){
            try{
                if(check_and_cast<getControl *>(context->msg->getControlInfo()) != NULL){

                    getCtrl = check_and_cast< getControl *>(context->msg->getControlInfo());
                    getControl *ctrl = new getControl();
                    ctrl->key_ = context->key;
                    ctrl->value_ = context->value;
                    ctrl->gateway_ = getCtrl->gateway_;
                    ctrl->host_ = getCtrl->host_;
                    GET_timer->setControlInfo(ctrl);
                }

            }catch(...){}
        }

        /* if this was a GET preceding a route advertisement in the DHT, then do this to reschedule the GET */

        if(context->msg->getKind() == addRoute){
            GET_timer->setKind(addRoute);
            GetScheduler(GET_timer);
            cout << "DHT4Routing: GET failed for add route [t=" << simTime() << "]" << getCtrl->gateway_ << endl;
            RECORD_STATS(numGetError++);
            delete context->msg->removeControlInfo();
            delete context->msg;
            delete context;
            return;
        }

        /* if this was a GET preceding a route delete from the DHT, then do this to reschedule the GET */
        if(context->msg->getKind() == DHT_Get_and_Delete){
            GET_timer->setKind(DHT_Get_and_Delete);
            GetScheduler(GET_timer);
            cout << "DHT4Routing: GET failed for delete route [t=" << simTime() << "]" << getCtrl->gateway_ << endl;
            RECORD_STATS(numGetError++);
            delete context->msg->removeControlInfo();
            delete context->msg;
            delete context;
            return;
        }

        /* if this was a route request from the DHT, then do this to reschedule the GET */
        GET_timer->setKind(DHTGet);
        GetScheduler(GET_timer);
        delete context->msg->removeControlInfo();
        delete context->msg;
        delete context;
        return;
    }

    const DHTEntry* entry = globalDhtTestMap->findEntry(context->key);

    /* if an entry was retrieved from the DHT but an unexpected entry,
    then do the following*/

    if ((entry == NULL)) { //unexpected key

        GET_timer = new cMessage("DHT_GET_timer");
        getControl *getCtrl;

        if((context->msg->getKind() == addRoute) || (context->msg->getKind() == DHTGet)
                ||(context->msg->getKind() == DHT_Get_and_Delete)){
            try{
                if(check_and_cast<getControl *>(context->msg->getControlInfo()) != NULL){

                    getCtrl = check_and_cast< getControl *>(context->msg->getControlInfo());
                    getControl *ctrl = new getControl();
                    ctrl->key_ = context->key;
                    ctrl->value_ = context->value;
                    ctrl->gateway_ = getCtrl->gateway_;
                    ctrl->host_ = getCtrl->host_;
                    GET_timer->setControlInfo(ctrl);
                }

            }catch(...){}
        }

        if(context->msg->getKind() == addRoute){
            PUT_timer = new cMessage("DHT_PUT_timer");
            putControl *ctrl = new putControl();
            ctrl->key_ = context->key;
            ctrl->value_ = context->value;
            ctrl->gateway_ = getCtrl->gateway_;
            ctrl->host_ = getCtrl->host_;
            PUT_timer->setControlInfo(ctrl);
            PUT_timer->setKind(addRoute);
            sendPUT(getCtrl->host_, getCtrl->hops_, PUT_timer);
            delete context->msg->removeControlInfo();
            delete context->msg;
            delete context;
            delete GET_timer;
            return;
        }

        //todo
        if(context->msg->getKind() == DHT_Get_and_Delete){
            delete context->msg->removeControlInfo();
            delete context->msg;
            delete context;
            delete GET_timer;
            return;
        }

        if(context->msg->getKind() == DHTGet){
            RECORD_STATS(numGetError++);
            cout << "DHT4Routing: GET failed [t=" << simTime() << "] unexpected value"
                    << " ; " << context->key << " ; " << getCtrl->host_<< " ; "<<endl;

            choose_back_Off(getCtrl->host_, context->key, true);

            delete context->msg->removeControlInfo();
            delete context->msg;
            delete context;
            delete GET_timer;
            return;
        }
        delete context->msg->removeControlInfo();
        delete context->msg;
        delete context;
        return;
    }

    if (simTime() > entry->endtime) {
        //this key doesn't exist anymore in the DHT, delete it in our hashtable

        globalDhtTestMap->eraseEntry(context->key);
        delete context->msg->removeControlInfo();
        delete context->msg;
        delete context;

        if (msg->getResultArraySize() > 0) {
            RECORD_STATS(numGetError++);
            cout << "DHT4Routing: GET failed [t=" << simTime() << "] deleted key still available" << endl;
            return;
        } else {
            RECORD_STATS(numGetSuccess++);
            cout << "DHT4Routing: GET success [t=" << simTime() << "]" << endl;
            cout << "key = " << context->value << " ; " << "value = "
                    << entry->value << endl;
            return;
        }
    } else {


        /*The GET was successful and returned a real value.
         * according to the kind of GET, process the GET success*/

        if ((msg->getResultArraySize() > 0)
                && (msg->getResult(0).getValue() == entry->value)) {
            RECORD_STATS(numGetSuccess++);
            cout << "DHT4Routing: GET success [t=" << simTime() << "]" << endl;


            const char* interface = par("overlayInterface");
            IPv4Address thisGateway = IPvXAddressResolver().addressOf(
                    getParentModule()->getParentModule(), interface).get4();

            getControl* getCtrl;

            try{
                if(check_and_cast<getControl *>(context->msg->getControlInfo()) != NULL)
                {
                    getCtrl = check_and_cast< getControl *>(context->msg->getControlInfo());
                }

            }catch(...){}

            /*remove this entry from the cache which holds unsuccessful gets
             * to track redundant retries
             */
            for(auto it = value().begin(); it != value().end();){
                back_off_cache entry = *it;
                if(entry.host() == getCtrl->host_){
                    it = value().erase(it);
                    break;
                }else{
                    ++it;
                }
            }


            /*in the case of mobility, this often comes as a list of entries
             * the entries need to be de-serialized and processed individually*/

            std::vector<char> buffer(entry->value.begin(), entry->value.end());
            std::vector<Entry> entries = deserialize(buffer);

            if(context->msg->getKind() == addRoute){

                /*when a node switched between clusters, the new gateway
                 * has to add its self to the list of gateways which can potentially
                 * reach this node.*/

                for (auto entry : entries) {
                    if(entry.ip == thisGateway){
                        delete context->msg->removeControlInfo();
                        delete context->msg;
                        delete context;
                        return;
                    }
                }
                PUT_timer = new cMessage("DHT_PUT_timer");
                putControl *ctrl = new putControl();
                ctrl->key_ = context->key;
                ctrl->value_ = context->value;
                ctrl->gateway_ = getCtrl->gateway_;
                ctrl->host_ = getCtrl->host_;
                PUT_timer->setControlInfo(ctrl);
                PUT_timer->setKind(addRoute);
                PUT_updatedList(context, buffer, getCtrl->hops_, PUT_timer);

                delete context->msg->removeControlInfo();
                delete context->msg;
                delete context;
                return;
            }

            if(context->msg->getKind() == DHTGet){
                /*where the request was a route request,
                 * (it is possible that multiple gateways may exist to a particular destination node).
                 * therefore, calculate the best route and pass it down to the
                 * network layer for routing as follows : */

                BestRouteCalculation(context, entry);
                delete context->msg->removeControlInfo();
                delete context->msg;
                delete context;
                return;
            }

            if(context->msg->getKind() == DHT_Get_and_Delete){

                /*where this route exists already in the DHT but has to be removed because it is no longer valid
                 * then, remove the stale entry and update the DHT as follows : */

                for (auto it = item().begin(); it != item().end();) {
                    Cache prevItem = *it;
                    if (prevItem.dest_addr() == getCtrl->host_) {
                        it = item().erase(it);
                        break;
                    }else {
                        ++it;
                    }
                }


                std::vector<char> newList;
                std::vector<Entry> listEntries;
                int defaultHops = -1;
                int capacity = entries.capacity();

                for (auto entry : entries) {
                    if((capacity<2) && (entry.ip == thisGateway)){
                        if(!(dhtChecker(getCtrl, context))){
                            delete context->msg->removeControlInfo();
                            delete context->msg;
                            delete context;
                            return;
                        }
                    }
                    if(entry.ip != thisGateway){
                        if(entry.ip.isUnspecified()){
                            delete context->msg->removeControlInfo();
                            delete context->msg;
                            delete context;
                            return;
                        }
                        listEntries.push_back(Entry(entry.ip, entry.hops));
                    }else{}
                }

                newList = serialize(listEntries);

                PUT_timer = new cMessage("DHT_PUT_timer");
                putControl *ctrl = new putControl();
                ctrl->key_ = context->key;
                ctrl->value_ = context->value;
                ctrl->gateway_ = getCtrl->gateway_;
                ctrl->host_ = getCtrl->host_;
                PUT_timer->setControlInfo(ctrl);
                PUT_timer->setKind(addRoute);
                PUT_updatedList(context, newList, defaultHops, PUT_timer);
                delete context->msg->removeControlInfo();
                delete context->msg;
                delete context;
                return;
            }

            delete context->msg->removeControlInfo();
            delete context->msg;
            delete context;
            return;

        } else {
            getControl *getCtrl;

            try{
                if(check_and_cast<getControl *>(context->msg->getControlInfo()) != NULL){
                    getCtrl = check_and_cast< getControl *>(context->msg->getControlInfo());
                }

            }catch(...){}

            choose_back_Off(getCtrl->host_, context->key, true);

            cout << "DHT4Routing: GET failed [t=" << simTime() << "]" << " for thisHost: " << getCtrl->host_ << endl;
            RECORD_STATS(numGetError++);
#if 0
            if (msg->getResultArraySize()) {
                cout << "DHT4Routing: wrong value: " << msg->getResult(0).getValue() << endl;
            } else {
                cout << "DHT4Routing: no value" << endl;
            }
#endif
            return;
        }
    }

}

//TODO
void DHT4Routing::handleTimerEvent(cMessage* msg) {
    bool isPutTimer = msg->isName("DHT_PUT_timer");
    bool isGetTimer = msg->isName("DHT_GET_timer");
    bool isDHTChecker = msg->isName("DHT_checker");
    OverlayKey destKey = OverlayKey::UNSPECIFIED_KEY;
    IPv4Address destAddr;

    if (!(isPutTimer || isGetTimer || isDHTChecker)) {

        delete msg->removeControlInfo();
        delete msg;
        return;
    }

    // do nothing if the network is still in the initialization phase
    if (((!activeNetwInitPhase) && (underlayConfigurator->isInInitPhase()))
            || underlayConfigurator->isSimulationEndingSoon()
            || nodeIsLeavingSoon) {

        delete msg->removeControlInfo();
        delete msg;
        return;
    }


    if (isGetTimer) {

        /* The different scheduled GET requests are initiated here.
         * according to their different types, they are distinguished*/

        if((msg->getKind() == addRoute) || (msg->getKind() == DHTGet)
                || (msg->getKind() == DHT_Get_and_Delete)){
            try{
                if(check_and_cast<getControl *>(msg->getControlInfo()) != NULL){
                    getControl *ctrl = check_and_cast< getControl *>(msg->getControlInfo());
                    if((msg->getKind() == addRoute) || (msg->getKind() == DHT_Get_and_Delete))
                    {
                        verify_Routing_table(ctrl->host_, msg);
                        return;
                    }
                    if((msg->getKind() == DHTGet))
                    {
                        sendGET(ctrl->host_, msg);
                        return;
                    }
                }
            }catch(...){}
        }

        delete msg->removeControlInfo();
        delete msg;
    }

    if (isPutTimer) {
        //--------put route in DHT
        if((msg->getKind() == DHTPut) || (msg->getKind() == addRoute)){
            if(check_and_cast<putControl *>(msg->getControlInfo()) != NULL){
                putControl *ctrl = check_and_cast< putControl *>(msg->getControlInfo());
                sendPUT(ctrl->host_, ctrl->hops_, msg);
                return;
            }
        }
    }

    if(isDHTChecker){

        /*this function will cross check with the cache to make sure
         * that the relevant entries were advertised in the DHT as should
         * and will reschedule entries that were not successful*/

        if(msg->getKind() == addRoute){
            if(check_and_cast<getControl *>(msg->getControlInfo()) != NULL){
                getControl *getCtrl = check_and_cast< getControl *>(msg->getControlInfo());
                for (cacheVector::iterator it = item().begin(); it != item().end(); it++) {
                    Cache prevItem = *it;
                    if ((prevItem.dest_addr() == getCtrl->host_) && (!prevItem.inDHT()) && (prevItem.timeCreated()<CURRENT_TIME)) {
                        GET_timer = new cMessage("DHT_GET_timer");//message to schedule a get before a put for each entry
                        getControl *ctrl = new getControl();
                        ctrl->gateway_ = getCtrl->gateway_;
                        ctrl->host_ = getCtrl->host_;
                        ctrl->hops_ = getCtrl->hops_;
                        GET_timer->setControlInfo(ctrl);
                        GET_timer->setKind(addRoute);
                        sendGET(getCtrl->host_, GET_timer);
                        delete msg->removeControlInfo();
                        delete msg;
                        return;


                        //schedule new checker
                        DHT_checker = new cMessage("DHT_checker");//message to schedule a get before a put for each entry
                        DHT_checker->setControlInfo(ctrl);
                        DHT_checker->setKind(addRoute);
                        DHT_checkScheduler(DHT_checker);
                        return;
                    }
                    if ((prevItem.dest_addr() == getCtrl->host_) && (prevItem.inDHT())) {
                        delete msg->removeControlInfo();
                        delete msg;
                        return;
                    }
                }
            }
        }
    }
}

void DHT4Routing::sendGET(IPv4Address addr, cMessage* msg) {

    if (simTime() < par("DHT4RoutingtransitionTime")) {

        getControl *ctrl = new getControl();
        getControl *getCtrl;
        GET_timer = new cMessage("DHT_GET_timer");

        try{
            if(check_and_cast<getControl *>(msg->getControlInfo()) != NULL){
                getCtrl = check_and_cast< getControl *>(msg->getControlInfo());
                ctrl->key_ = getCtrl->key_;
                ctrl->value_ = getCtrl->value_;
                ctrl->gateway_ = getCtrl->gateway_;
                ctrl->host_ = getCtrl->host_;
                GET_timer->setControlInfo(ctrl);
            }

        }catch(...){}

        if(msg->getKind() == addRoute){
            GET_timer->setKind(addRoute);
            GetScheduler(GET_timer);
            //            delete msg;
        }
        if(msg->getKind() == DHTGet){
            GET_timer->setKind(DHTGet);
            GetScheduler(GET_timer);
            //            delete msg;
        }
        if(msg->getKind() == DHT_Get_and_Delete){
            delete msg->removeControlInfo();
            delete msg;
            return;
        }
        delete msg->removeControlInfo();
        delete msg;
        return;
    }

    /*cache all GET queries for tracking against the GET responses*/

    if(check_and_cast<getControl *>(msg->getControlInfo()) != NULL){
        getControl *getCtrl = check_and_cast< getControl *>(msg->getControlInfo());
        DHT_checker = new cMessage("DHT_checker");//message to schedule a get before a put for each entry
        getControl *ctrl = new getControl();
        ctrl->key_ = getCtrl->key_;
        ctrl->value_ = getCtrl->value_;
        ctrl->gateway_ = getCtrl->gateway_;
        ctrl->host_ = getCtrl->host_;
        DHT_checker->setControlInfo(ctrl);
        DHT_checker->setKind(addRoute);
        DHT_checkScheduler(DHT_checker);
    }

    for (gwCacheVector::iterator it = gwItem().begin(); it != gwItem().end();
            it++) {
        gwInfoCache prevItem = *it;
        if (prevItem.gw_addr() == addr)
            return;
    }

    const char* destAddr = addr.str().c_str();
    const OverlayKey& key = OverlayKey::sha1(BinaryValue(destAddr));

    if (key.isUnspecified()) {
        EV << "[DHT4Routing::handleTimerEvent() @ " << addr << " ("
                << key.toString(16) << ")]\n"
                << "    Error: No key available in global DHT test map!"
                << endl;
        return;
    }
    if(msg->getKind() == DHTGet){
        TempRoutingTable(addr, key);
    }

    DHTgetCAPICall* dhtGetMsg = new DHTgetCAPICall();
    dhtGetMsg->setKey(key);
    RECORD_STATS(numSent++; numGetSent++);

    sendInternalRpcCall(TIER1_COMP, dhtGetMsg,
            new DHTStatsContext(globalStatistics->isMeasuring(), simTime(),
                    key, BinaryValue::UNSPECIFIED_VALUE, msg));

    cout << "DHT4Routing: GET sent for key: " << key << "; [t=" << simTime() << "]" << endl;

}

void DHT4Routing::sendPUT(IPv4Address &addr, int &hops, cMessage* msg) {

    // create a vector of put messages with destination key set to each node in the routing table

    const char* interface = par("overlayInterface");
    IPv4Address gatewayAddress = IPvXAddressResolver().addressOf(
            getParentModule()->getParentModule(), interface).get4();
    OverlayKey destKey = OverlayKey::sha1(BinaryValue(addr.str()));
    if (destKey.isUnspecified()) {
        return;
    }

    std::vector<char> gatewayList;


    std::vector<Entry> listEntries;
    listEntries.push_back(Entry(gatewayAddress, hops));
    gatewayList = serialize(listEntries);

    DHTputCAPICall* dhtPutMsg = new DHTputCAPICall();
    dhtPutMsg->setKey(destKey);
    dhtPutMsg->setValue(BinaryValue(gatewayList));
    dhtPutMsg->setTtl(ttl);
    dhtPutMsg->setIsModifiable(true);

    RECORD_STATS(numSent++; numPutSent++);
    sendInternalRpcCall(TIER1_COMP, dhtPutMsg,
            new DHTStatsContext(globalStatistics->isMeasuring(), simTime(),
                    destKey, dhtPutMsg->getValue(), msg));

}



void DHT4Routing::PUT_updatedList(DHTStatsContext* context, const std::vector<char> &buffer, int &hops, cMessage* msg) {

    /*this function is called when ever there is to be an update of
     * most resent routing information in the DHT. For example, a node
     * leaves a cluster or a node arrives in a new cluster
     * */

    const char* interface = par("overlayInterface");
    IPv4Address gatewayAddress = IPvXAddressResolver().addressOf(
            getParentModule()->getParentModule(), interface).get4();
    OverlayKey destKey = context->key;
    if (destKey.isUnspecified()) {
        return;
    }
    std::vector<char> gatewayList;
    std::vector<Entry> listEntries;

    if(hops == -1)
    {
        std::vector<Entry> entries = deserialize(buffer);
        for (auto entry : entries)
        {
            IPv4Address gateway = entry.ip;
            int hopCount = entry.hops;
            listEntries.push_back(Entry(gateway, hopCount));
        }

    } else {
        listEntries.push_back(Entry(gatewayAddress, hops));
        std::vector<Entry> entries = deserialize(buffer);
        for (auto entry : entries) {
            IPv4Address gateway = entry.ip;
            int hopCount = entry.hops;
            if(gateway != gatewayAddress){
                listEntries.push_back(Entry(gateway, hopCount));
            }
        }
    }

    gatewayList = serialize(listEntries);

    DHTputCAPICall* dhtPutMsg = new DHTputCAPICall();
    dhtPutMsg->setKey(destKey);
    dhtPutMsg->setValue(BinaryValue(gatewayList));
    dhtPutMsg->setTtl(ttl);
    dhtPutMsg->setIsModifiable(true);

    RECORD_STATS(numSent++; numPutSent++);
    sendInternalRpcCall(TIER1_COMP, dhtPutMsg,
            new DHTStatsContext(globalStatistics->isMeasuring(), simTime(),
                    destKey, dhtPutMsg->getValue(), msg));

}

void DHT4Routing::BestRouteCalculation(DHTStatsContext* context, const DHTEntry *entry){

    /*whenever there exists multiple gateways to a destination host,
     * then the one with the least hop count through the backbone is selected*/

    IPv4Datagram data;
    bool updated = false;
    int bestHop = -1;
    //    int count = 0;
    IPv4Address bestGateWay;
    IPv4Address destinationNode;
    IPv4Address tempGateWay;
    uint32_t hopCount;

    const char* interface = par("overlayInterface");
    IPv4Address thisGateway = IPvXAddressResolver().addressOf(
            getParentModule()->getParentModule(), interface).get4();

    std::vector<char> buffer(entry->value.begin(), entry->value.end());

    std::vector<Entry> entries = deserialize(buffer);


    for (auto entry : entries) {
        if(entry.ip != thisGateway){
            tempGateWay = entry.ip;
            hopCount = entry.hops;

            for (tempVector::iterator it = num().begin(); it != num().end();
                    it++) {
                Temp_table prevNum = *it;

                if (prevNum.key() == context->key) {
                    prevNum.setDestGw_addr(tempGateWay);
                    prevNum.setHopCount(hopCount);
                    destinationNode = prevNum.dest_addr();
                    updated = true;
                }
            }


            if(updated){

                for(gwCacheVector::iterator it = gwItem().begin(); it != gwItem().end();
                        it++) {
                    gwInfoCache prevNum = *it;

                    if(prevNum.gw_addr() == tempGateWay)
                    {
                        if(bestHop == -1)
                        {
                            bestHop = prevNum.gwHopCount();
                            bestGateWay = prevNum.gw_addr();
                        }else{
                            if(prevNum.gwHopCount() <= bestHop)
                            {
                                bestHop = prevNum.gwHopCount();
                            }
                        }

                    }
                }
            }
        }
    }

    data.setSrcAddress(bestGateWay);
    data.setDestAddress(destinationNode);
    nb->fireChangeNotification(GET_RESPONSE_FROM_BACKBONE, &data);
    return;
}





//** when a node leaves/joins a cluster, this function will remove/add
//the destination gateway from/to the list of possible destination
//gateway entries that can reach this node.

void DHT4Routing::receiveChangeNotification(int category,
        const cObject *details) {
    // ignore notifications during initialize phase
    if (simulation.getContextType() == CTX_INITIALIZE)
        return;

    Enter_Method_Silent();
    //printNotificationBanner(category, details);

    //cast cObject *details into something useful
    if (category == NF_IPv4_ROUTE_ADDED) {
        IPv4Route *route = const_cast<IPv4Route*>(check_and_cast<
                const IPv4Route*>(details));
        const char *iface = route->getInterfaceName();
        const char *ie = "wlan0";

        if (strcmp(iface, ie) != 0) {
            IPv4Address gatewayNode = route->getDestination();
            int gwHops = route->getMetric();
            BackBoneGwCache(gatewayNode, gwHops);
            return;
        }

        IPv4Address destination = route->getDestination();
        int hops = route->getMetric();
        const char* interface = par("overlayInterface");
        IPv4Address gateway = IPvXAddressResolver().addressOf(
                getParentModule()->getParentModule(), interface).get4();

        for (cacheVector::iterator it = item().begin(); it != item().end(); it++) {
            Cache prevItem = *it;
            if ((prevItem.dest_addr() == destination) && (prevItem.seq())
            /*&& prevItem.cycle()>=4*/) {
                return;
            }
        }
        TempCache(destination, gateway, hops, CURRENT_TIME); //call to cache all local MANET routes from the underlay network.

        GET_timer = new cMessage("DHT_GET_timer");//message to schedule a get before a put for each entry
        getControl *ctrl = new getControl();
        ctrl->gateway_ = gateway;
        ctrl->host_ = route->getDestination();
        ctrl->hops_ = route->getMetric();
        GET_timer->setControlInfo(ctrl);
        GET_timer->setKind(addRoute);
        GetScheduler(GET_timer); //schedule a put for each route entry
        return;
    }
    if (category == GET_DESTINATION_IP_FROM_BACKBONE) {
        IPv4Datagram *data = const_cast<IPv4Datagram*>(check_and_cast<
                const IPv4Datagram*>(details));

        for(auto it = value().begin(); it != value().end();){
            back_off_cache entry = *it;
            if((entry.host() == data->getDestAddress())
                    && (entry.backoff() >= CURRENT_TIME)){
                return;
            }else if((entry.host() == data->getDestAddress())
                    && (entry.backoff() < CURRENT_TIME) && (entry.backoff() != -1)){
                back_off_cache newValue;
                newValue.setHost(data->getDestAddress());
                newValue.setSequence(0);
                newValue.setBackoff(-1);
                it = value().erase(it);
                value().push_back(newValue);
                break;
            }else{
                ++it;
            }
        }

        GET_timer = new cMessage("DHT_GET_timer");//message to schedule a get before a put for each entry
        getControl *ctrl = new getControl();
        ctrl->gateway_ = data->getSrcAddress();
        ctrl->host_ = data->getDestAddress();
        GET_timer->setControlInfo(ctrl);
        GET_timer->setKind(DHTGet);
        sendGET(data->getDestAddress(), GET_timer);
        return;
    }

    if (category == NF_IPv4_ROUTE_REMOVED) {
        //TODO
        IPv4Route *route = const_cast<IPv4Route*>(check_and_cast<
                const IPv4Route*>(details));

        const char* interface = par("overlayInterface");
        IPv4Address gateway = IPvXAddressResolver().addressOf(
                getParentModule()->getParentModule(), interface).get4();
        //        TempCache(dest, gateway, hops);
        for (cacheVector::iterator it = item().begin(); it != item().end(); it++) {
            Cache prevItem = *it;
            if (prevItem.dest_addr() == route->getDestination() && (prevItem.seq()) && (prevItem.inDHT())) {
                GET_timer = new cMessage("DHT_GET_timer");//message to schedule a get before a delete
                getControl *ctrl = new getControl();
                ctrl->gateway_ = gateway;
                ctrl->host_ = route->getDestination();
                GET_timer->setControlInfo(ctrl);
                GET_timer->setKind(DHT_Get_and_Delete);
                //GetScheduler(GET_timer); //schedule a put for each route entry
                scheduleAt(simTime() + (truncnormal(mean, deviation)/3), GET_timer);
                return;
            }
        }
    }
    if (category == NF_IPv4_ROUTE_CHANGED || NF_IPv4_MROUTE_CHANGED) {
        //TODO
    }

}

/*track all GETS and choose a backoff interval when they do not succeed
 * or return a null entry
 */
void DHT4Routing::choose_back_Off(IPv4Address host, OverlayKey destKey, bool backoff){
    int sequence = 4;

    for(auto it = value().begin(); it != value().end();){
        back_off_cache thisEntry = *it;
        if(thisEntry.host() == host){
            if(thisEntry.sequence() < sequence){
                back_off_cache newValue;
                newValue.setHost(host);
                newValue.setSequence(thisEntry.sequence()+=1);
                newValue.setBackoff(-1);
                it = value().erase(it);
                value().push_back(newValue);
                return;
            }
            if(thisEntry.sequence() >= sequence){
                back_off_cache newValue;
                newValue.setHost(host);
                newValue.setSequence(0);
                newValue.setBackoff(CURRENT_TIME + truncnormal(BACK_OFF, 0.4*BACK_OFF));
                it = value().erase(it);
                value().push_back(newValue);
                return;
            }
        }else{
            ++it;
        }
    }

    if(backoff){
        back_off_cache newValue;
        newValue.setHost(host);
        newValue.setSequence(0);
        newValue.setBackoff(-1);
        value().push_back(newValue);
        return;
    }
}

//temporary repository which creates an entry for the
//local MANET routing table entries from the
//underlay MANET network to be added to the DHT.
void DHT4Routing::TempCache(IPv4Address host, IPv4Address gateway, int hops, double time) {
    //Cache
    for (cacheVector::iterator it = item().begin(); it != item().end(); it++) {
        Cache prevItem = *it;
        if ((prevItem.dest_addr() == host)
                || ((prevItem.dest_addr() == host)
                        && (prevItem.gw_addr() == gateway))) {
            //            prevItem.setTimeCreated(CURRENT_TIME+20);
            return;
        }
    }

    // if not, cache this route ...

    Cache newItem;
    OverlayKey destKey = OverlayKey::sha1(BinaryValue(host.str()));
    newItem.setDest_addr(host);
    newItem.setGw_addr(gateway);
    newItem.setKey(destKey);
    newItem.setSeq(true);
    newItem.setHopCount(hops);
    newItem.setCycle(3);
    newItem.setTimeCreated(time);
    insert_Cache(newItem);
    return;
}

//this maintains entries of reachable gateways and their hop counts
void DHT4Routing::BackBoneGwCache(IPv4Address address, int gwHops) {
    //Cache

    for (gwCacheVector::iterator it = gwItem().begin(); it != gwItem().end();
            it++) {
        gwInfoCache prevItem = *it;
        if (prevItem.gw_addr() == address) {
            if (prevItem.gwHopCount() == gwHops) {
                return;
            } else {
                prevItem.setGwHopCount(gwHops);
            }

        }
    }

    // if not, cache this route ...

    gwInfoCache newGwItem;
    newGwItem.setGw_addr(address);
    newGwItem.setGwHopCount(gwHops);
    insert_gwCache(newGwItem);
    return;
}

//This function creates a table of gateway to host mapping entries for
//routing to external networks through the underlay network.
//at this point, the function only holds destination address-to-overlay
//key mappings. Gateway entries are then added when a GET
//response to this key is received.
void DHT4Routing::TempRoutingTable(IPv4Address addr, OverlayKey destKey) {
    for (tempVector::iterator it = num().begin(); it != num().end(); it++) {
        Temp_table prevNum = *it;
        if (prevNum.dest_addr() == addr) {
            return;
        }
    }

    Temp_table newNum;
    newNum.setDest_addr(addr);
    newNum.setKey(destKey);
    insert_Temp_table(newNum);
    return;
}

/*this function will double check with the dht to make sure the entry to be
 * deleted will not overwrite an existing / already updated entry*/

bool DHT4Routing::dhtChecker(getControl* getCtrl, DHTStatsContext* context){
    //    int count = 0;

    for (auto it = entry().begin(); it != entry().end();) {
        dhtChecker_cache Item = *it;
        if(Item.key() == context->key && Item.count() == 1){
            it = entry().erase(it);
            return true;
        }else {
            ++it;
        }
    }

    dhtChecker_cache newItem;
    newItem.setKey(context->key);
    newItem.setCount(1);
    entry().push_back(newItem);
    GET_timer = new cMessage("DHT_GET_timer");
    getControl *ctrl = new getControl();
    ctrl->key_ = context->key;
    ctrl->value_ = context->value;
    ctrl->gateway_ = getCtrl->gateway_;
    ctrl->host_ = getCtrl->host_;
    GET_timer->setControlInfo(ctrl);
    GET_timer->setKind(DHT_Get_and_Delete);
    scheduleAt(simTime() + (truncnormal(mean, deviation)/15), GET_timer);
    return false;
}
//INSERTS ROUTING TABE ENTRIES RECEIVED FROM THE NETWORK
//LAYER INTO TEMPOPRARY CACHE BEFORE ADDING TO THE DHT.

void DHT4Routing::insert_Cache(Cache newItem) {
    item().push_back(newItem);
}

//INSERTS GATEWAY TO HOST MAPPINGS OF EXTERNAL
//NETWORKS INTO TEMPOPRARY TABLE FOR UNDERLAY ROUTING

void DHT4Routing::insert_Temp_table(Temp_table newItem) {
    num().push_back(newItem);
}

//inserts backbone gateway and hop count mappings

void DHT4Routing::insert_gwCache(gwInfoCache newGwItem) {
    gwItem().push_back(newGwItem);
}


// ----- silas, my input ........
// Erase all the entries in the routing table with interface wlan0
//
void DHT4Routing::verify_Routing_table(IPv4Address addr, cMessage* msg)
{

    IPv4Route *entry;
    inet_rt = RoutingTableAccess().getIfExists();
    bool found;
    double stabilization = par("stabilization");


    for (int i= 0; i<inet_rt->getNumRoutes(); i++)
    {
        found = false;
        entry = inet_rt->getRoute(i);
        if (strcmp(entry->getInterfaceName() , "wlan0")== 0)
        {
            if(entry->getDestination() == addr)
            {
                if(msg->getKind() == addRoute){
                    sendGET(addr, msg);
                    found = true;
                    return;
                }
                if(msg->getKind()  == DHT_Get_and_Delete){
                    found = true;
                    delete msg->removeControlInfo();
                    delete msg;
                    return;
                }
            }
        }
    }

    if(!(found) && (msg->getKind() == DHT_Get_and_Delete)
            && (CURRENT_TIME >= stabilization)){
        for (auto it = item().begin(); it != item().end();) {
            Cache prevItem = *it;
            if (prevItem.dest_addr() == addr) {
                sendGET(addr, msg);
                return;
            }else {
                ++it;
            }
        }
    }
}

void DHT4Routing::handleNodeLeaveNotification() {
    nodeIsLeavingSoon = true;
}

void DHT4Routing::finishApp() {
    simtime_t time = globalStatistics->calcMeasuredLifetime(creationTime);

    if (time >= GlobalStatistics::MIN_MEASURED) {
        globalStatistics->addStdDev("DHT4Routing: Sent Total Messages/s",
                numSent / time);
        globalStatistics->addStdDev("DHT4Routing: Sent GET Messages/s",
                numGetSent / time);
        globalStatistics->addStdDev("DHT4Routing: Failed GET Requests/s",
                numGetError / time);
        globalStatistics->addStdDev("DHT4Routing: Successful GET Requests/s",
                numGetSuccess / time);
        globalStatistics->addStdDev("DHT4Routing: Sent PUT Messages/s",
                numPutSent / time);
        globalStatistics->addStdDev("DHT4Routing: Failed PUT Requests/s",
                numPutError / time);
        globalStatistics->addStdDev("DHT4Routing: Successful PUT Requests/s",
                numPutSuccess / time);
        if ((numGetSuccess + numGetError) > 0) {
            globalStatistics->addStdDev("DHT4Routing: GET Success Ratio",
                    (double) numGetSuccess
                    / (double) (numGetSuccess + numGetError));
        }
    }
}
