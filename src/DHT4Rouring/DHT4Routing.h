/*
 * DHT4Routing.h
 *
 *  Created on: Aug 22, 2017
 *      Author: silas
 */


#ifndef __DHT4Routing_H_
#define __DHT4Routing_H_
/// Maximum number of addresses advertised on a message.
#define DHT_MAX_ADDRS       32
// backoff interval
#define BACK_OFF  10
// codes for the different DHT messages
#define DHTPut 1051
#define DHTGet 1052
#define DHT_Get_and_Delete 1053
#define addRoute 1050

#include <omnetpp.h>

#include <GlobalNodeList.h>
#include <GlobalStatistics.h>
#include <UnderlayConfigurator.h>
#include <TransportAddress.h>
#include <OverlayKey.h>
#include <InitStages.h>
#include <BinaryValue.h>
#include <BaseApp.h>
#include <set>
#include <sstream>
#include "IRoutingTable.h"
#include "INotifiable.h"
#include "INETDefs.h"
#include "IPv4Address.h"
#include "IPv4.h"
#include "NotificationBoard.h"
#include "IInterfaceTable.h"
#include "IPv4Datagram.h"
#include "ManetAddress.h"


class GlobalDhtTestMap;
//class GlobalP2PSIPTestMap;

/**
 * A simple test application for the DHT layer
 *
 * A test application that does put and get calls on the DHT layer
 *
 * @author Silas Ngozi
 */


class DHT4Routing: public BaseApp, public cListener
{
public:
    /**
     * A container used by the DHT4Routing to
     * store context information for statistics
     *
     * @Silas Ngozi
     *
     */

    IRoutingTable *inet_rt;


    cMessage *PUT_timer, *GET_timer, *DHT_checker;

    class DHTStatsContext: public cPolymorphic {
    public:
        bool measurementPhase;
        simtime_t requestTime;
        OverlayKey key;
        BinaryValue value;
        cMessage* msg;

        DHTStatsContext(bool measurementPhase, simtime_t requestTime,
                const OverlayKey& key, const BinaryValue& value =
                        BinaryValue::UNSPECIFIED_VALUE, cMessage* msg = NULL/*putControl *putCtrl = NULL, getControl *getCtrl = NULL, IPv4Route *route = NULL*/) :
                            measurementPhase(measurementPhase), requestTime(requestTime), key(
                                    key), value(value), msg(msg) /*,putCtrl(putCtrl), getCtrl(getCtrl), route(route)*/{
        }
        ;
    };

    struct putControl : public cObject
    {
        OverlayKey key_;
        BinaryValue value_;
        IPv4Address gateway_;
        IPv4Address host_;
        int hops_;
    };

    struct getControl : public cObject
    {
        OverlayKey key_;
        BinaryValue value_;
        IPv4Address gateway_;
        IPv4Address host_;
        int hops_;
    };


    void initializeApp(int stage);

    bool isP2PNSNameCountLessThan4TimesNumNodes();

    /**
     * create a GET message for a destination gateway and send it
     */
    void sendGET(IPv4Address addr, cMessage* msg);

    /**
     * create a PUT message of a list of entries and send it
     */
    void sendPUT(IPv4Address &addr, int &hops, cMessage* msg);
    /**
     * generate a random human readable binary value
     */

    /**
     * adds or removes an entry from a list and updates it
     */
    void PUT_updatedList(DHTStatsContext* context, const std::vector<char> &buffer, int &hops, cMessage* msg);


    BinaryValue generateRandomValue();

    void finishApp();

    /**
     * processes get responses
     *
     * method to handle get responses
     * should be overwritten in derived application if needed
     * @param msg get response message
     * @param context context object used for collecting statistics
     */
    virtual void handleGetResponse(DHTgetCAPIResponse* msg,
            DHTStatsContext* context);

    /**
     * processes put responses
     *
     * method to handle put responses
     * should be overwritten in derived application if needed
     * @param msg put response message
     * @param context context object used for collecting statistics
     */
    virtual void handlePutResponse(DHTputCAPIResponse* msg,
            DHTStatsContext* context);

