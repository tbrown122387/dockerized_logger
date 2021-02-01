/* Copyright (C) 2019 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include "StdAfx.h"

#include "EminiLogger.h"

#include "EClientSocket.h"
#include "EPosixClientSocketPlatform.h"

#include "Contract.h"
#include "Order.h"
#include "OrderState.h"
#include "Execution.h"
#include "CommissionReport.h"
#include "ContractSamples.h"
#include "OrderSamples.h"
#include "ScannerSubscription.h"
#include "ScannerSubscriptionSamples.h"
#include "executioncondition.h"
#include "PriceCondition.h"
#include "MarginCondition.h"
#include "PercentChangeCondition.h"
#include "TimeCondition.h"
#include "VolumeCondition.h"
#include "AvailableAlgoParams.h"
#include "FAMethodSamples.h"
#include "CommonDefs.h"
#include "AccountSummaryTags.h"
#include "Utils.h"

#include <stdio.h>
#include <chrono>
#include <iostream>
#include <thread>
#include <ctime>
#include <fstream>
#include <cstdint>



EminiLogger::EminiLogger() :
      m_osSignal(2000)//2-seconds timeout
    , m_pClient(new EClientSocket(this, &m_osSignal))
	, m_state(ST_CONNECT)
    , m_pReader(0)
    , m_extraAuth(false)
    , m_printing(true)
    , m_tick_writer("/usr/src/app/IBJts/samples/Cpp/TestCppClient/mysql_config.txt", 
                    "/usr/src/app/IBJts/samples/Cpp/TestCppClient/tickers.txt",
                    10, // auto flush every ten ticks
                    true, // printing
                    true) // reconnect to db    
{
}


EminiLogger::~EminiLogger()
{
    if (m_pReader)
        delete m_pReader;
    delete m_pClient;
}


bool EminiLogger::connect(const char *host, int port, int clientId)
{
	printf( "Connecting to %s:%d clientId:%d\n", !( host && *host) ? "127.0.0.1" : host, port, clientId);
	bool bRes = m_pClient->eConnect( host, port, clientId, m_extraAuth);
	
	if (bRes) {
		printf( "Connected to %s:%d clientId:%d\n", m_pClient->host().c_str(), m_pClient->port(), clientId);
        	m_pReader = new EReader(m_pClient, &m_osSignal);
		m_pReader->start();
	}
	else
		printf( "Cannot connect to %s:%d clientId:%d\n", m_pClient->host().c_str(), m_pClient->port(), clientId);

    return bRes;
}

void EminiLogger::disconnect() const
{
	m_pClient->eDisconnect();

	printf ( "Disconnected\n");
}

bool EminiLogger::isConnected() const
{
	return m_pClient->isConnected();
}

void EminiLogger::setConnectOptions(const std::string& connectOptions)
{
	m_pClient->setConnectOptions(connectOptions);
}


void EminiLogger::processMessages()
{
	// connection sequence, then oscillate back and forth
	// between checking actual/desired position and checking pnl
    // here's a chart:
	
    // ST_REQTRADEDATA -> ST_REQORDERDATA -> (ST_IDLE) -> ST_UNSUBSCRIBE 

	switch (m_state) {

		case ST_REQTRADEDATA:
			reqAllTradeData(); // changes m_state to ST_REQPNL
			break;
        	case ST_REQORDERDATA:
            		// need to wait more than 15 seconds to request two things
            		// from the same symbol
			std::this_thread::sleep_for(std::chrono::seconds(16));
            		reqAllOrderData(); // changes m_state to ST_IDLE
			break;
        	case ST_IDLE:
            		doNothing(); // changes m_state to ST_UNSUBSCRIBE only in rare circumstance
		        break;
        	case ST_UNSUBSCRIBE:
            		unsubscribeAll();   // does not change m_state
            		break; // 
	}

	m_osSignal.waitForSignal();
	errno = 0;
	m_pReader->processMsgs();
}


void EminiLogger::connectAck() {
	if (!m_extraAuth && m_pClient->asyncEConnect())
        m_pClient->startApi();
}



void EminiLogger::unsubscribeAll(){
    for(unsigned int i = 0; i < m_tick_writer.size(); ++i){
        std::string ticker = m_tick_writer.loc_syms(i); 
        m_pClient->cancelMktData(m_tick_writer.unique_order_id(ticker));
        m_pClient->cancelMktData(m_tick_writer.unique_trade_id(ticker));
    }
}


void EminiLogger::reqAllTradeData()
{
    for(unsigned int i = 0; i < m_tick_writer.size(); ++i){
        Contract contract;
        contract.symbol  = m_tick_writer.syms(i);
        contract.secType = m_tick_writer.sec_types(i);
        contract.currency = m_tick_writer.currencies(i);
        contract.exchange = m_tick_writer.exchs(i);
        // TODO make the next few lines not futures specific
        contract.localSymbol = m_tick_writer.loc_syms(i);

        if(m_printing) std::cout << "requesting bid/ask data for " << contract.symbol << "\n";

        m_pClient->reqTickByTickData(m_tick_writer.unique_order_id(contract.localSymbol), 
                                     contract, 
                                     "Last", 
                                     0, // nonzero means historical data too
                                     true); // ignore size only changes?
    }
    
    m_state = ST_REQORDERDATA;

}


void EminiLogger::reqAllOrderData()
{

    for(unsigned int i = 0; i < m_tick_writer.size(); ++i){
        Contract contract;
        contract.symbol  = m_tick_writer.syms(i);
        contract.secType = m_tick_writer.sec_types(i);
        contract.currency = m_tick_writer.currencies(i);
        contract.exchange = m_tick_writer.exchs(i);
        // TODO make the next few lines not futures specific
        contract.localSymbol = m_tick_writer.loc_syms(i);

        if(m_printing) std::cout << "requesting bid/ask data for " << contract.symbol << "\n";

        m_pClient->reqTickByTickData(m_tick_writer.unique_order_id(contract.localSymbol), 
                                     contract, 
                                     "BidAsk", 
                                     0, // nonzero means historical data too
                                     true); // ignore size only changes?
    }
    
    m_state = ST_IDLE;

}


void EminiLogger::nextValidId( OrderId orderId)
{
    // the starting state after connection is achieved
    m_state = ST_REQTRADEDATA; 
}


void EminiLogger::error(int id, int errorCode, const std::string& errorString)
{
	printf( "Error. Id: %d, Code: %d, Msg: %s\n", id, errorCode, errorString.c_str());
}



void EminiLogger::connectionClosed() {
	printf( "Connection Closed\n");
}



void EminiLogger::tickByTickAllLast(int reqId, int tickType, time_t time, double price, int size, const TickAttribLast& tickAttribLast, const std::string& exchange, const std::string& specialConditions) {
    if(m_printing){
        printf("Tick-By-Tick. ReqId: %d, TickType: %s, Time: %s, Price: %g, Size: %d, PastLimit: %d, Unreported: %d, Exchange: %s, SpecialConditions:%s\n", 
            reqId, (tickType == 1 ? "Last" : "AllLast"), ctime(&time), price, size, tickAttribLast.pastLimit, tickAttribLast.unreported, exchange.c_str(), specialConditions.c_str());
        std::cout << "trade for ticker: " << m_tick_writer.loc_sym_from_uid(reqId) << "\n";
    }

    m_tick_writer.addTrade(hft::ClockType::now(), price, size, exchange, m_tick_writer.loc_sym_from_uid(reqId)); 
}


void EminiLogger::tickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, int bidSize, int askSize, const TickAttribBidAsk& tickAttribBidAsk) {

    //  printed stuff gets redirected to another logfile
    if(m_printing){
        std::cout << "\n\n";	
        printf("Tick-By-Tick. ReqId: %d, TickType: BidAsk, Time: %s, BidPrice: %g, AskPrice: %g, BidSize: %d, AskSize: %d, BidPastLow: %d, AskPastHigh: %d\n", 
            reqId, ctime(&time), bidPrice, askPrice, bidSize, askSize, tickAttribBidAsk.bidPastLow, tickAttribBidAsk.askPastHigh);
        std::cout << "quote for ticker: " << m_tick_writer.loc_sym_from_uid(reqId) << "\n";
    }

    // store price info
    m_tick_writer.addBidAsk(hft::ClockType::now(), bidPrice, askPrice, bidSize, askSize, m_tick_writer.loc_sym_from_uid(reqId));
}


void EminiLogger::doNothing()
{
    // doesn't have to do anything...the callbacks above will write all the data 
    if(false) m_state = ST_UNSUBSCRIBE;
}


// virtual functions that need to be defined but aren't used at all
void EminiLogger::marketRule(int marketRuleId, const std::vector<PriceIncrement> &priceIncrements) {}
void EminiLogger::orderBound(long long orderId, int apiClientId, int apiOrderId) {}
void EminiLogger::tickString(TickerId tickerId, TickType tickType, const std::string& value) {}
void EminiLogger::currentTime( long time) {}
void EminiLogger::familyCodes(const std::vector<FamilyCode> &familyCodes) {}
void EminiLogger::newsArticle(int requestId, int articleType, const std::string& articleText) {}
void EminiLogger::realtimeBar(TickerId reqId, long time, double open, double high, double low, 
double close, long volume, double wap, int count) {}
void EminiLogger::tickGeneric(TickerId tickerId, TickType tickType, double value) {}
void EminiLogger::scannerData(int reqId, int rank, const ContractDetails& contractDetails, const std::string& distance, const std::string& benchmark, const std::string& projection, const std::string& legsStr) {}
void EminiLogger::scannerDataEnd(int reqId) {}
void EminiLogger::histogramData(int reqId, const HistogramDataVector& data) {}
void EminiLogger::newsProviders(const std::vector<NewsProvider> &newsProviders) {}
void EminiLogger::symbolSamples(int reqId, const std::vector<ContractDescription> &contractDescriptions) {}
void EminiLogger::tickReqParams(int tickerId, double minTick, const std::string& bboExchange, int snapshotPermissions) {}
void EminiLogger::accountSummary( int reqId, const std::string& account, const std::string& tag, const std::string& value, const std::string& currency) {}
void EminiLogger::historicalData(TickerId reqId, const Bar& bar) {}
void EminiLogger::historicalNews(int requestId, const std::string& time, const std::string& providerCode, const std::string& articleId, const std::string& headline) {}
void EminiLogger::headTimestamp(int reqId, const std::string& headTimestamp) {}
void EminiLogger::marketDataType(TickerId reqId, int marketDataType) {}
void EminiLogger::updateMktDepth(TickerId id, int position, int operation, int side, double price, int size) {}
void EminiLogger::contractDetails( int reqId, const ContractDetails& contractDetails) {}
void EminiLogger::fundamentalData(TickerId reqId, const std::string& data) {}
void EminiLogger::historicalTicks(int reqId, const std::vector<HistoricalTick>& ticks, bool done) {}
void EminiLogger::managedAccounts( const std::string& accountsList) {}
void EminiLogger::smartComponents(int reqId, const SmartComponentsMap& theMap) {}
void EminiLogger::softDollarTiers(int reqId, const std::vector<SoftDollarTier> &tiers) {}
void EminiLogger::tickSnapshotEnd(int reqId) {}
void EminiLogger::verifyCompleted( bool isSuccessful, const std::string& errorText) {}
void EminiLogger::displayGroupList( int reqId, const std::string& groups) {}
void EminiLogger::verifyMessageAPI( const std::string& apiData) {}
void EminiLogger::accountSummaryEnd( int reqId) {}
void EminiLogger::historicalDataEnd(int reqId, const std::string& startDateStr, const std::string& endDateStr) {}
void EminiLogger::historicalNewsEnd(int requestId, bool hasMore) {}
void EminiLogger::mktDepthExchanges(const std::vector<DepthMktDataDescription> &depthMktDataDescriptions) {}
void EminiLogger::updateMktDepthL2(TickerId id, int position, const std::string& marketMaker, int operation,
                                     int side, double price, int size, bool isSmartDepth) {}
void EminiLogger::rerouteMktDataReq(int reqId, int conid, const std::string& exchange) {}
void EminiLogger::scannerParameters(const std::string& xml) {}
void EminiLogger::updateAccountTime(const std::string& timeStamp) {}
void EminiLogger::accountDownloadEnd(const std::string& accountName) {}
void EminiLogger::accountUpdateMulti( int reqId, const std::string& account, const std::string& modelCode, const std::string& key, const std::string& value, const std::string& currency) {}
void EminiLogger::contractDetailsEnd( int reqId) {}
void EminiLogger::rerouteMktDepthReq(int reqId, int conid, const std::string& exchange) {}
void EminiLogger::tickByTickMidPoint(int reqId, time_t time, double midPoint) {}
void EminiLogger::updateNewsBulletin(int msgId, int msgType, const std::string& newsMessage, const std::string& originExch) {}
void EminiLogger::bondContractDetails( int reqId, const ContractDetails& contractDetails) {}
void EminiLogger::displayGroupUpdated( int reqId, const std::string& contractInfo) {}
void EminiLogger::historicalTicksLast(int reqId, const std::vector<HistoricalTickLast>& ticks, bool done) {}
void EminiLogger::accountUpdateMultiEnd( int reqId) {}
void EminiLogger::historicalTicksBidAsk(int reqId, const std::vector<HistoricalTickBidAsk>& ticks, bool done) {}
void EminiLogger::tickOptionComputation( TickerId tickerId, TickType tickType, double impliedVol, double delta,
                                          double optPrice, double pvDividend,
                                          double gamma, double vega, double theta, double undPrice) {}
void EminiLogger::deltaNeutralValidation(int reqId, const DeltaNeutralContract& deltaNeutralContract) {}
void EminiLogger::verifyAndAuthCompleted( bool isSuccessful, const std::string& errorText) {}
void EminiLogger::verifyAndAuthMessageAPI( const std::string& apiDatai, const std::string& xyzChallenge) {}
void EminiLogger::securityDefinitionOptionalParameter(int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass,
                                                        const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes) {}
void EminiLogger::securityDefinitionOptionalParameterEnd(int reqId) {}
void EminiLogger::tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const std::string& formattedBasisPoints,
                            double totalDividends, int holdDays, const std::string& futureLastTradeDate, double dividendImpact, double dividendsToLastTradeDate) {}
void EminiLogger::tickNews(int tickerId, time_t timeStamp, const std::string& providerCode, const std::string& articleId, const std::string& headline, const std::string& extraData) {}
void EminiLogger::tickSize( TickerId tickerId, TickType field, int size) {}
void EminiLogger::receiveFA(faDataType pFaDataType, const std::string& cxml) {}
void EminiLogger::tickPrice( TickerId tickerId, TickType field, double price, const TickAttrib& attribs) {}
void EminiLogger::historicalDataUpdate(TickerId reqId, const Bar& bar) {}
void EminiLogger::pnlSingle(int reqId, int pos, double dailyPnL, double unrealizedPnL, double realizedPnL, double value) {}
void EminiLogger::winError( const std::string& str, int lastError) {}
void EminiLogger::completedOrdersEnd() {}
void EminiLogger::positionMulti( int reqId, const std::string& account,const std::string& modelCode, const Contract& contract, double pos, double avgCost){} 
void EminiLogger::positionMultiEnd( int reqId) {}
void EminiLogger::orderStatus(OrderId orderId, const std::string& status, double filled,
		double remaining, double avgFillPrice, int permId, int parentId,
		double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice){}
void EminiLogger::openOrder( OrderId orderId, const Contract& contract, const Order& order, const OrderState& orderState) {}
void EminiLogger::openOrderEnd() {}
void EminiLogger::updateAccountValue(const std::string& key, const std::string& val,
                                       const std::string& currency, const std::string& accountName) {}
void EminiLogger::updatePortfolio(const Contract& contract, double position,
                                    double marketPrice, double marketValue, double averageCost,
                                    double unrealizedPNL, double realizedPNL, const std::string& accountName){}
void EminiLogger::execDetails( int reqId, const Contract& contract, const Execution& execution) {}
void EminiLogger::execDetailsEnd( int reqId) {}
void EminiLogger::commissionReport( const CommissionReport& commissionReport) {}
void EminiLogger::position( const std::string& account, const Contract& contract, double position, double avgCost){}
void EminiLogger::positionEnd() {}
void EminiLogger::pnl(int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL) {}
void EminiLogger::completedOrder(const Contract& contract, const Order& order, const OrderState& orderState) {}




























