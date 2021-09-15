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
 * @file ipAddressInfo.cpp
 * @author Christophe Calmejane
 */

#include "networkInterfaceHelper_common.hpp"

#include <stdexcept> // invalid_argument

namespace la
{
namespace networkInterface
{
static void checkValidIPAddressInfo(IPAddress const& address, IPAddress const& netmask)
{
	// Check if address and netmask types are identical
	auto const addressType = address.getType();
	if (addressType != netmask.getType())
	{
		throw std::invalid_argument("address and netmask not of the same Type");
	}

	// Check if netmask is contiguous
	switch (addressType)
	{
		case IPAddress::Type::V4:
			validateNetmaskV4(netmask);
			break;
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

IPAddress IPAddressInfo::getNetworkBaseAddress() const
{
	checkValidIPAddressInfo(address, netmask);

	switch (address.getType())
	{
		case IPAddress::Type::V4:
		{
			return IPAddress{ address.getIPV4Packed() & netmask.getIPV4Packed() };
		}
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

IPAddress IPAddressInfo::getBroadcastAddress() const
{
	checkValidIPAddressInfo(address, netmask);

	switch (address.getType())
	{
		case IPAddress::Type::V4:
		{
			return IPAddress{ address.getIPV4Packed() | ~netmask.getIPV4Packed() };
		}
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

bool IPAddressInfo::isPrivateNetworkAddress() const
{
	checkValidIPAddressInfo(address, netmask);

	switch (address.getType())
	{
		case IPAddress::Type::V4:
		{
			auto const isInRange = [](auto const rangeStart, auto const rangeEnd, auto const rangeMask, auto const adrs, auto const mask)
			{
				return (adrs >= rangeStart) && (adrs <= rangeEnd) && (mask >= rangeMask);
			};

			constexpr auto PrivateClassAStart = IPAddress::value_type_packed_v4{ 0x0A000000 }; // 10.0.0.0
			constexpr auto PrivateClassAEnd = IPAddress::value_type_packed_v4{ 0x0AFFFFFF }; // 10.255.255.255
			constexpr auto PrivateClassAMask = IPAddress::value_type_packed_v4{ 0xFF000000 }; // 255.0.0.0
			constexpr auto PrivateClassBStart = IPAddress::value_type_packed_v4{ 0xAC100000 }; // 172.16.0.0
			constexpr auto PrivateClassBEnd = IPAddress::value_type_packed_v4{ 0xAC1FFFFF }; // 172.31.255.255
			constexpr auto PrivateClassBMask = IPAddress::value_type_packed_v4{ 0xFFF00000 }; // 255.240.0.0
			constexpr auto PrivateClassCStart = IPAddress::value_type_packed_v4{ 0xC0A80000 }; // 192.168.0.0
			constexpr auto PrivateClassCEnd = IPAddress::value_type_packed_v4{ 0xC0A8FFFF }; // 192.168.255.255
			constexpr auto PrivateClassCMask = IPAddress::value_type_packed_v4{ 0xFFFF0000 }; // 255.255.0.0

			// Get the packed address and mask for easy comparison
			auto const adrs = address.getIPV4Packed();
			auto const mask = netmask.getIPV4Packed();

			// Check if the address is in any of the ranges
			if (isInRange(PrivateClassAStart, PrivateClassAEnd, PrivateClassAMask, adrs, mask) || isInRange(PrivateClassBStart, PrivateClassBEnd, PrivateClassBMask, adrs, mask) || isInRange(PrivateClassCStart, PrivateClassCEnd, PrivateClassCMask, adrs, mask))
			{
				return true;
			}

			return false;
		}
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

bool operator==(IPAddressInfo const& lhs, IPAddressInfo const& rhs) noexcept
{
	return (lhs.address == rhs.address) && (lhs.netmask == rhs.netmask);
}

bool operator!=(IPAddressInfo const& lhs, IPAddressInfo const& rhs) noexcept
{
	return !operator==(lhs, rhs);
}

bool operator<(IPAddressInfo const& lhs, IPAddressInfo const& rhs)
{
	if (lhs.address != rhs.address)
	{
		return lhs.address < rhs.address;
	}
	return lhs.netmask < rhs.netmask;
}

bool operator<=(IPAddressInfo const& lhs, IPAddressInfo const& rhs)
{
	if (lhs.address != rhs.address)
	{
		return lhs.address < rhs.address;
	}
	return lhs.netmask <= rhs.netmask;
}


} // namespace networkInterface
} // namespace la
