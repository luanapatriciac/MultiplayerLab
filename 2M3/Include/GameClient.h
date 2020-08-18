#pragma once
#include <SFML/Network.hpp>

class GameClient
{
public:


	sf::IpAddress getAddress();
	unsigned short getPort();

	void processWaitingPackets(sf::UdpSocket& socket);
	void processReceivedPacket(sf::UdpSocket& socket, sf::Packet& packet, sf::IpAddress& remoteAddress, unsigned short remotePort);

private:
	sf::IpAddress mAddress;
	unsigned short mPort;

};