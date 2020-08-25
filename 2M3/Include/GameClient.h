#pragma once
#include <SFML/Network.hpp>
#include <Player.h>
#include <World.h>
#include <queue>

class GameClient
{
public:

	GameClient();

	sf::IpAddress getAddress();
	unsigned short getPort();

	void setServerAddress(sf::IpAddress serverAddress);
	void setServerPort(unsigned short port);

	sf::Socket::Status bindSocket();

	void sendPacket(sf::Packet packet, sf::IpAddress remoteAddress, unsigned short remotePort);	

	void processWaitingPackets(World& world);
	void processReceivedPacket(sf::Packet& packet, sf::IpAddress& remoteAddress, unsigned short remotePort, World& world);

	void sendCarsInputs(const std::vector<Player*>& players);

private:
	sf::Uint32 mID;
	sf::IpAddress mAddress;
	unsigned short mPort;
	sf::UdpSocket mSocket;

	sf::IpAddress mServerAddress;
	unsigned short mServerPort;

	sf::Clock mClock;
	sf::Time mClockOffset;	//server.clock - this.clock - delay

	std::queue<Entity*> mToBeAssignedID;

};