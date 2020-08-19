#include <ClientData.h>

ClientData::ClientData(sf::IpAddress address, unsigned short port) :
	mAddress(address),
	mPort(port)
{
}

sf::IpAddress ClientData::getAddress()
{
	return mAddress;
}

unsigned short ClientData::getPort()
{
	return mPort;
}
