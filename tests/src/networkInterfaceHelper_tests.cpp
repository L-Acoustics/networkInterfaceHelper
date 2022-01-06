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

// Public API
#include <la/networkInterfaceHelper/networkInterfaceHelper.hpp>

// Internal API
#include "networkInterfaceHelper_common.hpp"

#include <gtest/gtest.h>

#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>

/* ************************************************************ */
/* Static Method Tests                                          */
/* ************************************************************ */
TEST(NetworkInterfaceHelper, MacAddressToString)
{
	auto const& s = la::networkInterface::NetworkInterfaceHelper::macAddressToString(la::networkInterface::MacAddress{ 0, 1, 2, 3, 4, 5 });
	EXPECT_STREQ("00:01:02:03:04:05", s.c_str());
}

/*
* The purpose of this manual test is to check for valid enumeration
* after the engine has been restarted (ie. All observers removed, then a new one added)
*/
TEST(MANUAL_NetworkInterfaceHelper, EnumerationAfterRestart)
{
	class Observer final : public la::networkInterface::NetworkInterfaceHelper::Observer
	{
	public:
		using Interfaces = std::unordered_map<std::string, la::networkInterface::Interface>;

		Interfaces getInterfaces() const noexcept
		{
			auto const lg = std::lock_guard{ _lock };
			return _interfaces;
		}

		bool isConnected(std::string const& id) const noexcept
		{
			auto const lg = std::lock_guard{ _lock };
			if (auto const intfcIt = _interfaces.find(id); intfcIt != _interfaces.end())
			{
				return intfcIt->second.isConnected;
			}
			return false;
		}

	private:
		virtual void onInterfaceAdded(la::networkInterface::Interface const& intfc) noexcept override
		{
			auto const lg = std::lock_guard{ _lock };
			_interfaces[intfc.id] = intfc;
		}
		virtual void onInterfaceRemoved(la::networkInterface::Interface const& intfc) noexcept override
		{
			auto const lg = std::lock_guard{ _lock };
			_interfaces.erase(intfc.id);
		}
		virtual void onInterfaceConnectedStateChanged(la::networkInterface::Interface const& intfc, bool const isConnected) noexcept override
		{
			auto const lg = std::lock_guard{ _lock };
			if (auto intfcIt = _interfaces.find(intfc.id); intfcIt != _interfaces.end())
			{
				intfcIt->second.isConnected = isConnected;
			}
		}

		mutable std::mutex _lock{};
		Interfaces _interfaces{};
	};

	auto monitoredInterfaceID = std::string{};
	{
		auto obs = Observer{};
		la::networkInterface::NetworkInterfaceHelper::getInstance().registerObserver(&obs);
		auto const interfaces = obs.getInterfaces();
		// Search for an active ethernet interface with an IP address named "en0"
		for (auto const& [id, intfc] : interfaces)
		{
			if (intfc.type == la::networkInterface::Interface::Type::Ethernet && intfc.isEnabled && intfc.isConnected && intfc.id == "en0")
			{
				monitoredInterfaceID = id;
				break;
			}
		}
		la::networkInterface::NetworkInterfaceHelper::getInstance().unregisterObserver(&obs); // Not mandatory as observer will remove itself
		ASSERT_FALSE(monitoredInterfaceID.empty()) << "Valid interface not found, or not active";
	}

	// Wait a bit
	std::cout << "Remove the ethernet cable from the interface (you have 5 seconds)\n";
	std::this_thread::sleep_for(std::chrono::seconds(5));

	{
		auto obs = Observer{};
		la::networkInterface::NetworkInterfaceHelper::getInstance().registerObserver(&obs);
		auto const interfaces = obs.getInterfaces();
		EXPECT_FALSE(obs.isConnected(monitoredInterfaceID)) << "Interface should be seen as disconnected";
	}
}

// TODO: Complete tests
