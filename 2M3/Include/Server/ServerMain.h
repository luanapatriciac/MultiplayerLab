#pragma once

#include <iostream>
#include <SFML/Network.hpp>
#include "Common/Network.h"

[[noreturn]] void networkThread(int port);
void killNetworkThread();
void interfaceThread();

[[noreturn]] void delayThread(sf::UdpSocket* socketPtr);