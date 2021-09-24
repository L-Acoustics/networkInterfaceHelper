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
 * @file networkInterfaceHelper_common.cpp
 * @author Christophe Calmejane
 */

#include "networkInterfaceHelper_common.hpp"

#include <sstream>
#include <stdexcept> // invalid_argument
#include <iomanip> // setfill
#include <ios> // uppercase
#include <algorithm> // remove / copy
#include <string>
#include <mutex>
#include <vector>
#include <set>

#if defined(_WIN32)
#	include <Windows.h>
#else // !_WIN32
#	include <pthread.h>
#endif // _WIN32

namespace la
{
namespace networkInterface
{
namespace utils
{
void setCurrentThreadName(std::string const& name) noexcept
{
#if defined(_WIN32)
	struct
	{
		DWORD dwType;
		LPCSTR szName;
		DWORD dwThreadID;
		DWORD dwFlags;
	} info;

	info.dwType = 0x1000;
	info.szName = name.c_str();
	info.dwThreadID = GetCurrentThreadId();
	info.dwFlags = 0;

	__try
	{
		RaiseException(0x406d1388 /*MS_VC_EXCEPTION*/, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_CONTINUE_EXECUTION)
	{
	}

#elif defined(__APPLE__)
	pthread_setname_np(name.c_str());

#elif defined(__unix__)
#	if (__GLIBC__ * 1000 + __GLIBC_MINOR__) >= 2012 || defined(__ANDROID__)
	pthread_setname_np(pthread_self(), name.c_str());
#	else // !GLIBC >= 2012
	prctl(PR_SET_NAME, name.c_str(), 0, 0, 0);
#	endif

#else
	(void)name;
#endif // !WIN32 && ! __APPLE__ && ! __unix__
}
} // namespace utils

class NetworkInterfaceHelperImpl final : public NetworkInterfaceHelper, public CommonDelegate
{
public:
	NetworkInterfaceHelperImpl() noexcept {}
	virtual ~NetworkInterfaceHelperImpl() noexcept
	{
		// Destroy OS-Dependent delegate, preventing any new events from triggering after we get destroyed
		_osDependentDelegate = nullptr;
	}

	void enumerateInterfaces(EnumerateInterfacesHandler const& onInterface) const noexcept
	{
		if (onInterface == nullptr)
		{
			return;
		}

		// Wait until first enumeration occured
		_osDependentDelegate->waitForFirstEnumeration();

		// Lock
		auto const lg = std::lock_guard(_lock);

		// Now enumerate all interfaces
		for (auto const& intfcKV : _networkInterfaces)
		{
			try
			{
				onInterface(intfcKV.second);
			}
			catch (...)
			{
				// Ignore exceptions
			}
		}
	}

	Interface getInterfaceByName(std::string const& name) const
	{
		// Wait until first enumeration occured
		_osDependentDelegate->waitForFirstEnumeration();

		// Lock
		auto const lg = std::lock_guard(_lock);

		// Search specified interface name in the list
		auto const it = _networkInterfaces.find(name);
		if (it == _networkInterfaces.end())
		{
			throw std::invalid_argument("getInterfaceByName() error: No interface found with specified name");
		}
		return it->second;
	}

	void registerObserver(Observer* const observer) noexcept
	{
		// Wait until first enumeration occured
		_osDependentDelegate->waitForFirstEnumeration();

		auto isFirst = false;
		{
			// Lock
			auto const lg = std::lock_guard(_lock);

			// Check if not null
			if (observer == nullptr)
			{
				return;
			}

			// Search if observer already registered
			if (_observers.find(observer) != _observers.end())
			{
				return;
			}

			isFirst = _observers.empty();

			// Add observer
			_observers.insert(observer);

			// Now call the observer for all interfaces
			for (auto const& intfcKV : _networkInterfaces)
			{
				try
				{
					observer->onInterfaceAdded(intfcKV.second);
				}
				catch (...)
				{
					// Ignore exceptions
				}
			}
		}

		// Notify OS-dependent code outside the lock
		if (isFirst)
		{
			_osDependentDelegate->onFirstObserverRegistered();
		}
	}

