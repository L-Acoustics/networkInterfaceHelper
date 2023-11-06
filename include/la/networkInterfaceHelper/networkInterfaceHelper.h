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
* @file networkInterfaceHelper.h
* @author Christophe Calmejane
* @brief Network interface helper for C bindings Library.
*/

#pragma once

#include "internals/exports.h"

typedef char const* nih_const_string_t;
typedef char* nih_string_t;
typedef unsigned char nih_mac_address_t[6];
typedef nih_mac_address_t* nih_mac_address_p;
typedef nih_mac_address_t const* nih_mac_address_cp;

typedef enum nih_bool_e
{
	nih_bool_false = 0,
	nih_bool_true = 1,
} nih_bool_t;

/** Valid values for nih_bool_t */
typedef enum nih_network_interface_type_e
{
	nih_network_interface_type_None = 0, /**< Only used for initialization purpose. Never returned as a real Interface::Type */
	nih_network_interface_type_Loopback = 1, /**< Loopback interface */
	nih_network_interface_type_Ethernet = 2, /**< Ethernet interface */
	nih_network_interface_type_WiFi = 3, /**< 802.11 WiFi interface */
	nih_network_interface_type_AWDL = 4, /**< Apple Wireless Direct Link */
} nih_network_interface_type_t;

typedef struct nih_network_interface_s
{
	nih_string_t id; /** Identifier of the interface (system chosen, unique) (UTF-8) */
	nih_string_t description; /** Description of the interface (system chosen) (UTF-8) */
	nih_string_t alias; /** Alias of the interface (often user chosen) (UTF-8) */
	nih_mac_address_t mac_address; /** Mac address */
	nih_string_t* ip_addresses; /** List of IP addresses attached to this interface, terminated with NULL */
	nih_string_t* gateways; /** List of Gateways available for this interface, terminated with NULL */
	nih_network_interface_type_t type; /** The type of interface */
	nih_bool_t is_enabled; /** True if this interface is enabled */
	nih_bool_t is_connected; /** True if this interface is connected to a working network (able to send and receive packets) */
	nih_bool_t is_virtual; /** True if this interface is emulating a physical adapter (Like BlueTooth, VirtualMachine, or Software Loopback) */
} nih_network_interface_t, *nih_network_interface_p;
typedef nih_network_interface_t const* nih_network_interface_cp;

/** LA_NIH_freeNetworkInterface must be called on each returned 'intfc' when no longer needed. */
typedef void(LA_NIH_BINDINGS_C_CALL_CONVENTION* nih_enumerate_interfaces_cb)(nih_network_interface_p intfc);

/** Enumerates network interfaces. The specified handler is called for each found interface. */
LA_NIH_BINDINGS_C_API void LA_NIH_BINDINGS_C_CALL_CONVENTION LA_NIH_enumerateInterfaces(nih_enumerate_interfaces_cb const onInterface);
/** Retrieve a copy of an interface from it's name. Returns NULL if no interface exists with that name. LA_NIH_freeNetworkInterface must be called on the returned interface. */
LA_NIH_BINDINGS_C_API nih_network_interface_p LA_NIH_BINDINGS_C_CALL_CONVENTION LA_NIH_getInterfaceByName(nih_string_t const name);
/** Converts the specified MAC address to string (in the form: xx:xx:xx:xx:xx:xx). LA_NIH_freeString must be called on the returned string. */
LA_NIH_BINDINGS_C_API nih_string_t LA_NIH_BINDINGS_C_CALL_CONVENTION LA_NIH_macAddressToString(nih_mac_address_cp const macAddress, nih_bool_t const upperCase);
/** Returns true if specified MAC address is valid. */
LA_NIH_BINDINGS_C_API nih_bool_t LA_NIH_BINDINGS_C_CALL_CONVENTION LA_NIH_isMacAddressValid(nih_mac_address_cp const macAddress);
/** Frees nih_network_interface_p. */
LA_NIH_BINDINGS_C_API void LA_NIH_BINDINGS_C_CALL_CONVENTION LA_NIH_freeNetworkInterface(nih_network_interface_p intfc);
