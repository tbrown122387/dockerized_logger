/* Copyright (C) 2019 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once
#ifndef TWS_API_SAMPLES_TESTCPPCLIENT_TESTCPPCLIENT_H
#define TWS_API_SAMPLES_TESTCPPCLIENT_TESTCPPCLIENT_H

#include "EWrapper.h"
#include "EReaderOSSignal.h"
#include "EReader.h"

#include <memory>
#include <vector>

// my stuff
#include "config.h"
#include "tick_writer.h"

class EClientSocket;

enum State {
    ST_CONNECT,
    ST_REQ_DATA,
    ST_IDLE,
    ST_CLOSEOUT,
    ST_UNSUBSCRIBE
};

class EminiLogger : public EWrapper
{
private:

public:

	EminiLogger();
	~EminiLogger();

	void setConnectOptions(const std::string&);
	void processMessages();

public:

	bool connect(const char * host, int port, int clientId = 0);
	void disconnect() const;
	bool isConnected() const;

private:
    void reqAllData();
    void doNothing();
    void unsubscribeAll();
public:
	// events
	#include "EWrapper_prototypes.h"

private:
	EReaderOSSignal m_osSignal;
	EClientSocket * const m_pClient;
	State m_state;

	EReader *m_pReader;
    bool m_extraAuth;
	std::string m_bboExchange;

    // new stuff! 
    const bool m_printing;
    hft::TickWriter m_tick_writer;

};

#endif