    /**
     * processes self-messages
     *
     * method to handle self-messages
     * should be overwritten in derived application if needed
     * @param msg self-message
     */
    virtual void handleTimerEvent(cMessage* msg);
    virtual void GetScheduler(cMessage *msg );
    virtual void PutScheduler(cMessage *msg );
    virtual void DHT_checkScheduler(cMessage *msg);

    /**
     * handleTraceMessage gets called of handleMessage(cMessage* msg)
     * if a message arrives at trace_in. The command included in this
     * message should be parsed and handled.
     *
     * @param msg the command message to handle
     */
    //   virtual void handleTraceMessage(cMessage* msg);

    virtual void handleNodeLeaveNotification();

    virtual void receiveChangeNotification(int category, const cObject *details);

    /// Erase all entries for wlan0 interfaces in the routing table
    virtual void verify_Routing_table(IPv4Address addr, cMessage* msg);

    //virtual void getContent(IPv4Address destAddr, simtime_t lastUpdate);

    // see RpcListener.h
    void handleRpcResponse(BaseResponseMessage* msg, const RpcState& state,
            simtime_t rtt);

    UnderlayConfigurator* underlayConfigurator; /**< pointer to UnderlayConfigurator in this node */

    GlobalNodeList* globalNodeList; /**< pointer to GlobalNodeList in this node*/

    NotificationBoard *nb;

    GlobalStatistics* globalStatistics; /**< pointer to GlobalStatistics module in this node*/
    GlobalDhtTestMap* globalDhtTestMap; /**< pointer to the GlobalDhtTestMap module */

    virtual void TempCache(IPv4Address host, IPv4Address gateway, int hops, double time);
    virtual void TempRoutingTable(IPv4Address addr, OverlayKey destKey);
    virtual void BackBoneGwCache(IPv4Address address, int gwHops);
    virtual void BestRouteCalculation(DHTStatsContext* context, const DHTEntry *entry);
    virtual bool dhtChecker(getControl* getCtrl, DHTStatsContext* context);
    virtual void choose_back_Off(IPv4Address host, OverlayKey destKey, bool backoff);


    // parameters
    bool debugOutput; /**< debug output yes/no?*/
    double mean; //!< mean time interval between sending test messages
    double deviation; //!< deviation of time interval
    int ttl; /**< ttl for stored DHT records */
    bool p2pnsTraffic; //!< model p2pns application traffic */
    bool activeNetwInitPhase; //!< is app active in network init phase?

    // statistics
    int numSent; /**< number of sent packets*/
    int numGetSent; /**< number of get sent*/
    int numGetError; /**< number of false get responses*/
    int numGetSuccess; /**< number of false get responses*/
    int numPutSent; /**< number of put sent*/
    int numPutError; /**< number of error in put responses*/
    int numPutSuccess; /**< number of success in put responses*/

    bool nodeIsLeavingSoon; //!< true if the node is going to be killed shortly

    static const int DHT4Routing_VALUE_LEN = 20;


    public:
    DHT4Routing();


    typedef struct gwInfoCache: public cObject
    {
        //gateway node
        IPv4Address gw_addr_;
        //number of hops to this gateway
        int gwHopCount_;
        int totalHopCount_;


        inline IPv4Address& gw_addr() {return gw_addr_;}
        inline int& gwHopCount() {return gwHopCount_;}
        inline int& totalHopCount() {return totalHopCount_;}
        void setGw_addr(const IPv4Address &gw_addr) {gw_addr_ = gw_addr;}
        void setGwHopCount(int &gwHopCount) {gwHopCount_ = gwHopCount;}
        void setTotalHopCount(int &totalHopCount) {totalHopCount_ = totalHopCount;}

        gwInfoCache(){}
        gwInfoCache(gwInfoCache * e) {
            gw_addr_ = e->gw_addr_;
            //gwHopCount_ = e-gwHopCount_;
        }

        bool operator==(const gwInfoCache& other) const {return this->gw_addr_ == other.gw_addr_
                && this->gwHopCount_ == other.gwHopCount_ && this->totalHopCount_ == other.totalHopCount_;
        }

    }gwInfoCache;

    typedef std::vector<gwInfoCache> gwCacheVector;
    gwCacheVector gwItem_;
    inline gwCacheVector& gwItem() {return gwItem_;}

