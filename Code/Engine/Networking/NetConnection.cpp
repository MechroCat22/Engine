/************************************************************************/
/* File: NetConnection.cpp
/* Author: Andrew Chase
/* Date: September 20th, 2018
/* Description: Implementation of the NetConnection class
/************************************************************************/
#include "Engine/Core/LogSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Time/Stopwatch.hpp"
#include "Engine/Networking/UDPSocket.hpp"
#include "Engine/Networking/NetPacket.hpp"
#include "Engine/Networking/NetMessage.hpp"
#include "Engine/Networking/NetSession.hpp"
#include "Engine/Networking/NetConnection.hpp"


//-----------------------------------------------------------------------------------------------
// Constructor
//
NetConnection::NetConnection(NetAddress_t& address, NetSession* session, uint8_t connectionIndex)
	: m_address(address)
	, m_owningSession(session)
	, m_indexInSession(connectionIndex)
{
	m_sendTimer = new Stopwatch();
	m_heartbeatTimer = new Stopwatch();
	m_heartbeatTimer->SetInterval(m_timeBetweenHeartbeats);
}


//-----------------------------------------------------------------------------------------------
// Destructor
//
NetConnection::~NetConnection()
{
	// Check for any pending messages that didn't get sent
	for (int i = 0; i < (int)m_outboundUnreliables.size(); ++i)
	{
		delete m_outboundUnreliables[i];
	}

	m_outboundUnreliables.clear();

	if (m_sendTimer != nullptr)
	{
		delete m_sendTimer;
		m_sendTimer = nullptr;
	}

	if (m_heartbeatTimer != nullptr)
	{
		delete m_heartbeatTimer;
		m_heartbeatTimer = nullptr;
	}
}


//-----------------------------------------------------------------------------------------------
// Queues the message to be sent during a flush
//
void NetConnection::Send(NetMessage* msg)
{
	m_outboundUnreliables.push_back(msg);
}


//-----------------------------------------------------------------------------------------------
// Sends all pending messages out of the socket
//
void NetConnection::FlushMessages()
{
	if (m_outboundUnreliables.size() == 0)
	{
		return;
	}

	// Package them all into one NetPacket
	NetPacket* packet = new NetPacket();
	packet->AdvanceWriteHead(2); // Advance the write head now, and write the header later
	packet->SetSenderConnectionIndex(m_owningSession->GetLocalConnectionIndex());
	packet->SetReceiverConnectionIndex(m_indexInSession);

	uint8_t messagesWritten = 0;

	for (int msgIndex = 0; msgIndex < m_outboundUnreliables.size(); ++msgIndex)
	{
		NetMessage* msg = m_outboundUnreliables[msgIndex];

		// Check if the message will fit
		// 2 bytes for the header and message size, 1 byte for message definition index,
		// and then the payload
		uint16_t totalSize = (uint16_t) (2 + 1 + msg->GetWrittenByteCount());
		if (packet->GetRemainingWritableByteCount() >= totalSize)
		{
			packet->WriteMessage(msg);
			messagesWritten++;
		}
		else
		{
			PacketHeader_t header(m_owningSession->GetLocalConnectionIndex(), messagesWritten);
			packet->WriteHeader(header);

			// Send the current packet and start again
			bool sent = m_owningSession->SendPacket(packet);

			if (sent)
			{
				LogTaggedPrintf("NET", "NetConnection sent packet with %i messages", messagesWritten);
			}
			else
			{
				LogTaggedPrintf("NET", "NetConnection couldn't send packet for %i messages", messagesWritten);
			}

			// Reset
			packet->ResetWrite();
			packet->AdvanceWriteHead(2);

			packet->WriteMessage(msg);
			messagesWritten = 1;
		}

		delete msg;
	}
	
	PacketHeader_t header(m_owningSession->GetLocalConnectionIndex(), messagesWritten);
	packet->WriteHeader(header);

	bool sent = m_owningSession->SendPacket(packet);

	if (sent)
	{
		LogTaggedPrintf("NET", "NetConnection sent packet with %i messages", messagesWritten);
	}
	else
	{
		LogTaggedPrintf("NET", "NetConnection couldn't send packet for %i messages", messagesWritten);
	}
		

	m_outboundUnreliables.clear();

	// Reset the send timer
	m_sendTimer->Reset();
}


//-----------------------------------------------------------------------------------------------
// Returns the target address for this connection
//
NetAddress_t NetConnection::GetAddress()
{
	return m_address;
}


//-----------------------------------------------------------------------------------------------
// Sets the net tick rate for the connection to correspond to the provided refresh rate
//
void NetConnection::SetNetTickRate(float hertz)
{
	m_timeBetweenSends = (1.f / hertz);
}


//-----------------------------------------------------------------------------------------------
// Returns whether the connection should send based on the tick rate of the connection and the owning
// session
//
bool NetConnection::IsReadyToFlush() const
{
	float sessionTime = m_owningSession->GetTimeBetweenSends();
	float sendInterval = MaxFloat(sessionTime, m_timeBetweenSends);

	return (m_sendTimer->GetElapsedTime() > sendInterval);
}


//-----------------------------------------------------------------------------------------------
// Sets the heartbeat of the connection to correspond to the given frequency
//
void NetConnection::SetHeartbeat(float hertz)
{
	m_timeBetweenHeartbeats = (1.0f / hertz);
	m_heartbeatTimer->SetInterval(m_timeBetweenHeartbeats);
}


//-----------------------------------------------------------------------------------------------
// Returns true if the connection should send a heartbeat
//
bool NetConnection::HasHeartbeatElapsed() const
{
	bool elapsed = m_heartbeatTimer->HasIntervalElapsed();

	if (elapsed)
	{
		m_heartbeatTimer->SetInterval(m_timeBetweenHeartbeats);
	}

	return elapsed;
}
