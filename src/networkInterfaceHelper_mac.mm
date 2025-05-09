/*
* Copyright (C) 2016-2025, L-Acoustics

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
* @file networkInterfaceHelper_mac.mm
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
#include <net/if_media.h>
#include <errno.h>
#include <unordered_map>
#include <memory>
#include <string>
#include <mutex> // once, mutex
#include <cstring> // memcpy
#include <thread>
#include <future>
#include <algorithm>
#include <chrono>

#import <SystemConfiguration/SystemConfiguration.h>
#import <Foundation/Foundation.h>

// https://developer.apple.com/library/archive/documentation/Networking/Conceptual/SystemConfigFrameworks/
// Command line tool: scutil
//   eg. Start scutil
//       List all known keys: list
//       List all enXX interface link keys: list State:/Network/Interface/en[^/]+/Link
//       Monitor Link changes for en0 interface: n.add State:/Network/Interface/en0/Link
#define DYNAMIC_STORE_NETWORK_STATE_STRING @"State:/Network/Interface/"
#define DYNAMIC_STORE_NETWORK_SERVICE_STRING @"Setup:/Network/Service/"
#define DYNAMIC_STORE_LINK_STRING @"/Link"
#define DYNAMIC_STORE_IPV4_STRING @"/IPv4"
#define DYNAMIC_STORE_INTERFACE_STRING @"/Interface"

namespace la
{
namespace networkInterface
{
/** std::string to NSString conversion */
static NSString* getNSString(std::string const& cString)
{
	return [NSString stringWithCString:cString.c_str() encoding:NSUTF8StringEncoding];
}

/** NSString to std::string conversion */
static std::string getStdString(NSString const* const nsString) noexcept
{
	return std::string{ [nsString UTF8String] };
}

static void clearIterator(io_iterator_t iterator)
{
	@autoreleasepool
	{
		io_service_t service;
		while ((service = IOIteratorNext(iterator)))
		{
			IOObjectRelease(service);
		}
	}
}

class OsDependentDelegate_MacOS final : public OsDependentDelegate
{
public:
	OsDependentDelegate_MacOS(CommonDelegate& commonDelegate) noexcept
		: _commonDelegate{ commonDelegate }
	{
	}

	virtual ~OsDependentDelegate_MacOS() noexcept
	{
		terminateObserverThread();
	}

private:
	// Private methods
	void clearInterfaceToServiceMapping() noexcept
	{
		auto const lg = std::lock_guard{ _serviceMappingLock };

		_interfaceToServiceMapping.clear();
	}

	void setInterfaceToServiceMapping(NSString const* const interfaceName, NSString const* const serviceID) noexcept
	{
		auto const lg = std::lock_guard{ _serviceMappingLock };

		_interfaceToServiceMapping[getStdString(interfaceName)] = getStdString(serviceID);
	}

	NSString* getServiceForInterface(NSString const* const interfaceName) const noexcept
	{
		auto const lg = std::lock_guard{ _serviceMappingLock };

		if (auto const serviceIt = _interfaceToServiceMapping.find(getStdString(interfaceName)); serviceIt != _interfaceToServiceMapping.end())
		{
			return getNSString(serviceIt->second);
		}

		return nullptr;
	}

	NSString* getInterfaceForService(NSString const* const serviceID) const noexcept
	{
		auto const lg = std::lock_guard{ _serviceMappingLock };

		auto const std_serviceID = getStdString(serviceID);
		for (auto const& [interfaceName, servID] : _interfaceToServiceMapping)
		{
			if (servID == std_serviceID)
			{
				return getNSString(interfaceName);
			}
		}

		return nullptr;
	}

	static bool getIsVirtualInterface(NSString* deviceName, Interface::Type const type) noexcept
	{
		if (type == Interface::Type::Loopback)
		{
			return true;
		}

		if ([deviceName hasPrefix:@"bridge"])
		{
			return true;
		}

		return false;
	}

