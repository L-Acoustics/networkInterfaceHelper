/*
* Copyright (c) 2016-2021, L-Acoustics

* This file is part of LA_networkInterfaceHelper.

* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:

*  - Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  - Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*  - Neither the name of  nor the names of its contributors may be used to
*    endorse or promote products derived from this software without specific
*    prior written permission.

* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.

* You should have received a copy of the BSD 3-clause License
* along with LA_networkInterfaceHelper.  If not, see <https://opensource.org/licenses/BSD-3-Clause>.
*/

/**
* @file networkInterfacesEnumerator.cpp
* @author Christophe Calmejane
* @brief Example enumerating all detected network interfaces on the local computer.
*/

#include <la/networkInterfaceHelper/networkInterfaceHelper.hpp>

#include <iostream>
#include <string>
#include <chrono>
#include <thread>

std::ostream& operator<<(std::ostream& stream, la::networkInterface::MacAddress const& macAddress)
{
	stream << la::networkInterface::NetworkInterfaceHelper::macAddressToString(macAddress);
	return stream;
}

std::ostream& operator<<(std::ostream& stream, la::networkInterface::Interface::Type const type)
{
	switch (type)
	{
		case la::networkInterface::Interface::Type::Loopback:
			stream << "Loopback";
			break;
		case la::networkInterface::Interface::Type::Ethernet:
			stream << "Ethernet";
			break;
		case la::networkInterface::Interface::Type::WiFi:
			stream << "WiFi";
			break;
		case la::networkInterface::Interface::Type::AWDL:
			stream << "AWDL";
			break;
		default:
			break;
	}
	return stream;
}

int displayInterfaces()
{
	std::cout << "Available interfaces:\n\n";

	unsigned int intNum = 1;
	// Enumerate available interfaces
	la::networkInterface::NetworkInterfaceHelper::getInstance().enumerateInterfaces(
		[&intNum](la::networkInterface::Interface const& intfc) noexcept
		{
			try
			{
				std::cout << intNum << ": " << intfc.id << std::endl;
				std::cout << "  Description:  " << intfc.description << std::endl;
				std::cout << "  Alias:        " << intfc.alias << std::endl;
				std::cout << "  MacAddress:   " << intfc.macAddress << std::endl;
				std::cout << "  Type:         " << intfc.type << std::endl;
				std::cout << "  Enabled:      " << (intfc.isEnabled ? "YES" : "NO") << std::endl;
				std::cout << "  Connected:    " << (intfc.isConnected ? "YES" : "NO") << std::endl;
				std::cout << "  Virtual:      " << (intfc.isVirtual ? "YES" : "NO") << std::endl;
				if (!intfc.ipAddressInfos.empty())
				{
					std::cout << "  IP Addresses: " << std::endl;
					for (auto const& info : intfc.ipAddressInfos)
					{
						std::cout << "    " << static_cast<std::string>(info.address) << " (" << static_cast<std::string>(info.netmask) << ") -> " << static_cast<std::string>(info.getNetworkBaseAddress()) << " / " << static_cast<std::string>(info.getBroadcastAddress()) << std::endl;
					}
				}
				if (!intfc.gateways.empty())
				{
					std::cout << "  Gateways:     " << std::endl;
					for (auto const& ip : intfc.gateways)
					{
						std::cout << "    " << static_cast<std::string>(ip) << std::endl;
					}
				}
			}
			catch (std::exception const& e)
			{
				std::cout << "Got exception: " << e.what() << std::endl;
			}

			std::cout << std::endl;
			++intNum;
		});

	return 0;
}

int main()
{
	std::cout << "Using " << la::networkInterface::getLibraryName() << " v" << la::networkInterface::getLibraryVersion() << std::endl;
	std::cout << la::networkInterface::getLibraryCopyright() << std::endl << std::endl;

	return displayInterfaces();
}
