void DHT4Routing::BestRouteCalculation(DHTStatsContext* context, const DHTEntry *entry){

    IPv4Datagram data;
    bool found = false;
    int bestHop = 0;
    IPv4Address bestGateWay;
    IPv4Address destinationNode; 

    std::vector<char> buffer(entry->value.begin(), entry->value.end());

    std::vector<Entry> entries = deserialize(buffer);
    IPv4Address GateWay; uint32_t hopCount;

    for (auto entry : entries) {
        GateWay = entry.ip;
        hopCount = entry.hops;
        cout << "key = " << context->key << " ; " << "value = "
                << GateWay << "; hop count : "<< hopCount <<" [t= "<<simTime() << "]"<<endl;

        for (tempVector::iterator it = num().begin(); it != num().end();
                it++) {
            Temp_table prevNum = *it;

            if (prevNum.key() == context->key) {
                prevNum.setDestGw_addr(GateWay, count);
                prevNum.setHopCount(hopCount, count);
                destinationNode = prevNum.dest_addr();
                cout << prevNum.dest_addr() << " ; "
                        << prevNum.destGw_addr(count) << " ; " << endl;
                found = true;
            }
        }

        if(found){

            for(gwCacheVector::iterator it = gwItem().begin(); it != gwItem().end();
                    it++) {
                gwInfoCache prevNum = *it;


                if(prevNum.gw_addr() == GateWay)
                {
                    prevNum.setTotalHopCount(prevNum.gwHopCount() + hopCount);
                }

                if(bestHop == 0)
                {
                    bestHop = prevNum.totalHopCount();
                    bestGateWay = prevNum.gw_addr();
                }else{
                    if(prevNum.totalHopCount() <= bestHop)
                    {
                        bestHop = prevNum.totalHopCount();
                        bestGateWay = prevNum.gw_addr();
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






\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
            IPv4Datagram data;
            int count = 0; bool check = false; uint32_t trueCount;
            IPv4Address ra_; uint32_t hopCount;

            std::vector<char> buffer(entry->value.begin(), entry->value.end());

            std::vector<Entry> entries = deserialize(buffer);

            for (auto entry : entries) {
               ra_ = entry.ip;
               hopCount = entry.hops;
               cout << "key = " << context->key << " ; " << "value = "
                       << ra_ << "; hop count : "<< hopCount <<" [t= "<<simTime() << "]"<<endl;

            for (tempVector::iterator it = num().begin(); it != num().end();
                    it++) {
                Temp_table prevNum = *it;

                if (prevNum.key() == context->key) {
                    prevNum.setDestGw_addr(ra_, count);
                    prevNum.setHopCount(hopCount, count);
                    cout << prevNum.dest_addr() << " ; "
                            << prevNum.destGw_addr(count) << " ; " << endl;

                }
                if(check)
                {
                    if(prevNum.hopCount(count) <= trueCount)
                    {
                        data.setDestAddress(prevNum.dest_addr());
                        data.setSrcAddress(prevNum.destGw_addr(count));
                    }
                    else
                    {
                        data.setDestAddress(prevNum.dest_addr());
                        data.setSrcAddress(prevNum.destGw_addr(trueCount));
                    }
                }
                else
                {
                    data.setDestAddress(prevNum.dest_addr());
                    data.setSrcAddress(prevNum.destGw_addr(count));
                    trueCount = prevNum.hopCount(count);
                }
            }
            count++; check = true;
          }