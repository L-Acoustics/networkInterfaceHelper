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
* @file networkInterfaceHelper_win32.cpp
* @author Christophe Calmejane
*/

#include "networkInterfaceHelper_common.hpp"

#ifndef _MBCS
#	error "Multi-Byte Character Set required"
#endif

#include <memory>
#include <cstdint> // std::uint8_t
#include <cstring> // memcpy
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <future>
#include <chrono>
#include <WinSock2.h>
#include <Iphlpapi.h>
#include <wbemidl.h>
#include <ws2tcpip.h>
#include <Mstcpip.h>
#include <ip2string.h>
#include <Windows.h>
#include <comutil.h>
#include <objbase.h>
#pragma comment(lib, "KERNEL32.lib")
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "NTDLL.lib")
#pragma comment(lib, "WBEMUUID.lib")

#if defined(_WIN32) && defined(__clang__)
// Right now, we need to silence "ISO C++11 does not allow conversion from string literal to 'BSTR'" warning for MSVC ClangCL
#	pragma clang diagnostic ignored "-Wwritable-strings"
#endif

namespace la
{
namespace networkInterface
{
static std::string wideCharToUTF8(PWCHAR const wide) noexcept
{
	// All APIs calling this method have to provide a NULL-terminated PWCHAR
	auto const wideLength = wcsnlen_s(wide, 1024); // Compute the size, in characters, of the wide string

	if (wideLength != 0)
	{
		auto const sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wide, static_cast<int>(wideLength), nullptr, 0, nullptr, nullptr);
		auto result = std::string(static_cast<std::string::size_type>(sizeNeeded), std::string::value_type{ 0 }); // Brace-initialization constructor prevents the use of {}

		if (WideCharToMultiByte(CP_UTF8, 0, wide, static_cast<int>(wideLength), result.data(), sizeNeeded, nullptr, nullptr) > 0)
		{
			return result;
		}
	}

	return {};
}


class OsDependentDelegate_Win32 final : public OsDependentDelegate
{
public:
	OsDependentDelegate_Win32(CommonDelegate& commonDelegate) noexcept
		: _commonDelegate{ commonDelegate }
	{
	}

	virtual ~OsDependentDelegate_Win32() noexcept
	{
		terminateObserverThread();
	}

	// Deleted compiler auto-generated methods
	OsDependentDelegate_Win32(OsDependentDelegate_Win32 const&) = delete;
	OsDependentDelegate_Win32(OsDependentDelegate_Win32&&) = delete;
	OsDependentDelegate_Win32& operator=(OsDependentDelegate_Win32 const&) = delete;
	OsDependentDelegate_Win32& operator=(OsDependentDelegate_Win32&&) = delete;

private:
	// Private methods
	static Interface::Type getInterfaceType(IFTYPE const ifType) noexcept
	{
		switch (ifType)
		{
			case IF_TYPE_ETHERNET_CSMACD:
				return Interface::Type::Ethernet;
			case IF_TYPE_SOFTWARE_LOOPBACK:
				return Interface::Type::Loopback;
			case IF_TYPE_IEEE80211:
				return Interface::Type::WiFi;
			default:
				break;
		}

		// Not supported type
		return Interface::Type::None;
	}