    virtual void insert_gwCache(gwInfoCache);


//cache all entries to keep track of what is or is not in the DHT per time.
    typedef struct Cache: public cObject
    {
        //destination node
        IPv4Address dest_addr_;
        //gateway of this Originator
        IPv4Address gw_addr_;
        //Overlay key of originator of this packet
        OverlayKey key_;
        int hopCount_;
        int cycle_;
        double timeCreated_;
        bool seq_ = false;
        bool inDHT_ = false;

        inline IPv4Address& dest_addr() {return dest_addr_;}
        inline IPv4Address& gw_addr() {return gw_addr_;}
        inline OverlayKey& key() {return key_;}
        inline bool& seq() {return seq_;}
        inline bool& inDHT() {return inDHT_;}
        inline int& hopCount() {return hopCount_;}
        inline int& cycle() {return cycle_;}
        inline double timeCreated() {return timeCreated_;}
        void setDest_addr(const IPv4Address &dest_addr) {dest_addr_ = dest_addr;}
        void setGw_addr(const IPv4Address &gw_addr) {gw_addr_ = gw_addr;}
        void setKey(const OverlayKey &key)  {key_ = key;}
        void setSeq(const bool &seq)     {seq_ = seq;}
        void setInDHT(const bool &inDHT)     {inDHT_ = inDHT;}
        void setHopCount(int &hopCount) {hopCount_ = hopCount;}
        void setCycle(int cycle) {cycle_ = cycle;}
        void setTimeCreated(double timeCreated) {timeCreated_ = timeCreated;}

        Cache(){}
        Cache(Cache * e) {
            dest_addr_ = e->dest_addr_;
            gw_addr_ = e->gw_addr_;
            key_ = e->key_;
            seq_ = e->seq_;
            timeCreated_ = e->timeCreated_;
        }

        bool operator==(const Cache& other) const {return this->dest_addr_ == other.dest_addr_
                && this->gw_addr_ == other.gw_addr_ && this->key_ == other.key_ && this->seq_ == other.seq_
                && this->hopCount_ == other.hopCount_ && this->inDHT_ == other.inDHT_ && this->cycle_ == other.cycle_;
        }

    }Cache;

    typedef std::vector<Cache> cacheVector; ///<DHT entry Set type.
    cacheVector item_;
    inline cacheVector& item() {return item_;}

    virtual void insert_Cache(Cache);




    struct Entry {
        IPv4Address ip;
        uint32_t hops;

        Entry(IPv4Address ip, uint32_t hops)
        : ip(ip)
        , hops(hops)
        {}

        void serialize(std::vector<char>::iterator& it) {
            *it++ = (char) ((ip.getInt() >> 24) & 0xFF);
            *it++ = (char) ((ip.getInt() >> 16) & 0xFF);
            *it++ = (char) ((ip.getInt() >> 8) & 0xFF);
            *it++ = (char) ((ip.getInt()) & 0xFF);
            *it++ = (char) ((hops >> 24) & 0xFF);
            *it++ = (char) ((hops >> 16) & 0xFF);
            *it++ = (char) ((hops >> 8) & 0xFF);
            *it++ = (char) ((hops) & 0xFF);
        }

        static Entry deserialize(std::vector<char>::iterator& it) {
            uint32_t ip = 0;
            ip |= *it++ << 24;
            ip |= *it++ << 16;
            ip |= *it++ << 8;
            ip |= *it++;
            uint32_t hops = 0;
            hops |= *it++ << 24;
            hops |= *it++ << 16;
            hops |= *it++ << 8;
            hops |= *it++;
            return Entry(IPv4Address(ip), hops);
        }
    };

    std::vector<char> serialize(std::vector<Entry> entries) {
        std::vector<char> bin(entries.size() * 8);
        auto it = std::begin(bin);
        for (auto entry : entries) {
            entry.serialize(it);
        }
        return bin;
    }

    std::vector<Entry> deserialize(std::vector<char> bin) {
        std::vector<Entry> entries;
        for (auto it = bin.begin(); it != bin.end();) {
            entries.push_back(Entry::deserialize(it));
        }
        return entries;
    }


