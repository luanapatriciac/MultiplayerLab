//
// Created by jglrxavpok on 15/10/2020.
//

#include <Common/Network.h>
#include <Common/Constants.h>
#include <Common/PingPongPackets.h>
#include <iostream>
#include <thread>

///
///
///     CODE DE TEST
///
///
///

void processWaitingPackets(sf::UdpSocket &socket)
{
    sf::Socket::Status status;
    do
    {
        // We try to see if there is a message to process
        sf::Packet packet;
        sf::IpAddress remoteAddress;
        unsigned short remotePort;
        status = socket.receive(packet, remoteAddress, remotePort);
        if (status == sf::Socket::NotReady)
            break;

        // We process the message
        auto logicalPacket = deserializePacket(packet);
        if(logicalPacket) {
            auto response = logicalPacket->handle();
            if(response) {
                response->send(socket, remoteAddress, remotePort);
            }
        }
    }
    while (true); // We exit this loop thanks to break instruction when ((status == sf::Socket::NotReady) || (status == sf::Socket::Disconnected))
}

int main(int argc, char** argv) {
    unsigned short localPort = DEFAULT_PORT+1; // TODO: customisable
    unsigned short remotePort = DEFAULT_PORT; // TODO: customisable
    sf::IpAddress ip("localhost");
    sf::UdpSocket socket;
    sf::Socket::Status status = socket.bind(localPort);
    if (status != sf::Socket::Done)
    {
        std::cerr << "Error: Problem during binding to local port " << localPort << " (status = " << status << ") : Another process is probably already using this port" << std::endl;
        return EXIT_FAILURE;
    }
    socket.setBlocking(false);
    PingPacket().send(socket, ip, remotePort);

    sf::sleep(sf::milliseconds(1));

    while(true) {
        processWaitingPackets(socket);
        sf::sleep(sf::milliseconds(10));
        // TODO: replace with game code?
        PingPacket().send(socket, ip, remotePort);

        sf::sleep(sf::milliseconds(1));
    }
    return EXIT_SUCCESS;
}