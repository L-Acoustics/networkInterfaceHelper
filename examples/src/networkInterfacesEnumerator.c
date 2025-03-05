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
* @file networkInterfaceEnumerator.c
* @author Christophe Calmejane
* @brief Example enumerating all detected network interfaces on the local computer (using C Bindings Library).
*/

#include <la/networkInterfaceHelper/networkInterfaceHelper.h>
#include <stdio.h>

static nih_string_t getMacAddress(nih_mac_address_cp const macAddress)
{
	return LA_NIH_macAddressToString(macAddress, nih_bool_true);
	// The returned nih_string_t should be freed, but for this example we don't really care for non freed memory
}

static nih_const_string_t getInterfaceType(nih_network_interface_type_t const type)
{
	switch (type)
	{
		case nih_network_interface_type_Loopback:
			return "Loopback";
			break;
		case nih_network_interface_type_Ethernet:
			return "Ethernet";
			break;
		case nih_network_interface_type_WiFi:
			return "WiFi";
			break;
		case nih_network_interface_type_AWDL:
			return "AWDL";
			break;
		default:
			return "Unknown type";
	}
}

static void LA_NIH_BINDINGS_C_CALL_CONVENTION on_nih_enumerate_interfaces_cb(nih_network_interface_p intfc)
{
	unsigned int intNum = 1;

	printf("%d: %s\n", intNum, intfc->id);
	printf("  Description:  %s\n", intfc->description);
	printf("  Alias:        %s\n", intfc->alias);
	printf("  MacAddress:   %s\n", getMacAddress((nih_mac_address_cp)(&intfc->mac_address)));
	printf("  Type:         %s\n", getInterfaceType(intfc->type));
	printf("  Enabled:      %s\n", intfc->is_enabled ? "YES" : "NO");
	printf("  Connected:    %s\n", intfc->is_connected ? "YES" : "NO");
	printf("  Virtual:      %s\n", intfc->is_virtual ? "YES" : "NO");
	if (intfc->ip_addresses != NULL)
	{
		printf("  IP Addresses: \n");
		nih_string_t* ptr = intfc->ip_addresses;
		while (*ptr != NULL)
		{
			printf("    %s\n", *ptr);
			++ptr;
		}
	}
	if (intfc->gateways != NULL)
	{
		printf("  Gateways: \n");
		nih_string_t* ptr = intfc->gateways;
		while (*ptr != NULL)
		{
			printf("    %s\n", *ptr);
			++ptr;
		}
	}

	printf("\n");
	++intNum;

	LA_NIH_freeNetworkInterface(intfc);
}

static int displayInterfaces(void)
{
	printf("Available interfaces:\n\n");

	// Enumerate available interfaces
	LA_NIH_enumerateInterfaces(on_nih_enumerate_interfaces_cb);

	return 0;
}

int main(void)
{
	return displayInterfaces();
}