	static bool refreshInterfaces_WMI(Interfaces& interfaces) noexcept
	{
		auto wmiSucceeded = false;

		// First pass, use WMI API to retrieve all the adapters and most of their information
		{
			// https://msdn.microsoft.com/en-us/library/Hh968170%28v=VS.85%29.aspx?f=255&MSPPError=-2147217396
			auto* locator = static_cast<IWbemLocator*>(nullptr);
			if (SUCCEEDED(CoCreateInstance(CLSID_WbemAdministrativeLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<void**>(&locator))))
			{
				auto const locatorGuard = ComObjectGuard{ locator };

				auto* service = static_cast<IWbemServices*>(nullptr);
				if (SUCCEEDED(locator->ConnectServer(L"root\\StandardCimv2", NULL, NULL, NULL, WBEM_FLAG_CONNECT_USE_MAX_WAIT, NULL, NULL, &service)))
				{
					auto const serviceGuard = ComObjectGuard{ service };

					// Set the security to Impersonate
					if (SUCCEEDED(CoSetProxyBlanket(service, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, COLE_DEFAULT_AUTHINFO, EOAC_DEFAULT)))
					{
						auto* adapterEnumerator = static_cast<IEnumWbemClassObject*>(nullptr);
						if (SUCCEEDED(service->ExecQuery(L"WQL", L"SELECT * FROM MSFT_NetAdapter", WBEM_FLAG_FORWARD_ONLY, NULL, &adapterEnumerator)))
						{
							auto const adapterEnumeratorGuard = ComObjectGuard{ adapterEnumerator };

							while (adapterEnumerator)
							{
								auto* adapter = static_cast<IWbemClassObject*>(nullptr);
								auto retcnt = ULONG{ 0u };
								if (!SUCCEEDED(adapterEnumerator->Next(WBEM_INFINITE, 1L, reinterpret_cast<IWbemClassObject**>(&adapter), &retcnt)))
								{
									adapterEnumerator = nullptr;
									continue;
								}

								auto const adapterGuard = ComObjectGuard{ adapter };

								// No more adapters
								if (retcnt == 0)
								{
									adapterEnumerator = nullptr;
									continue;
								}

								// Check if interface is hidden
								{
									VARIANT hidden;
									if (!SUCCEEDED(adapter->Get(L"Hidden", 0, &hidden, NULL, NULL)))
									{
										continue;
									}

									auto const hiddenGuard = VariantGuard{ &hidden };
									// Only process visible adapters
									if (hidden.vt != VT_BOOL || hidden.boolVal)
									{
										continue;
									}
								}

								// Create a new Interface
								Interface i;

								// Get the type of interface
								{
									VARIANT interfaceType;
									// We absolutely need the type of interface
									if (!SUCCEEDED(adapter->Get(L"InterfaceType", 0, &interfaceType, NULL, NULL)))
									{
										continue;
									}

									auto const interfaceTypeGuard = VariantGuard{ &interfaceType };
									// vt seems to be VT_I4 although the doc says it should be VT_UINT or VT_UI4, thus not checking!

									auto const type = getInterfaceType(static_cast<IFTYPE>(interfaceType.uintVal));
									// Only process supported interface types
									if (type == Interface::Type::None)
									{
										continue;
									}

									i.type = type;
								}

								// Get the interface ID
								{
									VARIANT deviceID;
									// We absolutely need the interface ID
									if (!SUCCEEDED(adapter->Get(L"DeviceID", 0, &deviceID, NULL, NULL)))
									{
										continue;
									}

									if (deviceID.vt != VT_BSTR)
									{
										continue;
									}

									i.id = wideCharToUTF8(deviceID.bstrVal);
								}

								// Get the macAddress of the interface
								{
									VARIANT macAddress;
									// We absolutely need the macAddress of the interface
									if (!SUCCEEDED(adapter->Get(L"PermanentAddress", 0, &macAddress, NULL, NULL)))
									{
										continue;
									}

									auto const macAddressGuard = VariantGuard{ &macAddress };
									if (macAddress.vt != VT_BSTR)
									{
										continue;
									}

									auto const mac = wideCharToUTF8(macAddress.bstrVal);
									// Only process adapters with a MAC address
									if (mac.empty())
									{
										continue;
									}

									try
									{
										i.macAddress = NetworkInterfaceHelper::stringToMacAddress(mac, '\0');
									}
									catch (...)
									{
										// Failed to convert macAddress, ignore this interface
										continue;
									}
								}

								// Optionally get the Description of the interface
								{
									VARIANT interfaceDescription;
									if (SUCCEEDED(adapter->Get(L"InterfaceDescription", 0, &interfaceDescription, NULL, NULL)))
									{
										auto const interfaceDescriptionGuard = VariantGuard{ &interfaceDescription };
										if (interfaceDescription.vt == VT_BSTR)
										{
											i.description = wideCharToUTF8(interfaceDescription.bstrVal);
										}
									}
								}

								// Optionally get the Friendly name of the interface
								{
									VARIANT friendlyName;
									if (SUCCEEDED(adapter->Get(L"Name", 0, &friendlyName, NULL, NULL)))
									{
										auto const friendlyNameGuard = VariantGuard{ &friendlyName };
										if (friendlyName.vt == VT_BSTR)
										{
											i.alias = wideCharToUTF8(friendlyName.bstrVal);
										}
									}
								}

								// Optionally get the Enabled State of the interface
								{
									VARIANT state;
									if (SUCCEEDED(adapter->Get(L"State", 0, &state, NULL, NULL)))
									{
										auto const stateGuard = VariantGuard{ &state };
										// vt seems to be VT_I4 although the doc says it should be VT_UINT or VT_UI4, thus not checking!

										//Unknown(0)
										//Present(1)
										//Started(2)
										//Disabled(3)

										i.isEnabled = state.uintVal == 2;
									}
									else
									{
										i.isEnabled = true; // In case we don't know, assume it's enabled
									}
								}

								// Optionally get the Operational Status of the interface
								{
									VARIANT operationalStatus;
									if (SUCCEEDED(adapter->Get(L"InterfaceOperationalStatus", 0, &operationalStatus, NULL, NULL)))
									{
										auto const operationalStatusGuard = VariantGuard{ &operationalStatus };
										// vt seems to be VT_I4 although the doc says it should be VT_UINT or VT_UI4, thus not checking!

										i.isConnected = static_cast<IF_OPER_STATUS>(operationalStatus.uintVal) == IfOperStatusUp;
									}
									else
									{
										i.isConnected = true; // In case we don't know, assume it's connected
									}
								}

								{
									VARIANT isVirtual;
									if (SUCCEEDED(adapter->Get(L"Virtual", 0, &isVirtual, NULL, NULL)))
									{
										auto const isVirtualGuard = VariantGuard{ &isVirtual };
										if (isVirtual.vt == VT_BOOL)
										{
											i.isVirtual = isVirtual.boolVal;
										}
									}
								}

								// Everything OK, save this interface
								interfaces[i.id] = i;
							}

							// Only if WMI succeeded that we will try it again
							wmiSucceeded = true;
						}
					}
				}
			}
		}

		// Second pass, get the IP configuration for all discovered adapters
		if (wmiSucceeded)
		{
			ULONG ulSize = 0;
			ULONG family = AF_INET; // AF_UNSPEC for ipV4 and ipV6, AF_INET6 for ipV6 only

			GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS, nullptr, nullptr, &ulSize); // Make an initial call to get the needed size to allocate
			auto buffer = std::make_unique<std::uint8_t[]>(ulSize);
			if (GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS, nullptr, (PIP_ADAPTER_ADDRESSES)buffer.get(), &ulSize) != ERROR_SUCCESS)
			{
				return false;
			}