    typedef struct Temp_table: public cObject
    {
        //originator of this packet
        IPv4Address dest_addr_;
        //gateway to destination node
        IPv4Address destGw_addr_;
        //source node overlay key
        OverlayKey key_;
        uint32_t hopCount_;
        bool isSuccess_ = false;

        inline IPv4Address& dest_addr() {return dest_addr_;}
        inline IPv4Address& destGw_addr() {return destGw_addr_;}
        inline OverlayKey& key() {return key_;}
        inline uint32_t& hopCount() {return hopCount_;}
        inline bool& isSuccess() {return isSuccess_;}
        void setDest_addr(const IPv4Address &dest_addr) {dest_addr_ = dest_addr;}
        void setDestGw_addr(const IPv4Address &destGw_addr) {destGw_addr_ = destGw_addr;}
        void setKey(const OverlayKey &key) {key_ = key;}
        void setHopCount(uint32_t &hopCount) {hopCount_ = hopCount;}
        void setIsSuccess(const bool &isSuccess) {isSuccess_ = isSuccess;}

        Temp_table(){}
        Temp_table(Temp_table * e) {
            dest_addr_ = e->dest_addr_;
            destGw_addr_ = e->destGw_addr_;
            key_ = e->key_;
            hopCount_ = e->hopCount_;
            isSuccess_ = e->isSuccess_;
        }

        bool operator==(const Temp_table& other) const {return this->dest_addr_ == other.dest_addr_
                && this->destGw_addr_ == other.destGw_addr_ && this->key_ == other.key_
                && this->hopCount_ == other.hopCount_ && this->isSuccess_ == other.isSuccess_;
        }

    }Temp_table;


    typedef std::vector<Temp_table> tempVector; ///temporary routing table
    tempVector num_;
    inline tempVector& num() {return num_;}

    void insert_Temp_table(Temp_table);


    typedef struct dhtChecker_cache: public cObject
        {
            OverlayKey key_;
            int count_;

            inline OverlayKey& key() {return key_;}
            inline int& count() {return count_;}
            void setKey(const OverlayKey &key) {key_ = key;}
            void setCount(int count) {count_ = count;}

            dhtChecker_cache(){}
            dhtChecker_cache(dhtChecker_cache * e) {
                key_ = e->key_;
                count_ = e->count_;
            }

            bool operator==(const dhtChecker_cache& other) const {return this->key_ == other.key_
                    && this->count_ == other.count_;
            }

        }dhtChecker_cache;


        typedef std::vector<dhtChecker_cache> dhtCheckerVector; ///temporary routing table
        dhtCheckerVector entry_;
        inline dhtCheckerVector& entry() {return entry_;}


/*This structure stores entries that chose a random back-off-interval
 * before re-issuing a DHT querry for which they received a NULL
 */
        typedef struct back_off_cache: public cObject
        {
            IPv4Address host_;
            OverlayKey key_;
            double backoff_;
            int sequence_;

            inline IPv4Address& host() {return host_;}
            inline OverlayKey& key() {return key_;}
            inline double& backoff() {return backoff_;}
            inline int& sequence() {return sequence_;}
            void setHost(const IPv4Address &host) {host_ = host;}
            void setKey(const OverlayKey &key) {key_ = key;}
            void setBackoff(const double backoff) {backoff_ = backoff;}
            void setSequence(int sequence) {sequence_ = sequence;}

            back_off_cache(){}
            back_off_cache(back_off_cache * e) {
                host_ = e->host_;
                key_ = e->key_;
                backoff_ = e->backoff_;
                sequence_ = e->sequence_;
            }

            bool operator==(const back_off_cache& other) const {return this->host_ == other.host_ &&
                    this->key_ == other.key_ && this->backoff_ == other.backoff_ &&
                    this->sequence_ == other.sequence_;
            }

        }back_off_cache;


        typedef std::vector<back_off_cache> backOff; ///temporary routing table
        backOff value_;
        inline backOff& value() {return value_;}


    DHT4Routing(DHT4Routing *);

    /**
     * virtual destructor
     */
    virtual ~DHT4Routing();


};

#endif /* DHT4ROUTING_H_ */