	void unregisterObserver(Observer* const observer) noexcept
	{
		auto isLast = false;
		{
			// Lock
			auto const lg = std::lock_guard(_lock);

			// Check if not null
			if (observer == nullptr)
			{
				return;
			}

			// Search if observer is registered
			auto const it = _observers.find(observer);
			if (it == _observers.end())
			{
				return;
			}

			// Remove observer
			_observers.erase(it);

			// Notify OS-dependent code
			isLast = _observers.empty();
		}

		// Notify OS-dependent code outside the lock
		if (isLast)
		{
			_osDependentDelegate->onLastObserverUnregistered();
		}
	}

	// Deleted compiler auto-generated methods
	NetworkInterfaceHelperImpl(NetworkInterfaceHelperImpl const&) = delete;
	NetworkInterfaceHelperImpl(NetworkInterfaceHelperImpl&&) = delete;
	NetworkInterfaceHelperImpl& operator=(NetworkInterfaceHelperImpl const&) = delete;
	NetworkInterfaceHelperImpl& operator=(NetworkInterfaceHelperImpl&&) = delete;

private:
	// CommonDelegate overrides
	/** When the list of interfaces changed */
	virtual void onNewInterfacesList(Interfaces&& interfaces) noexcept override
	{
		// Lock
		auto const lg = std::lock_guard(_lock);

		// Compare previous interfaces and new ones
		auto addedInterfaces = std::vector<std::string>{};
		auto removedInterfaces = std::vector<std::string>{};

		// Process all previous interfaces and search if it's still present in the new list
		for (auto const& [name, previousIntfc] : _networkInterfaces)
		{
			if (interfaces.count(name) == 0)
			{
				notifyObserversMethod(&Observer::onInterfaceRemoved, previousIntfc);
			}
		}

		// Process all new interfaces and search if it was present in the previous list
		for (auto const& [name, newIntfc] : interfaces)
		{
			if (_networkInterfaces.count(name) == 0)
			{
				notifyObserversMethod(&Observer::onInterfaceAdded, newIntfc);
			}
		}

		// Process previous list and check if some property changed
		for (auto const& [name, previousIntfc] : _networkInterfaces)
		{
			if (auto const newIntfcIt = interfaces.find(name); newIntfcIt != interfaces.end())
			{
				auto const& newIntfc = newIntfcIt->second;
				if (previousIntfc.isEnabled != newIntfc.isEnabled)
				{
					notifyEnabledStateChanged(newIntfc, newIntfc.isEnabled);
				}
				if (previousIntfc.isConnected != newIntfc.isConnected)
				{
					notifyConnectedStateChanged(newIntfc, newIntfc.isConnected);
				}
				if (previousIntfc.alias != newIntfc.alias)
				{
					notifyAliasChanged(newIntfc, newIntfc.alias);
				}
				if (previousIntfc.ipAddressInfos != newIntfc.ipAddressInfos)
				{
					notifyIPAddressInfosChanged(newIntfc, newIntfc.ipAddressInfos);
				}
				if (previousIntfc.gateways != newIntfc.gateways)
				{
					notifyGatewaysChanged(newIntfc, newIntfc.gateways);
				}
			}
		}

		// Update the interfaces list
		_networkInterfaces = std::move(interfaces);
	}

	/** When the Enabled state of an interface changed */
	virtual void onEnabledStateChanged(std::string const& interfaceName, bool const isEnabled) noexcept override
	{
		// Lock
		auto const lg = std::lock_guard(_lock);

		// Search the interface matching the name
		if (auto intfcIt = _networkInterfaces.find(interfaceName); intfcIt != _networkInterfaces.end())
		{
			auto& intfc = intfcIt->second;
			if (intfc.isEnabled != isEnabled)
			{
				intfc.isEnabled = isEnabled;
				notifyEnabledStateChanged(intfc, intfc.isEnabled);
			}
		}
	}