			for (auto adapter = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.get()); adapter != nullptr; adapter = adapter->Next)
			{
				// Only process adapters already discovered by WMI
				auto intfcIt = interfaces.find(adapter->AdapterName);
				if (intfcIt == interfaces.end())
				{
					// Special case for the loopback interface that is not discovered by WMI, add it now
					if (adapter->IfType != IF_TYPE_SOFTWARE_LOOPBACK)
					{
						continue;
					}
					// Create a loopback Interface and setup information
					auto i = Interface{};

					i.id = adapter->AdapterName;
					i.description = wideCharToUTF8(adapter->Description);
					i.alias = wideCharToUTF8(adapter->FriendlyName);
					if (adapter->PhysicalAddressLength == i.macAddress.size())
						std::memcpy(i.macAddress.data(), adapter->PhysicalAddress, adapter->PhysicalAddressLength);
					i.type = Interface::Type::Loopback;
					i.isConnected = adapter->OperStatus == IfOperStatusUp;
					i.isEnabled = true;
					i.isVirtual = true;

					// Add it to the list, so we can get its IP addresses
					intfcIt = interfaces.emplace(adapter->AdapterName, std::move(i)).first;
				}
				auto& i = intfcIt->second;

				// Retrieve IP addresses
				for (auto ua = adapter->FirstUnicastAddress; ua != nullptr; ua = ua->Next)
				{
					auto const isIPv6 = ua->Address.lpSockaddr->sa_family == AF_INET6;
					if (isIPv6)
					{
						std::array<char, 46> ip;
						RtlIpv6AddressToStringA(&reinterpret_cast<struct sockaddr_in6*>(ua->Address.lpSockaddr)->sin6_addr, ip.data());
						try
						{
							i.ipAddressInfos.push_back(IPAddressInfo{ IPAddress{ std::string(ip.data()) }, IPAddress{ makePackedMaskV6(ua->OnLinkPrefixLength) } });
						}
						catch (...)
						{
						}
					}
					else
					{
						i.ipAddressInfos.push_back(IPAddressInfo{ IPAddress{ ntohl(reinterpret_cast<struct sockaddr_in*>(ua->Address.lpSockaddr)->sin_addr.S_un.S_addr) }, IPAddress{ makePackedMaskV4(ua->OnLinkPrefixLength) } });
					}
				}

				// Retrieve Gateways
				for (auto ga = adapter->FirstGatewayAddress; ga != nullptr; ga = ga->Next)
				{
					auto const isIPv6 = ga->Address.lpSockaddr->sa_family == AF_INET6;
					if (isIPv6)
					{
						std::array<char, 46> ip;
						RtlIpv6AddressToStringA(&reinterpret_cast<struct sockaddr_in6*>(ga->Address.lpSockaddr)->sin6_addr, ip.data());
						try
						{
							i.gateways.push_back(IPAddress{ std::string(ip.data()) });
						}
						catch (...)
						{
						}
					}
					else
					{
						i.gateways.push_back(IPAddress{ ntohl(reinterpret_cast<struct sockaddr_in*>(ga->Address.lpSockaddr)->sin_addr.S_un.S_addr) });
					}
				}
			}
		}

		return wmiSucceeded;
	}

	static void refreshInterfaces_WinAPI(Interfaces& interfaces) noexcept
	{
		// Unfortunately, GetAdaptersAddresses (even GetAdaptersInfo) is very limited and can only retrieve NICs that have IP enabled (and are active)

		ULONG ulSize = 0;
		ULONG family = AF_INET; // AF_UNSPEC for ipV4 and ipV6, AF_INET6 for ipV6 only

		GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS, nullptr, nullptr, &ulSize); // Make an initial call to get the needed size to allocate
		auto buffer = std::make_unique<std::uint8_t[]>(ulSize);
		if (GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS, nullptr, (PIP_ADAPTER_ADDRESSES)buffer.get(), &ulSize) != ERROR_SUCCESS)
		{
			return;
		}

		for (auto adapter = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.get()); adapter != nullptr; adapter = adapter->Next)
		{
			auto const type = getInterfaceType(adapter->IfType);
			// Only process supported interface types
			if (type == Interface::Type::None)
			{
				continue;
			}

			auto i = Interface{};

			i.id = adapter->AdapterName;
			i.description = wideCharToUTF8(adapter->Description);
			i.alias = wideCharToUTF8(adapter->FriendlyName);
			if (adapter->PhysicalAddressLength == i.macAddress.size())
				std::memcpy(i.macAddress.data(), adapter->PhysicalAddress, adapter->PhysicalAddressLength);
			i.type = type;
			i.isEnabled = true; // GetAdaptersAddresses (even GetAdaptersInfo) can only retrieve NICs that are active, so it's always Enabled
			i.isConnected = adapter->OperStatus == IfOperStatusUp;
			i.isVirtual = type == Interface::Type::Loopback; // GetAdaptersAddresses (even GetAdaptersInfo) cannot get the Virtual information (that WMI can), so only define Loopback as virtual

			// Retrieve IP addresses
			for (auto ua = adapter->FirstUnicastAddress; ua != nullptr; ua = ua->Next)
			{
				auto const isIPv6 = ua->Address.lpSockaddr->sa_family == AF_INET6;
				if (isIPv6)
				{
					std::array<char, 46> ip;
					RtlIpv6AddressToStringA(&reinterpret_cast<struct sockaddr_in6*>(ua->Address.lpSockaddr)->sin6_addr, ip.data());
					try
					{
						i.ipAddressInfos.push_back(IPAddressInfo{ IPAddress{ std::string(ip.data()) }, IPAddress{ makePackedMaskV6(ua->OnLinkPrefixLength) } });
					}
					catch (...)
					{
					}
				}
				else
				{
					i.ipAddressInfos.push_back(IPAddressInfo{ IPAddress{ ntohl(reinterpret_cast<struct sockaddr_in*>(ua->Address.lpSockaddr)->sin_addr.S_un.S_addr) }, IPAddress{ makePackedMaskV4(ua->OnLinkPrefixLength) } });
				}
			}

			// Retrieve Gateways
			for (auto ga = adapter->FirstGatewayAddress; ga != nullptr; ga = ga->Next)
			{
				auto const isIPv6 = ga->Address.lpSockaddr->sa_family == AF_INET6;
				if (isIPv6)
				{
					std::array<char, 46> ip;
					RtlIpv6AddressToStringA(&reinterpret_cast<struct sockaddr_in6*>(ga->Address.lpSockaddr)->sin6_addr, ip.data());
					try
					{
						i.gateways.push_back(IPAddress{ std::string(ip.data()) });
					}
					catch (...)
					{
					}
				}
				else
				{
					i.gateways.push_back(IPAddress{ ntohl(reinterpret_cast<struct sockaddr_in*>(ga->Address.lpSockaddr)->sin_addr.S_un.S_addr) });
				}
			}

			// Add it to the list, so we can get its IP addresses
			interfaces.emplace(adapter->AdapterName, std::move(i));
		}
	}

	void terminateObserverThread() noexcept
	{
		_shouldTerminate = true;
		if (_observerThread.joinable())
		{
			_observerThread.join();
			_observerThread = {};
			_observerThreadCreated = false;
			_enumeratedOnce = false;
		}
	}

	void refreshInterfaces(Interfaces& interfaces) noexcept
	{
		if (_comGuard.has_value())
		{
			if (!refreshInterfaces_WMI(interfaces))
			{
				// WMI failed, never try this again
				_comGuard.reset();

				// Clear the list, just in case we managed to get some information
				interfaces.clear();
			}
		}

		// Cannot use WMI, use an alternative method (less powerful)
		if (!_comGuard.has_value())
		{
			refreshInterfaces_WinAPI(interfaces);
		}
	}

	void createObserverThread() noexcept
	{
		auto const created = _observerThreadCreated.exchange(true);
		if (!created)
		{
			_shouldTerminate = false;
			_observerThread = std::thread(
				[this]()
				{
					utils::setCurrentThreadName("networkInterfaceHelper::ObserverPolling");

					// Try to initialize COM (this MUST be done in the thread using COM)
					if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
					{
						_comGuard.emplace();
					}

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

							// Set that we enumerated at least once
							_enumeratedOnce = true;
							_syncCondVar.notify_all();

							// Setup next check time
							nextCheck = now + std::chrono::milliseconds(1000);
						}

						// Wait a little bit so we don't burn the CPU
						std::this_thread::sleep_for(std::chrono::milliseconds(10));
					}

					// Release COM now
					_comGuard.reset();
				});
		}
	}

	// OsDependentDelegate overrides
	/** Must block until the first enumeration occured since creation */
	virtual void waitForFirstEnumeration() noexcept override
	{
		// On Windows, we need to create the observer thread as soon as possible (before waiting for onFirstObserverRegistered).
		// This is because we cannot run first enumeration manually from waitForFirstEnumeration as it's not running on the required thread for WMI to work
		createObserverThread();

		// Check if enumeration was run at least once
		if (!_enumeratedOnce)
		{
			// Never ran, wait for it to run at least once
			auto lock = std::unique_lock{ _syncLock };
			_syncCondVar.wait(lock,
				[this]
				{
					return !!_enumeratedOnce;
				});
		}
	}

	/** When the first observer is registered */
	virtual void onFirstObserverRegistered() noexcept override
	{
		createObserverThread();
	}

	/** When the last observer is unregistered */
	virtual void onLastObserverUnregistered() noexcept override
	{
		terminateObserverThread();
	}

	class ComGuard final
	{
	public:
		ComGuard() noexcept {}
		~ComGuard()
		{
			CoUninitialize();
		}

		// Compiler auto-generated methods (move-only class)
		ComGuard(ComGuard const&) = delete;
		ComGuard(ComGuard&&) = delete;
		ComGuard& operator=(ComGuard const&) = delete;
		ComGuard& operator=(ComGuard&&) = default;
	};

	template<typename ComObject>
	class ComObjectGuard final
	{
	public:
		ComObjectGuard(ComObject* ptr) noexcept
			: _ptr(ptr)
		{
		}
		~ComObjectGuard() noexcept
		{
			if (_ptr)
			{
				_ptr->Release();
			}
		}

	private:
		ComObject* _ptr{ nullptr };
	};

	class VariantGuard final
	{
	public:
		VariantGuard(VARIANT* var) noexcept
			: _var(var)
		{
		}
		~VariantGuard() noexcept
		{
			if (_var)
			{
				VariantClear(_var);
			}
		}

	private:
		VARIANT* _var{ nullptr };
	};


	// Private members
	CommonDelegate& _commonDelegate;
	std::thread _observerThread{};
	bool _shouldTerminate{ false };
	std::atomic_bool _observerThreadCreated{ false };
	std::mutex _syncLock{};
	std::condition_variable _syncCondVar{};
	std::atomic_bool _enumeratedOnce{ false };
	std::optional<ComGuard> _comGuard{ std::nullopt }; // ComGuard to be sure CoUninitialize is called upon program termination
};

std::unique_ptr<OsDependentDelegate> getOsDependentDelegate(CommonDelegate& commonDelegate) noexcept
{
	return std::make_unique<OsDependentDelegate_Win32>(commonDelegate);
}

} // namespace networkInterface
} // namespace la
