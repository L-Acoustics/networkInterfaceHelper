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

/**
* @file networkInterfaceHelper.cpp
* @author Christophe Calmejane
* @brief Network interface helper for C bindings Library.
*/

#include <la/networkInterfaceHelper/networkInterfaceHelper.hpp>
#include "la/networkInterfaceHelper/networkInterfaceHelper.h"

#include <cstdlib>
#include <cstring> // memcpy

#ifdef _WIN32
#	define strdup _strdup
#endif // _WIN32


static void set_macAddress(la::networkInterface::MacAddress const& source, nih_mac_address_t& macAddress) noexcept
{
	std::memcpy(macAddress, source.data(), sizeof(macAddress));
}

static la::networkInterface::MacAddress make_macAddress(nih_mac_address_cp const macAddress) noexcept
{
	auto adrs = la::networkInterface::MacAddress{};

	std::memcpy(adrs.data(), macAddress, adrs.size());

	return adrs;
}

static nih_network_interface_p make_nih_network_interface(la::networkInterface::Interface const& intfc) noexcept
{
	auto ifc = new nih_network_interface_t();

	ifc->id = strdup(intfc.id.c_str());
	ifc->description = strdup(intfc.description.c_str());
	ifc->alias = strdup(intfc.alias.c_str());
	set_macAddress(intfc.macAddress, ifc->mac_address);
	if (intfc.ipAddressInfos.empty())
	{
		ifc->ip_addresses = nullptr;
	}
	else
	{
		auto const count = intfc.ipAddressInfos.size();
		ifc->ip_addresses = new nih_string_t[count + 1];
		for (auto i = 0u; i < count; ++i)
		{
			ifc->ip_addresses[i] = strdup(static_cast<std::string>(intfc.ipAddressInfos[i].address).c_str());
		}
		ifc->ip_addresses[count] = nullptr;
	}
	if (intfc.gateways.empty())
	{
		ifc->gateways = nullptr;
	}
	else
	{
		auto const count = intfc.gateways.size();
		ifc->gateways = new nih_string_t[count + 1];
		for (auto i = 0u; i < count; ++i)
		{
			ifc->gateways[i] = strdup(static_cast<std::string>(intfc.gateways[i]).c_str());
		}
		ifc->gateways[count] = nullptr;
	}
	ifc->type = static_cast<nih_network_interface_type_t>(intfc.type);
	ifc->is_enabled = static_cast<nih_bool_t>(intfc.isEnabled);
	ifc->is_connected = static_cast<nih_bool_t>(intfc.isConnected);
	ifc->is_virtual = static_cast<nih_bool_t>(intfc.isVirtual);

	return ifc;
}

static void delete_nih_network_interface(nih_network_interface_p ifc) noexcept
{
	if (ifc == nullptr)
		return;
	std::free(ifc->id);
	std::free(ifc->description);
	std::free(ifc->alias);
	if (ifc->ip_addresses != nullptr)
	{
		auto* ptr = ifc->ip_addresses;
		while (*ptr != nullptr)
		{
			std::free(*ptr);
			++ptr;
		}
		delete[] ifc->ip_addresses;
	}
	if (ifc->gateways != nullptr)
	{
		auto* ptr = ifc->gateways;
		while (*ptr != nullptr)
		{
			std::free(*ptr);
			++ptr;
		}
		delete[] ifc->gateways;
	}

	delete ifc;
}

LA_NIH_BINDINGS_C_API void LA_NIH_BINDINGS_C_CALL_CONVENTION LA_NIH_enumerateInterfaces(nih_enumerate_interfaces_cb const onInterface)
{
	la::networkInterface::NetworkInterfaceHelper::getInstance().enumerateInterfaces(
		[onInterface](la::networkInterface::Interface const& intfc)
		{
			auto* i = make_nih_network_interface(intfc);
			onInterface(i);
		});
}

LA_NIH_BINDINGS_C_API nih_network_interface_p LA_NIH_BINDINGS_C_CALL_CONVENTION LA_NIH_getInterfaceByName(nih_string_t const name)
{
	try
	{
		return make_nih_network_interface(la::networkInterface::NetworkInterfaceHelper::getInstance().getInterfaceByName(name));
	}
	catch (...)
	{
		return nullptr;
	}
}

LA_NIH_BINDINGS_C_API nih_string_t LA_NIH_BINDINGS_C_CALL_CONVENTION LA_NIH_macAddressToString(nih_mac_address_cp const macAddress, nih_bool_t const upperCase)
{
	return strdup(la::networkInterface::NetworkInterfaceHelper::macAddressToString(make_macAddress(macAddress), upperCase).c_str());
}

LA_NIH_BINDINGS_C_API nih_bool_t LA_NIH_BINDINGS_C_CALL_CONVENTION LA_NIH_isMacAddressValid(nih_mac_address_cp const macAddress)
{
	return static_cast<nih_bool_t>(la::networkInterface::NetworkInterfaceHelper::isMacAddressValid(make_macAddress(macAddress)));
}

LA_NIH_BINDINGS_C_API void LA_NIH_BINDINGS_C_CALL_CONVENTION LA_NIH_freeNetworkInterface(nih_network_interface_p intfc)
{
	delete_nih_network_interface(intfc);
}
