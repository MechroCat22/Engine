/************************************************************************/
/* File: NetAddress.hpp
/* Author: Andrew Chase
/* Date: August 23rd, 2018
/* Description: Class to represent a single address and service to connect to
/************************************************************************/
#pragma once
#include <string>

struct sockaddr;

struct NetAddress_t
{
public:
	//-----Public Methods-----

	NetAddress_t();
	NetAddress_t(const sockaddr* addr);
	NetAddress_t(const char* string, bool bindable = false);

	bool operator==(const NetAddress_t& compare) const;

	bool					ToSockAddr(sockaddr* out_addr, size_t* out_addrLen) const;
	bool					FromSockAddr(const sockaddr* addr);

	std::string				ToString() const;

	static bool				GetLocalAddress(NetAddress_t* out_addr, unsigned short port, bool bindable);

public:
	//-----Public Data-----

	unsigned int ipv4Address = 0;
	unsigned short port = 0;

};
