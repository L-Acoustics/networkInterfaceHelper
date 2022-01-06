/*
* Copyright (C) 2016-2022, L-Acoustics

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

#include "networkInterfaceHelper_common.hpp"

#ifndef _GNU_SOURCE
#	define _GNU_SOURCE /* To get defns of NI_MAXSERV and NI_MAXHOST */
#endif // !_GNU_SOURCE
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/if_link.h>
#include <linux/if_packet.h>
#include <linux/wireless.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <unordered_map>
#include <memory>
#include <string>
#include <cstring> // memcpy
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex> // once

namespace la
{
namespace networkInterface
{
class OsDependentDelegate_Unix final : public OsDependentDelegate
{
public:
	OsDependentDelegate_Unix(CommonDelegate& commonDelegate) noexcept
		: _commonDelegate{ commonDelegate }
	{
	}

	virtual ~OsDependentDelegate_Unix() noexcept
	{
		terminateObserverThread();
	}

private:
	// Private methods
	static Interface::Type getInterfaceType(struct ifaddrs const* const ifa, int const sock) noexcept
	{
		// Check for loopback
		if ((ifa->ifa_flags & IFF_LOOPBACK) != 0)
			return Interface::Type::Loopback;

		// Check for WiFi
		{
			struct iwreq wrq;
			memset(&wrq, 0, sizeof(wrq));
			strncpy(wrq.ifr_name, ifa->ifa_name, IFNAMSIZ - 1);
			if (ioctl(sock, SIOCGIWNAME, &wrq) != -1)
			{
				// TODO: Might not be 802.11 only, find a way to differenciate wireless protocols... maybe with  "wrq.u.name" with partial string match, but it may not be standard
				return Interface::Type::WiFi;
			}
		}

		// It's an Ethernet interface
		return Interface::Type::Ethernet;
	}

	static void refreshInterfaces(Interfaces& interfaces) noexcept
	{
		std::unique_ptr<struct ifaddrs, std::function<void(struct ifaddrs*)>> scopedIfa{ nullptr, [](struct ifaddrs* ptr)
			{
				if (ptr != nullptr)
					freeifaddrs(ptr);
			} };

		struct ifaddrs* ifaddr{ nullptr };
		if (getifaddrs(&ifaddr) == -1)
		{
			return;
		}
		scopedIfa.reset(ifaddr);

		// We need a socket handle for ioctl calls
		int sck = socket(AF_INET, SOCK_DGRAM, 0);
		if (sck < 0)
		{
			return;
		}

		/* Walk through linked list, maintaining head pointer so we can free list later */
		for (auto ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
		{
			// Exclude ifaddr without addr field
			if (ifa->ifa_addr == nullptr)
				continue;

			/* Per interface, we first receive a AF_PACKET then any number of AF_INET* (one per IP address) */
			int family = ifa->ifa_addr->sa_family;

			/* For an AF_PACKET, get the mac and setup the interface struct */
			if (family == AF_PACKET && ifa->ifa_data != nullptr)
			{
				Interface interface;
				interface.id = ifa->ifa_name;
				interface.description = ifa->ifa_name;
				interface.alias = ifa->ifa_name;
				interface.type = getInterfaceType(ifa, sck);
				// Check if interface is enabled
				interface.isEnabled = (ifa->ifa_flags & IFF_UP) == IFF_UP;
				// Check if interface is connected
				interface.isConnected = (ifa->ifa_flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING);
				// Is interface Virtual (TODO: Try to detect for other kinds)
				interface.isVirtual = interface.type == Interface::Type::Loopback;

				// Get the mac address contained in the AF_PACKET specific data
				auto sll = reinterpret_cast<struct sockaddr_ll*>(ifa->ifa_addr);
				if (sll->sll_halen == 6)
				{
					std::memcpy(interface.macAddress.data(), sll->sll_addr, 6);
				}
				interfaces[ifa->ifa_name] = interface;
			}
			/* For an AF_INET* interface address, get the IP */
			else if (family == AF_INET || family == AF_INET6)
			{
				if (family == AF_INET6) // Right now, we don't want ipv6 addresses
					continue;

				// Check if interface has been recorded from AF_PACKET
				auto intfcIt = interfaces.find(ifa->ifa_name);
				if (intfcIt != interfaces.end())
				{
					auto& interface = intfcIt->second;

					char host[NI_MAXHOST];
					auto ret = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), host, sizeof(host) - 1, nullptr, 0, NI_NUMERICHOST);
					if (ret != 0)
					{
						continue;
					}
					host[NI_MAXHOST - 1] = 0;

					char mask[NI_MAXHOST];
					ret = getnameinfo(ifa->ifa_netmask, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), mask, sizeof(mask) - 1, nullptr, 0, NI_NUMERICHOST);
					if (ret != 0)
					{
						continue;
					}
					mask[NI_MAXHOST - 1] = 0;

					// Add the IP address of that interface
					try
					{
						interface.ipAddressInfos.emplace_back(IPAddressInfo{ IPAddress{ host }, IPAddress{ mask } });
					}
					catch (...)
					{
					}
				}
			}
		}

		// Release the socket
		close(sck);
	}

	void terminateObserverThread() noexcept
	{
		_shouldTerminate = true;
		if (_observerThread.joinable())
		{
			_observerThread.join();
			_observerThread = {};
			_enumeratedOnce = false;
		}
	}

	// OsDependentDelegate overrides
	/** Must block until the first enumeration occured since creation */
	virtual void waitForFirstEnumeration() noexcept override
	{
		// Check if enumeration was run at least once
		if (!_enumeratedOnce)
		{
			auto newList = Interfaces{};
			refreshInterfaces(newList);
			_commonDelegate.onNewInterfacesList(std::move(newList));
			// Set that we enumerated at least once
			_enumeratedOnce = true;
		}
	}

	/** When the first observer is registered */
	virtual void onFirstObserverRegistered() noexcept override
	{
		_shouldTerminate = false;
		_observerThread = std::thread(
			[this]()
			{
				utils::setCurrentThreadName("networkInterfaceHelper::ObserverPolling");
				auto nextCheck = std::chrono::time_point<std::chrono::system_clock>{};
				while (!_shouldTerminate)
				{
					auto const now = std::chrono::system_clock::now();
					if (now >= nextCheck)
					{
						auto newList = Interfaces{};
						refreshInterfaces(newList);

						// Check for changes in Interfaces
						_commonDelegate.onNewInterfacesList(std::move(newList));

						// Setup next check time
						nextCheck = now + std::chrono::milliseconds(1000);
					}

					// Wait a little bit so we don't burn the CPU
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}
			});
	}

	/** When the last observer is unregistered */
	virtual void onLastObserverUnregistered() noexcept override
	{
		terminateObserverThread();
	}

	// Private members
	CommonDelegate& _commonDelegate;
	std::thread _observerThread{};
	bool _shouldTerminate{ false };
	std::atomic_bool _enumeratedOnce{ false };
};

std::unique_ptr<OsDependentDelegate> getOsDependentDelegate(CommonDelegate& commonDelegate) noexcept
{
	return std::make_unique<OsDependentDelegate_Unix>(commonDelegate);
}

} // namespace networkInterface
} // namespace la
