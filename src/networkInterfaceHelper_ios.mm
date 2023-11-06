/*
* Copyright (C) 2016-2023, L-Acoustics

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
* @file networkInterfaceHelper_ios.mm
* @author Christophe Calmejane
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
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <errno.h>
#include <unordered_map>
#include <memory>
#include <string>
#include <mutex> // once, mutex
#include <cstring> // memcpy / strncmp
#include <atomic>
#include <thread>
#include <thread>

#import <UIKit/UIKit.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <Foundation/Foundation.h>
#import <SystemConfiguration/CaptiveNetwork.h>

//#define USE_REACHABILITY
#define USE_POLLING

namespace la
{
namespace networkInterface
{
/** std::string to NSString conversion */
//inline NSString* getNSString(std::string const& cString)
//{
//	return [NSString stringWithCString:cString.c_str() encoding:NSUTF8StringEncoding];
//}

/** NSString to std::string conversion */
inline std::string getStdString(NSString const* const nsString) noexcept
{
	return std::string{ [nsString UTF8String] };
}

class OsDependentDelegate_IOS final : public OsDependentDelegate
{
public:
	OsDependentDelegate_IOS(CommonDelegate& commonDelegate) noexcept
		: _commonDelegate{ commonDelegate }
	{
	}

	virtual ~OsDependentDelegate_IOS() noexcept
	{
		terminateObserverThread();
	}

private:
	// Private methods
#if defined(USE_REACHABILITY)
	static void updateReachabilityFromFlags(SCNetworkReachabilityFlags const flags) noexcept
	{
		auto reachableWithWifi = false;
		auto reachableWithCellular = false;

		// Somehow reachable
		if (flags & kSCNetworkReachabilityFlagsReachable)
		{
			// Doesn't require connection
			if ((flags & kSCNetworkReachabilityFlagsConnectionRequired) == 0)
			{
				reachableWithCellular = !!(flags & kSCNetworkReachabilityFlagsIsWWAN);
				reachableWithWifi = !(flags & kSCNetworkReachabilityFlagsIsWWAN);
			}
		}

		NSLog(@"Reachability changed: Wifi=%s Cellular=%s", reachableWithWifi ? "yes" : "no", reachableWithCellular ? "yes" : "no");
	}

	static void reachabilityChangedCallback(SCNetworkReachabilityRef target, SCNetworkReachabilityFlags flags, void* ctx) noexcept
	{
		updateReachabilityFromFlags(flags);
	}
#endif // USE_REACHABILITY

	static Interface::Type getInterfaceType(struct ifaddrs const* const ifa, int const sock) noexcept
	{
		// Check for AWDL
		if (std::strncmp(ifa->ifa_name, "awdl", 4) == 0)
			return Interface::Type::AWDL;

		// Check for loopback
		if ((ifa->ifa_flags & IFF_LOOPBACK) != 0)
			return Interface::Type::Loopback;

		// Check for WiFi will be done in another pass

		// It's an Ethernet interface
		return Interface::Type::Ethernet;
	}