	static Interface::Type getInterfaceType(NSString* hardware) noexcept
	{
		if ([hardware isEqualToString:@"AirPort"])
		{
			return Interface::Type::WiFi;
		}

		// Check for Ethernet
		if ([hardware isEqualToString:@"Ethernet"])
		{
			return Interface::Type::Ethernet;
		}

		// Not supported interface type
		return Interface::Type::None;
	}

	bool isInterfaceConnected(SCDynamicStoreRef const store, CFStringRef const linkStateKeyRef) const noexcept
	{
		auto isConnected = false;

		auto const value = RefGuard{ SCDynamicStoreCopyValue(store, linkStateKeyRef) };
		if (value)
		{
			isConnected = [(NSString*)[(__bridge NSDictionary const*)*value valueForKey:@"Active"] boolValue];
		}

		return isConnected;
	}

	std::string getUserDefinedName(SCDynamicStoreRef const store, NSString const* const serviceID) const noexcept
	{
		auto const key = [NSString stringWithFormat:DYNAMIC_STORE_NETWORK_SERVICE_STRING @"%@", serviceID];
		auto const* const keyRef = static_cast<CFStringRef>(key);

		auto const value = RefGuard{ SCDynamicStoreCopyValue(store, keyRef) };
		if (value)
		{
			auto* const userDefinedName = (NSString*)[(__bridge NSDictionary const*)*value valueForKey:@"UserDefinedName"];
			if (userDefinedName)
			{
				return getStdString(userDefinedName);
			}
		}
		return std::string{};
	}

	Interface::IPAddressInfos getIPAddressInfoFromKey(SCDynamicStoreRef const store, CFStringRef const ipKeyRef) const noexcept
	{
		auto ipAddressInfos = Interface::IPAddressInfos{};

		auto const value = RefGuard{ SCDynamicStoreCopyValue(store, ipKeyRef) };

		// Still have IP addresses
		if (value)
		{
			auto* const addresses = (NSArray*)[(__bridge NSDictionary const*)*value valueForKey:@"Addresses"];
			auto* const netmasks = (NSArray*)[(__bridge NSDictionary const*)*value valueForKey:@"SubnetMasks"];
			// Only parse if we have the same count of addresses and netmasks, or no masks at all
			// (No netmasks is allowed in DHCP with manual IP)
			if (netmasks == nullptr || [addresses count] == [netmasks count])
			{
				for (auto ipIndex = NSUInteger{ 0u }; ipIndex < [addresses count]; ++ipIndex)
				{
					auto const* const address = (NSString*)[addresses objectAtIndex:ipIndex];
					auto const* netmask = @"255.255.255.255";
					if (netmasks)
					{
						auto const* mask = (NSString*)[netmasks objectAtIndex:ipIndex];
						// Empty netmask is allowed in manual IP
						if (mask.length > 0)
						{
							netmask = mask;
						}
					}
					try
					{
						ipAddressInfos.push_back(IPAddressInfo{ IPAddress{ std::string{ [address UTF8String] } }, IPAddress{ std::string{ [netmask UTF8String] } } });
					}
					catch (...)
					{
					}
				}
			}
		}

		return ipAddressInfos;
	}

	Interface::IPAddressInfos getIPAddressInfo(SCDynamicStoreRef const store, NSString const* const interfaceName) const noexcept
	{
		auto ipInfos = Interface::IPAddressInfos{};

		auto const ipKey = [NSString stringWithFormat:DYNAMIC_STORE_NETWORK_STATE_STRING @"%@" DYNAMIC_STORE_IPV4_STRING, interfaceName];
		auto const* const ipKeyRef = static_cast<CFStringRef>(ipKey);
		ipInfos = getIPAddressInfoFromKey(store, ipKeyRef);

		// In case there is no cable or it was just plugged in we won't have any IPv4 information available in the State keys, so try to get it from the Setup keys
		if (ipInfos.empty())
		{
			if (auto const serviceID = getServiceForInterface(interfaceName); serviceID != nullptr)
			{
				auto const ipKey = [NSString stringWithFormat:DYNAMIC_STORE_NETWORK_SERVICE_STRING @"%@" DYNAMIC_STORE_IPV4_STRING, serviceID];
				auto const* const ipKeyRef = static_cast<CFStringRef>(ipKey);
				ipInfos = getIPAddressInfoFromKey(store, ipKeyRef);
			}
		}

		return ipInfos;
	}