	/** When the Connected state of an interface changed */
	virtual void onConnectedStateChanged(std::string const& interfaceName, bool const isConnected) noexcept override
	{
		// Lock
		auto const lg = std::lock_guard(_lock);

		// Search the interface matching the name
		if (auto intfcIt = _networkInterfaces.find(interfaceName); intfcIt != _networkInterfaces.end())
		{
			auto& intfc = intfcIt->second;
			if (intfc.isConnected != isConnected)
			{
				intfc.isConnected = isConnected;
				notifyConnectedStateChanged(intfc, intfc.isConnected);
			}
		}
	}

	/** When the Alias of an interface changed */
	virtual void onAliasChanged(std::string const& interfaceName, std::string&& alias) noexcept override
	{
		// Lock
		auto const lg = std::lock_guard(_lock);

		// Search the interface matching the name
		if (auto intfcIt = _networkInterfaces.find(interfaceName); intfcIt != _networkInterfaces.end())
		{
			auto& intfc = intfcIt->second;
			if (intfc.alias != alias)
			{
				intfc.alias = std::move(alias);
				notifyAliasChanged(intfc, intfc.alias);
			}
		}
	}

	/** When the IPAddressInfos of an interface changed */
	virtual void onIPAddressInfosChanged(std::string const& interfaceName, Interface::IPAddressInfos&& ipAddressInfos) noexcept override
	{
		// Lock
		auto const lg = std::lock_guard(_lock);

		// Search the interface matching the name
		if (auto intfcIt = _networkInterfaces.find(interfaceName); intfcIt != _networkInterfaces.end())
		{
			auto& intfc = intfcIt->second;
			if (intfc.ipAddressInfos != ipAddressInfos)
			{
				intfc.ipAddressInfos = std::move(ipAddressInfos);
				notifyIPAddressInfosChanged(intfc, intfc.ipAddressInfos);
			}
		}
	}

	/** When the Gateways of an interface changed */
	virtual void onGatewaysChanged(std::string const& interfaceName, Interface::Gateways&& gateways) noexcept override
	{
		// Lock
		auto const lg = std::lock_guard(_lock);

		// Search the interface matching the name
		if (auto intfcIt = _networkInterfaces.find(interfaceName); intfcIt != _networkInterfaces.end())
		{
			auto& intfc = intfcIt->second;
			if (intfc.gateways != gateways)
			{
				intfc.gateways = std::move(gateways);
				notifyGatewaysChanged(intfc, intfc.gateways);
			}
		}
	}

	// Private methods
	template<typename Method, typename... Parameters>
	void notifyObserversMethod(Method&& method, Parameters&&... params) const noexcept
	{
		if (method != nullptr)
		{
			// Lock
			auto const lg = std::lock_guard(_lock);

			// Call each observer
			for (auto* obs : _observers)
			{
				// Using try-catch to protect ourself from errors in the handler
				try
				{
					(obs->*method)(std::forward<Parameters>(params)...);
				}
				catch (...)
				{
				}
			}
		}
	}

	void notifyEnabledStateChanged(Interface const& intfc, bool const isEnabled) noexcept
	{
		notifyObserversMethod(&Observer::onInterfaceEnabledStateChanged, intfc, isEnabled);
	}

	void notifyConnectedStateChanged(Interface const& intfc, bool const isConnected) noexcept
	{
		notifyObserversMethod(&Observer::onInterfaceConnectedStateChanged, intfc, isConnected);
	}

	void notifyAliasChanged(Interface const& intfc, std::string const& alias) noexcept
	{
		notifyObserversMethod(&Observer::onInterfaceAliasChanged, intfc, alias);
	}

	void notifyIPAddressInfosChanged(Interface const& intfc, Interface::IPAddressInfos const& ipAddressInfos) noexcept
	{
		notifyObserversMethod(&Observer::onInterfaceIPAddressInfosChanged, intfc, ipAddressInfos);
	}

	void notifyGatewaysChanged(Interface const& intfc, Interface::Gateways const& gateways) noexcept
	{
		notifyObserversMethod(&Observer::onInterfaceGateWaysChanged, intfc, gateways);
	}