	void refreshInterfaces(Interfaces& interfaces) noexcept
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
		auto pos = std::uint8_t{ 0u }; // Variable used to generate a unique 'fake' mac address for the device
		for (auto ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
		{
			// Exclude ifaddr without addr field
			if (ifa->ifa_addr == nullptr)
				continue;

			/* Per interface, we first receive a AF_LINK then any number of AF_INET* (one per IP address) */
			int family = ifa->ifa_addr->sa_family;

			/* For AF_LINK, get the mac and setup the interface struct */
			if (family == AF_LINK)
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
				// Getting iOS device mac address is forbidden since iOS7 so use a fake address
				interface.macAddress = { 0x00, 0x01, 0x02, 0x03, 0x04, pos++ };
				// Add the interface to the list
				interfaces[ifa->ifa_name] = interface;
			}
			/* For an AF_INET* interface address, get the IP */
			else if (family == AF_INET || family == AF_INET6)
			{
				if (family == AF_INET6) // Right now, we don't want ipv6 addresses
					continue;

				// Check if interface has been recorded from AF_LINK
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
					interface.ipAddressInfos.emplace_back(IPAddressInfo{ IPAddress{ host }, IPAddress{ mask } });
				}
			}
		}

		// Second pass to detect WiFi interfaces
		{
			// This unfortunately only works on a physical device, we have to hardcode something for iPhone simulator
#if TARGET_IPHONE_SIMULATOR
			if (auto const intfcIt = interfaces.find("en0"); intfcIt != interfaces.end())
			{
				intfcIt->second.type = Interface::Type::WiFi;
			}
#else // TARGET_IPHONE_SIMULATOR
			auto const* const ifsRef = CNCopySupportedInterfaces();

			if (ifsRef)
			{
				auto const* const ifs = (__bridge NSArray const*)ifsRef;
				for (NSString* ifname in ifs)
				{
					if (auto const intfcIt = interfaces.find(getStdString(ifname)); intfcIt != interfaces.end())
					{
						intfcIt->second.type = Interface::Type::WiFi;
					}
				}
				CFRelease(ifsRef);
			}
#endif // TARGET_IPHONE_SIMULATOR
		}

		// Release the socket
		close(sck);
	}

	void terminateObserverThread() noexcept
	{
#if defined(USE_REACHABILITY)
		// Stop the thread
		if (_observerThread.joinable())
		{
			// Unschedule Reachability callbacks from the loop, which will cause the loop to end (no more tasks to run)
			SCNetworkReachabilityUnscheduleFromRunLoop(*_reachability, _threadRunLoopRef, kCFRunLoopCommonModes);

			// Stop the thread's run loop (_threadRunLoopRef will be set back to nullptr inside the thread)
			CFRunLoopStop(_threadRunLoopRef);

			// Wait for the thread to complete its pending tasks
			_observerThread.join();
		}

		// Release Reachability
		_reachability = {};
#endif // USE_REACHABILITY

#if defined(USE_POLLING)
		_shouldTerminate = true;
		if (_observerThread.joinable())
		{
			_observerThread.join();
			_observerThread = {};
			_enumeratedOnce = false;
		}
#endif // USE_POLLING
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
#if defined(USE_REACHABILITY)
		// Register to Reachability
		{
			// Create a 'zero' address
			auto zeroAddress = sockaddr_in{};
			bzero(&zeroAddress, sizeof(zeroAddress));
			zeroAddress.sin_len = sizeof(zeroAddress);
			zeroAddress.sin_family = AF_INET;
			_reachability = RefGuard{ SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, reinterpret_cast<sockaddr const*>(&zeroAddress)) };

			if (_reachability)
			{
				/* Start monitoring */
				auto ctx = SCNetworkReachabilityContext{ 0, nullptr, nullptr, nullptr, nullptr };
				if (SCNetworkReachabilitySetCallback(*_reachability, reachabilityChangedCallback, &ctx))
				{
					// Create a thread with a runloop to monitor changes
					_observerThread = std::thread(
						[this]()
						{
							// Add Reachability to the thread's loop, so it has something to do
							_threadRunLoopRef = CFRunLoopGetCurrent();
							SCNetworkReachabilityScheduleWithRunLoop(*_reachability, _threadRunLoopRef, kCFRunLoopCommonModes);

							// Run the thread's loop, until stopped
							CFRunLoopRun();

							// Undefine _threadRunLoopRef but do not CFRelease it as we don't own the ref
							_threadRunLoopRef = nullptr;
						});

					// Immediately update reachability status
					auto flags = SCNetworkReachabilityFlags{ 0u };
					SCNetworkReachabilityGetFlags(*_reachability, &flags);
					updateReachabilityFromFlags(flags);
				}
			}
		}
#endif // USE_REACHABILITY
#if defined(USE_POLLING)
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
#endif // USE_POLLING
	}

	/** When the last observer is unregistered */
	virtual void onLastObserverUnregistered() noexcept override
	{
		terminateObserverThread();
	}

#if defined(USE_REACHABILITY)
	template<typename ObjectRef, typename DestroyType = CFTypeRef, void (*DestroyMethod)(DestroyType) = &CFRelease>
	class RefGuard final
	{
	public:
		RefGuard() noexcept = default;

		explicit RefGuard(ObjectRef const ptr) noexcept
			: _ptr{ ptr }
		{
		}

		RefGuard& operator=(RefGuard&& other)
		{
			release();
			std::swap(_ptr, other._ptr);
			return *this;
		}

		~RefGuard() noexcept
		{
			release();
		}

		operator bool() const noexcept
		{
			return _ptr != nullptr;
		}

		ObjectRef operator*() const
		{
			if (_ptr == nullptr)
			{
				throw std::runtime_error("Ref is nullptr");
			}
			return _ptr;
		}

		// Deleted compiler auto-generated methods
		RefGuard(RefGuard const&) = delete;
		RefGuard(RefGuard&&) = delete;
		RefGuard& operator=(RefGuard const&) = delete;

	private:
		void release()
		{
			if (_ptr != nullptr)
			{
				DestroyMethod(_ptr);
				_ptr = nullptr;
			}
		}

		ObjectRef _ptr{ nullptr };
	};
#endif // USE_REACHABILITY

	// Private members
	CommonDelegate& _commonDelegate;
	std::thread _observerThread{};
#if defined(USE_POLLING)
	bool _shouldTerminate{ false };
#endif // USE_POLLING
#if defined(USE_REACHABILITY)
	RefGuard<SCNetworkReachabilityRef> _reachability{};
	CFRunLoopRef _threadRunLoopRef{ nullptr };
	std::mutex _lock{};
#endif // USE_REACHABILITY
	std::atomic_bool _enumeratedOnce{ false };
};

std::unique_ptr<OsDependentDelegate> getOsDependentDelegate(CommonDelegate& commonDelegate) noexcept
{
	return std::make_unique<OsDependentDelegate_IOS>(commonDelegate);
}


} // namespace networkInterface
} // namespace la
