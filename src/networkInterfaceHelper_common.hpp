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
* @file networkInterfaceHelper_common.hpp
* @author Christophe Calmejane
* @brief OS independent network interface types and methods.
*/

#pragma once

#include "la/networkInterfaceHelper/networkInterfaceHelper.hpp"

#include <unordered_map>
#include <string>
#include <cstdint>
#include <stdexcept> // invalid_argument
#include <memory>

namespace la
{
namespace networkInterface
{
namespace utils
{
/** Sets the current thread name (if supported) for debugging purpose */
void setCurrentThreadName(std::string const& name) noexcept;
} // namespace utils

using Interfaces = std::unordered_map<std::string, Interface>;

/*
* Class to handle notifications and queries from common implementation
*/
class OsDependentDelegate
{
public:
	virtual ~OsDependentDelegate() noexcept = default;

	/** Must block until the first enumeration occured since creation */
	virtual void waitForFirstEnumeration() noexcept = 0;
	/** When the first observer is registered */
	virtual void onFirstObserverRegistered() noexcept = 0;
	/** When the last observer is unregistered */
	virtual void onLastObserverUnregistered() noexcept = 0;
};

/*
* Class to handle notifications and queries from OS-dependent implementation
*/
class CommonDelegate
{
public:
	virtual ~CommonDelegate() noexcept = default;

	/** When the list of interfaces changed */
	virtual void onNewInterfacesList(Interfaces&& interfaces) noexcept = 0;
	/** When an interface was added */
	virtual void onInterfaceAdded(std::string const& interfaceName, Interface&& intfc) noexcept = 0;
	/** When an interface was removed */
	virtual void onInterfaceRemoved(std::string const& interfaceName) noexcept = 0;
	/** When the Enabled state of an interface changed */
	virtual void onEnabledStateChanged(std::string const& interfaceName, bool const isEnabled) noexcept = 0;
	/** When the Connected state of an interface changed */
	virtual void onConnectedStateChanged(std::string const& interfaceName, bool const isConnected) noexcept = 0;
	/** When the Alias of an interface changed */
	virtual void onAliasChanged(std::string const& interfaceName, std::string&& alias) noexcept = 0;
	/** When the IPAddressInfos of an interface changed */
	virtual void onIPAddressInfosChanged(std::string const& interfaceName, Interface::IPAddressInfos&& ipAddressInfos) noexcept = 0;
	/** When the Gateways of an interface changed */
	virtual void onGatewaysChanged(std::string const& interfaceName, Interface::Gateways&& gateways) noexcept = 0;
};

// Methods to be implemented by eachOS-dependent implementation
std::unique_ptr<OsDependentDelegate> getOsDependentDelegate(CommonDelegate& commonDelegate) noexcept;

// Some templates shared with unit tests
constexpr IPAddress::value_type_packed_v4 makePackedMaskV4(std::uint8_t const countBits) noexcept
{
	constexpr auto MaxBits = sizeof(IPAddress::value_type_packed_v4) * 8;
	if (countBits >= MaxBits)
	{
		return ~IPAddress::value_type_packed_v4(0);
	}
	if (countBits == 0)
	{
		return IPAddress::value_type_packed_v4(0);
	}
	return ~IPAddress::value_type_packed_v4(0) << (MaxBits - countBits);
}

constexpr IPAddress::value_type_packed_v6 makePackedMaskV6(std::uint8_t const countBits) noexcept
{
	constexpr auto MaxBits = (sizeof(IPAddress::value_type_packed_v6::first_type) + sizeof(IPAddress::value_type_packed_v6::second_type)) * 8;
	if (countBits >= MaxBits)
	{
		return IPAddress::value_type_packed_v6{ ~static_cast<IPAddress::value_type_packed_v6::first_type>(0), ~static_cast<IPAddress::value_type_packed_v6::second_type>(0) };
	}
	if (countBits == 0)
	{
		return IPAddress::value_type_packed_v6{ 0, 0 };
	}
	auto mask = IPAddress::value_type_packed_v6{ ~static_cast<IPAddress::value_type_packed_v6::first_type>(0), ~static_cast<IPAddress::value_type_packed_v6::second_type>(0) };
	auto const lsbToShift = std::min(countBits, static_cast<std::uint8_t>(sizeof(IPAddress::value_type_packed_v6::second_type) * 8));
	mask.second <<= lsbToShift;
	auto const msbToShift = countBits - lsbToShift;
	mask.first <<= msbToShift;
	return mask;
}

static inline void validateNetmaskV4(IPAddress const& netmask)
{
	auto packed = netmask.getIPV4Packed();
	auto maskStarted = false;
	for (auto i = 0u; i < (sizeof(IPAddress::value_type_packed_v4) * 8); ++i)
	{
		auto const isSet = packed & 0x00000001;
		// Bit is not set, check if mask was already started
		if (!isSet)
		{
			// Already started
			if (maskStarted)
			{
				throw std::invalid_argument("netmask is not contiguous");
			}
		}
		// Bit is set, start the mask
		else
		{
			maskStarted = true;
		}
		packed >>= 1;
	}
	// At least one bit must be set
	if (!maskStarted)
	{
		throw std::invalid_argument("netmask cannot be empty");
	}
}

static inline void validateNetmaskV6(IPAddress const& netmask)
{
	// Validate netmask by converting it to prefix length then back to packed and compare (they should be the same)
	auto const packedMask = netmask.getIPV6Packed();
	auto const prefixLength = IPAddress::prefixLengthFromPackedV6(packedMask);
	auto const packedMaskFromPrefix = IPAddress::packedV6FromPrefixLength(prefixLength);
	if (packedMask != packedMaskFromPrefix)
	{
		throw std::invalid_argument("netmask is not contiguous");
	}
}

} // namespace networkInterface
} // namespace la