	// Private members
	mutable std::recursive_mutex _lock{};
	std::set<Observer*> _observers{};
	Interfaces _networkInterfaces{};
	std::unique_ptr<OsDependentDelegate> _osDependentDelegate = { getOsDependentDelegate(*this) };
};

NetworkInterfaceHelper& NetworkInterfaceHelper::getInstance() noexcept
{
	static auto s_Instance = NetworkInterfaceHelperImpl{};

	return s_Instance;
}

std::string NetworkInterfaceHelper::macAddressToString(MacAddress const& macAddress, bool const upperCase, char const separator) noexcept
{
	try
	{
		bool isFirst = true;
		std::stringstream ss;

		if (upperCase)
		{
			ss << std::uppercase;
		}
		ss << std::hex << std::setfill('0');

		for (auto const v : macAddress)
		{
			if (isFirst)
			{
				isFirst = false;
			}
			else
			{
				if (separator != '\0')
				{
					ss << separator;
				}
			}
			ss << std::setw(2) << static_cast<std::uint32_t>(v); // setw has to be called every time // Force the value as 'unsigned int' so it's not printed as a 'char' (ASCII)
		}

		return ss.str();
	}
	catch (...)
	{
		return {};
	}
}

MacAddress NetworkInterfaceHelper::stringToMacAddress(std::string const& macAddressAsString, char const separator)
{
	auto str = macAddressAsString;
	if (separator != '\0')
	{
		str.erase(std::remove(str.begin(), str.end(), separator), str.end());
	}

	auto ss = std::stringstream{};
	ss << std::hex << str;

	auto macAsInteger = std::uint64_t{};
	ss >> macAsInteger;
	if (ss.fail())
	{
		throw std::invalid_argument("Invalid MacAddress representation: " + macAddressAsString);
	}

	auto out = MacAddress{};
	out[0] = static_cast<MacAddress::value_type>((macAsInteger >> 40) & 0xFF);
	out[1] = static_cast<MacAddress::value_type>((macAsInteger >> 32) & 0xFF);
	out[2] = static_cast<MacAddress::value_type>((macAsInteger >> 24) & 0xFF);
	out[3] = static_cast<MacAddress::value_type>((macAsInteger >> 16) & 0xFF);
	out[4] = static_cast<MacAddress::value_type>((macAsInteger >> 8) & 0xFF);
	out[5] = static_cast<MacAddress::value_type>((macAsInteger >> 0) & 0xFF);

	return out;
}

bool NetworkInterfaceHelper::isMacAddressValid(MacAddress const& macAddress) noexcept
{
	if (macAddress.size() != 6)
		return false;
	for (auto const v : macAddress)
	{
		if (v != 0)
			return true;
	}
	return false;
}

void NetworkInterfaceHelper::enumerateInterfaces(EnumerateInterfacesHandler const& onInterface) const noexcept
{
	auto const& impl = static_cast<NetworkInterfaceHelperImpl const&>(*this);
	impl.enumerateInterfaces(onInterface);
}

Interface NetworkInterfaceHelper::getInterfaceByName(std::string const& name) const
{
	auto const& impl = static_cast<NetworkInterfaceHelperImpl const&>(*this);
	return impl.getInterfaceByName(name);
}

void NetworkInterfaceHelper::registerObserver(Observer* const observer) noexcept
{
	auto& impl = static_cast<NetworkInterfaceHelperImpl&>(*this);
	impl.registerObserver(observer);
}

void NetworkInterfaceHelper::unregisterObserver(Observer* const observer) noexcept
{
	auto& impl = static_cast<NetworkInterfaceHelperImpl&>(*this);
	impl.unregisterObserver(observer);
}

NetworkInterfaceHelperImpl& getPrivateInstance() noexcept
{
	auto& helper = NetworkInterfaceHelper::getInstance();
	return static_cast<NetworkInterfaceHelperImpl&>(helper);
}

NetworkInterfaceHelper::Observer::~Observer() noexcept
{
	auto& helper = getPrivateInstance();
	helper.unregisterObserver(this);
}

} // namespace networkInterface
} // namespace la