	static void setOtherFieldsFromIOCTL(Interfaces& interfaces) noexcept
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

			int family = ifa->ifa_addr->sa_family;

			/* For AF_LINK, get the mac and setup the interface struct */
			if (family == AF_LINK)
			{
				// Only process interfaces that have already been recorded
				auto const intfcIt = interfaces.find(ifa->ifa_name);
				if (intfcIt == interfaces.end())
				{
					continue;
				}
				auto& interface = intfcIt->second;

				// Get media information
				struct ifmediareq ifmr;
				memset(&ifmr, 0, sizeof(ifmr));
				strncpy(ifmr.ifm_name, ifa->ifa_name, sizeof(ifmr.ifm_name));
				if (ioctl(sck, SIOCGIFMEDIA, &ifmr) != -1)
				{
					// Get the mac address contained in the AF_LINK specific data
					auto sdl = reinterpret_cast<struct sockaddr_dl*>(ifa->ifa_addr);
					if (sdl->sdl_alen == 6)
					{
						auto ptr = reinterpret_cast<unsigned char*>(LLADDR(sdl));
						std::memcpy(interface.macAddress.data(), ptr, 6);
					}
				}
			}
		}

		// Release the socket
		close(sck);
	}

	void refreshInterfaces(Interfaces& interfaces) noexcept
	{
		clearInterfaceToServiceMapping();

		// Create a temporary dynamic store session
		auto ctx = SCDynamicStoreContext{ 0, NULL, NULL, NULL, NULL };
		auto const store = RefGuard{ SCDynamicStoreCreate(nullptr, CFSTR("networkInterfaceHelper"), nullptr, &ctx) };

		if (store)
		{
			// List all services and retrieve the attached interface
			auto const serviceKeys = DYNAMIC_STORE_NETWORK_SERVICE_STRING @"[^/]+" DYNAMIC_STORE_INTERFACE_STRING;
			auto const* const serviceKeysRef = static_cast<CFStringRef>(serviceKeys);
			auto servicesArray = RefGuard{ SCDynamicStoreCopyKeyList(*store, serviceKeysRef) };
			if (servicesArray)
			{
				auto const count = CFArrayGetCount(*servicesArray);

				for (auto index = CFIndex{ 0 }; index < count; ++index)
				{
					auto const* const keyRef = static_cast<CFStringRef>(CFArrayGetValueAtIndex(*servicesArray, index));
					auto const value = RefGuard{ SCDynamicStoreCopyValue(*store, keyRef) };
					if (value)
					{
						auto const* const valueDictRef = static_cast<CFDictionaryRef>(*value);
						auto const* const valueDict = (__bridge NSDictionary const*)valueDictRef;

						NSString* const deviceName = [valueDict objectForKey:@"DeviceName"];
						NSString* const hardware = [valueDict objectForKey:@"Hardware"];
						NSString* const description = [valueDict objectForKey:@"UserDefinedName"];

						if (deviceName && hardware)
						{
							auto const* const key = (__bridge NSString const*)keyRef;
							auto const prefixLength = [DYNAMIC_STORE_NETWORK_SERVICE_STRING length];
							auto const suffixLength = [DYNAMIC_STORE_INTERFACE_STRING length];
							auto const* const serviceID = [key substringWithRange:NSMakeRange(prefixLength, [key length] - prefixLength - suffixLength)];
							setInterfaceToServiceMapping(deviceName, serviceID);

							auto interface = Interface{};
							auto const interfaceID = getStdString(deviceName);
							interface.id = interfaceID;
							interface.type = getInterfaceType(hardware);
							interface.description = getStdString(description);

							// Get User Defined Name from the DynamicStore
							interface.alias = getUserDefinedName(*store, serviceID);
							// If not defined, use the default one
							if (interface.alias.empty())
							{
								interface.alias = interface.description + " (" + interface.id + ")";
							}

							// Declared macOS services are always enabled through DynamicStore (not sure why, as Settings is able to display disabled interfaces and the __INACTIVE__ value is set in /Library/Preferences/SystemConfiguration/preferences.plist)
							interface.isEnabled = true;

							// Check if interface is connected
							{
								auto const linkStateKey = [NSString stringWithFormat:DYNAMIC_STORE_NETWORK_STATE_STRING @"%@" DYNAMIC_STORE_LINK_STRING, deviceName];
								auto const* const linkStateKeyRef = static_cast<CFStringRef>(linkStateKey);
								interface.isConnected = isInterfaceConnected(*store, linkStateKeyRef);
							}

							// Get IP Addresses info
							interface.ipAddressInfos = getIPAddressInfo(*store, deviceName);

							// Is interface Virtual
							interface.isVirtual = getIsVirtualInterface(deviceName, interface.type);

							// Add the interface to the list
							interfaces[interfaceID] = std::move(interface);
						}
					}
				}
			}

			// Set other fields we couldn't retrieve
			setOtherFieldsFromIOCTL(interfaces);

			// Remove interfaces that are not complete
			for (auto it = interfaces.begin(); it != interfaces.end(); /* Iterate inside the loop */)
			{
				auto& intfc = it->second;

				auto isValidMacAddress = false;
				for (auto const v : intfc.macAddress)
				{
					if (v != 0)
					{
						isValidMacAddress = true;
						break;
					}
				}
				if (intfc.type == Interface::Type::None || !isValidMacAddress)
				{
					// Remove from the list
					it = interfaces.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
	}

	static void dynamicStoreChangedCallback(SCDynamicStoreRef store, CFArrayRef changedKeys, void* ctx) noexcept
	{
		auto* self = static_cast<OsDependentDelegate_MacOS*>(ctx);
		auto const lg = std::lock_guard{ self->_notificationLock };

		auto const count = CFArrayGetCount(changedKeys);

		for (auto index = CFIndex{ 0 }; index < count; ++index)
		{
			auto const* const keyRef = static_cast<CFStringRef>(CFArrayGetValueAtIndex(changedKeys, index));
			auto const* const key = (__bridge NSString const*)keyRef;

			if ([key hasPrefix:DYNAMIC_STORE_NETWORK_STATE_STRING])
			{
				if ([key hasSuffix:DYNAMIC_STORE_LINK_STRING])
				{
					auto const prefixLength = [DYNAMIC_STORE_NETWORK_STATE_STRING length];
					auto const suffixLength = [DYNAMIC_STORE_LINK_STRING length];
					auto const* const interfaceName = [key substringWithRange:NSMakeRange(prefixLength, [key length] - prefixLength - suffixLength)];

					// Read value
					auto const isConnected = self->isInterfaceConnected(store, keyRef);

					// Notify
					self->_commonDelegate.onConnectedStateChanged(std::string{ [interfaceName UTF8String] }, isConnected);
				}
				else if ([key hasSuffix:DYNAMIC_STORE_IPV4_STRING])
				{
					auto const prefixLength = [DYNAMIC_STORE_NETWORK_STATE_STRING length];
					auto const suffixLength = [DYNAMIC_STORE_IPV4_STRING length];
					auto const* const interfaceName = [key substringWithRange:NSMakeRange(prefixLength, [key length] - prefixLength - suffixLength)];

					// Read IP addresses info
					auto ipAddressInfos = self->getIPAddressInfo(store, interfaceName);

					// Notify
					self->_commonDelegate.onIPAddressInfosChanged(std::string{ [interfaceName UTF8String] }, std::move(ipAddressInfos));
				}
			}
			else if ([key hasPrefix:DYNAMIC_STORE_NETWORK_SERVICE_STRING])
			{
				if ([key hasSuffix:DYNAMIC_STORE_IPV4_STRING])
				{
					auto const prefixLength = [DYNAMIC_STORE_NETWORK_SERVICE_STRING length];
					auto const suffixLength = [DYNAMIC_STORE_IPV4_STRING length];
					auto const* const serviceID = [key substringWithRange:NSMakeRange(prefixLength, [key length] - prefixLength - suffixLength)];
					auto const* const interfaceName = self->getInterfaceForService(serviceID);

					if (interfaceName)
					{
						// Read IP addresses info
						auto ipAddressInfos = self->getIPAddressInfo(store, interfaceName);

						// Notify
						self->_commonDelegate.onIPAddressInfosChanged(std::string{ [interfaceName UTF8String] }, std::move(ipAddressInfos));
					}
				}
			}
		}
	}

	static void onIONetworkControllerListChanged(void* refcon, io_iterator_t iterator) noexcept
	{
		auto* self = static_cast<OsDependentDelegate_MacOS*>(refcon);
		auto const lg = std::lock_guard{ self->_notificationLock };

		auto newList = Interfaces{};
		self->refreshInterfaces(newList);
		self->_commonDelegate.onNewInterfacesList(std::move(newList));
		clearIterator(iterator);
	}

	void terminateObserverThread() noexcept
	{
		//Clean up the IOKit code
		_controllerTerminateIterator = {};
		_controllerMatchIterator = {};
		_notificationPort = {};

		// Stop the thread
		if (_observerThread.joinable())
		{
			// Stop the thread's run loop (_threadRunLoopRef will be set back to nullptr inside the thread)
			CFRunLoopStop(_threadRunLoopRef);

			// Wait for the thread to complete its pending tasks
			_observerThread.join();
			_observerThread = {};
			_enumeratedOnce = false;
		}

		// Release the dynamic store
		_dynamicStore = {};
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
		// Register for Added/Removed interfaces notification (kernel events)
		{
			mach_port_t masterPort = 0;
			IOMasterPort(mach_task_self(), &masterPort);
			_notificationPort = RefGuard<IONotificationPortRef, IONotificationPortRef, &IONotificationPortDestroy>{ IONotificationPortCreate(masterPort) };
			if (_notificationPort)
			{
				IONotificationPortSetDispatchQueue(*_notificationPort, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));

				IOServiceAddMatchingNotification(*_notificationPort, kIOMatchedNotification, IOServiceMatching("IONetworkController"), onIONetworkControllerListChanged, this, _controllerMatchIterator.get());
				IOServiceAddMatchingNotification(*_notificationPort, kIOTerminatedNotification, IOServiceMatching("IONetworkController"), onIONetworkControllerListChanged, this, _controllerTerminateIterator.get());

				// Clear the iterators discarding already discovered adapters, we'll manually list them anyway
				clearIterator(*_controllerMatchIterator);
				clearIterator(*_controllerTerminateIterator);
			}
		}

		// Register for State change notification on all interfaces (Dynamic Store events)
		{
			auto* scKeys = [[NSMutableArray alloc] init];
#if !__has_feature(objc_arc)
			[scKeys autorelease];
#endif
			[scKeys addObject:DYNAMIC_STORE_NETWORK_STATE_STRING @"[^/]+" DYNAMIC_STORE_LINK_STRING]; // Monitor changes in the Link State
			[scKeys addObject:DYNAMIC_STORE_NETWORK_STATE_STRING @"[^/]+" DYNAMIC_STORE_IPV4_STRING]; // Monitor changes in the IPv4 State
			[scKeys addObject:DYNAMIC_STORE_NETWORK_SERVICE_STRING @"[^/]+" DYNAMIC_STORE_IPV4_STRING]; // Monitor changes in the IPv4 Setup

			/* Connect to the dynamic store */
			auto ctx = SCDynamicStoreContext{ 0, this, NULL, NULL, NULL };
			_dynamicStore = RefGuard{ SCDynamicStoreCreate(nullptr, CFSTR("networkInterfaceHelper"), dynamicStoreChangedCallback, &ctx) };

			if (_dynamicStore)
			{
				/* Start monitoring */
				if (SCDynamicStoreSetNotificationKeys(*_dynamicStore, nullptr, (__bridge CFArrayRef)scKeys))
				{
					auto threadReady = std::promise<void>{};
					_observerThread = std::thread(
						[this, &threadReady]()
						{
							// Create a source for the thread's loop
							auto const runLoopSource = RefGuard{ SCDynamicStoreCreateRunLoopSource(kCFAllocatorDefault, *_dynamicStore, 0) };

							// Add a source to the thread's loop, so it has something to do
							_threadRunLoopRef = CFRunLoopGetCurrent();
							CFRunLoopAddSource(_threadRunLoopRef, *runLoopSource, kCFRunLoopCommonModes);

							// Notify we are ready (the thread can safely be stopped from now on)
							threadReady.set_value();

							// Run the thread's loop, until stopped
							CFRunLoopRun();

							// Undefine _threadRunLoopRef but do not CFRelease it as we don't own the ref
							_threadRunLoopRef = nullptr;
						});

					// Wait for thread to be ready (don't really care if it times out, although it might means something is really bad)
					threadReady.get_future().wait_for(std::chrono::seconds(2));
				}
			}
		}
	}

	/** When the last observer is unregistered */
	virtual void onLastObserverUnregistered() noexcept override
	{
		terminateObserverThread();
	}

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

	class IteratorGuard final
	{
	public:
		IteratorGuard() noexcept = default;

		explicit IteratorGuard(io_iterator_t const value) noexcept
			: _value{ value }
		{
		}

		IteratorGuard& operator=(IteratorGuard&& other)
		{
			release();
			std::swap(_value, other._value);
			return *this;
		}

		~IteratorGuard() noexcept
		{
			release();
		}

		operator bool() const noexcept
		{
			return _value != 0;
		}

		io_iterator_t* get() noexcept
		{
			return &_value;
		}

		io_iterator_t operator*() const noexcept
		{
			return _value;
		}

		// Deleted compiler auto-generated methods
		IteratorGuard(IteratorGuard const&) = delete;
		IteratorGuard(IteratorGuard&&) = delete;
		IteratorGuard& operator=(IteratorGuard const&) = delete;

	private:
		void release()
		{
			if (_value != 0)
			{
				IOObjectRelease(_value);
				_value = 0;
			}
		}
		io_iterator_t _value{ 0 };
	};

	// Private members
	CommonDelegate& _commonDelegate;
	std::thread _observerThread{};
	std::atomic_bool _enumeratedOnce{ false };
	RefGuard<IONotificationPortRef, IONotificationPortRef, &IONotificationPortDestroy> _notificationPort{};
	IteratorGuard _controllerMatchIterator{};
	IteratorGuard _controllerTerminateIterator{};
	RefGuard<SCDynamicStoreRef> _dynamicStore{};
	CFRunLoopRef _threadRunLoopRef{ nullptr };
	mutable std::mutex _serviceMappingLock{};
	std::mutex _notificationLock{}; // Lock to prevent 2 notifications to be handled at the same time on different threads
	std::unordered_map<std::string, std::string> _interfaceToServiceMapping{};
};

std::unique_ptr<OsDependentDelegate> getOsDependentDelegate(CommonDelegate& commonDelegate) noexcept
{
	return std::make_unique<OsDependentDelegate_MacOS>(commonDelegate);
}


} // namespace networkInterface
} // namespace la
