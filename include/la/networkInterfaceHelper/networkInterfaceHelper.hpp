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
* @file networkInterfaceHelper.hpp
* @author Christophe Calmejane
* @brief OS dependent network interface helper.
*/

#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <array>
#include <functional>
#include <utility>

namespace la
{
namespace networkInterface
{
using MacAddress = std::array<std::uint8_t, 6>;

/** Gets the library version. */
std::string getLibraryVersion() noexcept;

/** Gets the library full name. */
std::string getLibraryName() noexcept;

/** Gets the library copyright. */
std::string getLibraryCopyright() noexcept;

/* ************************************************************ */
/* IPAddress class declaration                                  */
/* ************************************************************ */
class IPAddress final
{
public:
	using value_type_v4 = std::array<std::uint8_t, 4>; // "a.b.c.d" -> [0] = a, [1] = b, [2] = c, [3] = d
	using value_type_v6 = std::array<std::uint16_t, 8>; // "aa::bb::cc::dd::ee::ff::gg::hh" -> [0] = aa, [1] = bb, ..., [7] = hh
	using value_type_packed_v4 = std::uint32_t; // Packed version of an IP V4: "a.b.c.d" -> MSB = a, LSB = d
	using value_type_packed_v6 = std::pair<std::uint64_t, std::uint64_t>; // Packed version of an IP V6: "aa::bb::cc::dd::ee::ff::gg::hh" -> first MSB = aa, first LSB = dd, second MSB = ee, second LSB = hh

	enum class Type
	{
		None,
		V4,
		V6,
	};

	static struct CompatibleV6Tag
	{
	} CompatibleV6;
	static struct MappedV6Tag
	{
	} MappedV6;

	/** Default constructor. */
	IPAddress() noexcept;

	/** Constructor from value_type_v4. */
	explicit IPAddress(value_type_v4 const ipv4) noexcept;

	/** Constructor from value_type_v6. */
	explicit IPAddress(value_type_v6 const ipv6) noexcept;

	/** Constructor from a value_type_packed_v4. */
	explicit IPAddress(value_type_packed_v4 const ipv4) noexcept;

	/** Constructor from a value_type_packed_v6. */
	explicit IPAddress(value_type_packed_v6 const ipv6) noexcept;

	/** Constructor from a string. */
	explicit IPAddress(std::string const& ipString);

	/** Constructor for an IPV4 compatible IP inside a V6 one. */
	IPAddress(IPAddress const& ipv4, CompatibleV6Tag);

	/** Constructor for an IPV4 mapped IP inside a V6 one. */
	IPAddress(IPAddress const& ipv4, MappedV6Tag);

	/** Destructor. */
	~IPAddress() noexcept;

	/** Setter to change the IP value. */
	void setValue(value_type_v4 const ipv4) noexcept;

	/** Setter to change the IP value. */
	void setValue(value_type_v6 const ipv6) noexcept;

	/** Setter to change the IP value. */
	void setValue(value_type_packed_v4 const ipv4) noexcept;

	/** Setter to change the IP value. */
	void setValue(value_type_packed_v6 const ipv6) noexcept;

	/** Getter to retrieve the Type of address. */
	Type getType() const noexcept;

	/** Getter to retrieve the IP value. Throws std::invalid_argument if IPAddress is not a Type::V4. */
	value_type_v4 getIPV4() const;

	/** Getter to retrieve the IP value. Throws std::invalid_argument if IPAddress is not a Type::V6. */
	value_type_v6 getIPV6() const;

	/** Getter to retrieve the IP value in the packed format. Throws std::invalid_argument if IPAddress is not a Type::V4. */
	value_type_packed_v4 getIPV4Packed() const;

	/** Getter to retrieve the IP value in the packed format. Throws std::invalid_argument if IPAddress is not a Type::V6. */
	value_type_packed_v6 getIPV6Packed() const;

	/** True if the IPAddress contains a value, false otherwise. */
	bool isValid() const noexcept;

	/** True if the IPAddress is a V4 compatible IP inside a V6 one. */
	bool isIPV4Compatible() const noexcept;

	/** True if the IPAddress is a V4 mapped IP inside a V6 one. */
	bool isIPV4Mapped() const noexcept;

	/** Returns an IPV4 compatible IP inside a V6 one. Throws std::invalid_argument if IPAddress is not a Type::V6 or is not a V4 Compatible one. */
	IPAddress getIPV4Compatible() const;

	/** Returns an IPV4 mapped IP inside a V6 one. Throws std::invalid_argument if IPAddress is not a Type::V6 or is not a V4 Mapped one. */
	IPAddress getIPV4Mapped() const;

	/** IPV4 operator (equivalent to getIPV4()). Throws std::invalid_argument if IPAddress is not a Type::V4. */
	explicit operator value_type_v4() const;

	/** IPV6 operator (equivalent to getIPV6()). Throws std::invalid_argument if IPAddress is not a Type::V6. */
	explicit operator value_type_v6() const;

	/** IPV4 operator (equivalent to getIPV4Packed()). Throws std::invalid_argument if IPAddress is not a Type::V4. */
	explicit operator value_type_packed_v4() const;

	/** IPV6 operator (equivalent to getIPV6Packed()). Throws std::invalid_argument if IPAddress is not a Type::V6. */
	explicit operator value_type_packed_v6() const;

	/** IPAddress validity bool operator (equivalent to isValid()). */
	explicit operator bool() const noexcept;

	/** std::string convertion operator. */
	explicit operator std::string() const noexcept;

	/** Equality operator. Returns true if the IPAddress values are equal. */
	friend bool operator==(IPAddress const& lhs, IPAddress const& rhs) noexcept;

	/** Non equality operator. */
	friend bool operator!=(IPAddress const& lhs, IPAddress const& rhs) noexcept;

	/** Inferiority operator. Throws std::invalid_argument if Type is unsupported. */
	friend bool operator<(IPAddress const& lhs, IPAddress const& rhs);

	/** Inferiority or equality operator. Throws std::invalid_argument if Type is unsupported. */
	friend bool operator<=(IPAddress const& lhs, IPAddress const& rhs);

	/** Increment operator. Throws std::invalid_argument if Type is unsupported. Note: Increment value is currently limited to 32bits. */
	friend IPAddress operator+(IPAddress const& lhs, std::uint32_t const value);

	/** Decrement operator. Throws std::invalid_argument if Type is unsupported. Note: Decrement value is currently limited to 32bits. */
	friend IPAddress operator-(IPAddress const& lhs, std::uint32_t const value);

	/** operator++ Throws std::invalid_argument if Type is unsupported. */
	friend IPAddress& operator++(IPAddress& lhs);

	/** operator-- Throws std::invalid_argument if Type is unsupported. */
	friend IPAddress& operator--(IPAddress& lhs);

	/** operator& Throws std::invalid_argument if Type is unsupported. */
	friend IPAddress operator&(IPAddress const& lhs, IPAddress const& rhs);

	/** operator| Throws std::invalid_argument if Type is unsupported. */
	friend IPAddress operator|(IPAddress const& lhs, IPAddress const& rhs);

	/** Pack an IP of Type::V4. */
	static value_type_packed_v4 pack(value_type_v4 const ipv4) noexcept;

	/** Unpack an IP of Type::V4. */
	static value_type_v4 unpack(value_type_packed_v4 const ipv4) noexcept;

	/** Pack an IP of Type::V6. */
	static value_type_packed_v6 pack(value_type_v6 const ipv6) noexcept;

	/** Unpack an IP of Type::V6. */
	static value_type_v6 unpack(value_type_packed_v6 const ipv6) noexcept;

	/** Helper method to generate IPAddress::value_type_packed_v6 from prefix length. */
	static IPAddress::value_type_packed_v6 packedV6FromPrefixLength(std::uint8_t const length) noexcept;

	/** Helper method to retrieve the prefix length from IPAddress::value_type_packed_v6. */
	static std::uint8_t prefixLengthFromPackedV6(IPAddress::value_type_packed_v6 const packed) noexcept;

	/** Hash functor to be used for std::hash */
	struct hash
	{
		std::size_t operator()(IPAddress const& ip) const;
	};

	// Defaulted compiler auto-generated methods
	IPAddress(IPAddress&&) = default;
	IPAddress(IPAddress const&) = default;
	IPAddress& operator=(IPAddress const&) = default;
	IPAddress& operator=(IPAddress&&) = default;

private:
	// Private methods
	void buildIPString() noexcept;
	// Private defines
	static constexpr size_t IPStringMaxLength = 40u;
	// Private variables
	Type _type{ Type::None };
	value_type_v4 _ipv4{};
	value_type_v6 _ipv6{};
	std::array<std::string::value_type, IPStringMaxLength> _ipString{};
};

/* ************************************************************ */
/* IPAddressInfo declaration                                    */
/* ************************************************************ */
struct IPAddressInfo
{
	IPAddress address{};
	IPAddress netmask{};

	/** Gets the network base IPAddress from specified netmask. Throws std::invalid_argument if either address or netmask is invalid, or if they are not of the same IPAddress::Type */
	IPAddress getNetworkBaseAddress() const;

	/** Gets the broadcast IPAddress from specified netmask. Throws std::invalid_argument if either address or netmask is invalid, or if they are not of the same IPAddress::Type */
	IPAddress getBroadcastAddress() const;

	/** Returns true if the IPAddressInfo is in the private network range (see https://en.wikipedia.org/wiki/Private_network). Throws std::invalid_argument if either address or netmask is invalid, or if they are not of the same IPAddress::Type */
	bool isPrivateNetworkAddress() const;

	/** Equality operator. Returns true if the IPAddressInfo values are equal. */
	friend bool operator==(IPAddressInfo const& lhs, IPAddressInfo const& rhs) noexcept;

	/** Non equality operator. */
	friend bool operator!=(IPAddressInfo const& lhs, IPAddressInfo const& rhs) noexcept;

	/** Inferiority operator. Throws std::invalid_argument if IPAddress::Type of either address or netmask is unsupported. */
	friend bool operator<(IPAddressInfo const& lhs, IPAddressInfo const& rhs);

	/** Inferiority or equality operator. Throws std::invalid_argument if IPAddress::Type of either address or netmask is unsupported. */
	friend bool operator<=(IPAddressInfo const& lhs, IPAddressInfo const& rhs);
};

/* ************************************************************ */
/* Interface declaration                                        */
/* ************************************************************ */
struct Interface
{
	using IPAddressInfos = std::vector<IPAddressInfo>;
	using Gateways = std::vector<IPAddress>;

	enum class Type
	{
		None = 0, /**< Only used for initialization purpose. Never returned as a real Interface::Type */
		Loopback = 1, /**< Loopback interface */
		Ethernet = 2, /**< Ethernet interface */
		WiFi = 3, /**< 802.11 WiFi interface */
		AWDL = 4, /**< Apple Wireless Direct Link */
	};

	std::string id{}; /** Identifier of the interface (system chosen, unique) (UTF-8) */
	std::string description{}; /** Description of the interface (system chosen) (UTF-8) */
	std::string alias{}; /** Alias of the interface (often user chosen) (UTF-8) */
	MacAddress macAddress{}; /** Mac address */
	IPAddressInfos ipAddressInfos{}; /** List of IPAddressInfo attached to this interface */
	Gateways gateways{}; /** List of Gateways available for this interface */
	Type type{ Type::None }; /** The type of interface */
	bool isEnabled{ false }; /** True if this interface is enabled */
	bool isConnected{ false }; /** True if this interface is connected to a working network (able to send and receive packets) */
	bool isVirtual{ false }; /** True if this interface is emulating a physical adapter (Like BlueTooth, VirtualMachine, or Software Loopback) */
};

/** MacAddress hash functor to be used for std::hash */
struct MacAddressHash
{
	size_t operator()(MacAddress const& mac) const
	{
		size_t h = 0;
		for (auto const c : mac)
			h = h * 31 + c;
		return h;
	}
};

class NetworkInterfaceHelper
{
public:
	class Observer
	{
	public:
		/** Observer will remove itself in case it was not properly unregistered */
		virtual ~Observer() noexcept;

		/** Called when an Interface was added */
		virtual void onInterfaceAdded(la::networkInterface::Interface const& intfc) noexcept = 0;
		/** Called when an Interface was removed */
		virtual void onInterfaceRemoved(la::networkInterface::Interface const& intfc) noexcept = 0;
		/** Called when the isEnabled field of the specified Interface changed */
		virtual void onInterfaceEnabledStateChanged(la::networkInterface::Interface const& intfc, bool const isEnabled) noexcept = 0;
		/** Called when the isConnected field of the specified Interface changed */
		virtual void onInterfaceConnectedStateChanged(la::networkInterface::Interface const& intfc, bool const isConnected) noexcept = 0;
		/** Called when the alias field of the specified Interface changed */
		virtual void onInterfaceAliasChanged(la::networkInterface::Interface const& intfc, std::string const& alias) noexcept = 0;
		/** Called when the ipAddressInfos field of the specified Interface changed */
		virtual void onInterfaceIPAddressInfosChanged(la::networkInterface::Interface const& intfc, la::networkInterface::Interface::IPAddressInfos const& ipAddressInfos) noexcept = 0;
		/** Called when the gateways field of the specified Interface changed */
		virtual void onInterfaceGateWaysChanged(la::networkInterface::Interface const& intfc, la::networkInterface::Interface::Gateways const& gateways) noexcept = 0;
	};
	/** Defaulted version of the observer base class. */
	class DefaultedObserver : public Observer
	{
	public:
		/** Called when an Interface was added */
		virtual void onInterfaceAdded(la::networkInterface::Interface const& /*intfc*/) noexcept override {}
		/** Called when an Interface was removed */
		virtual void onInterfaceRemoved(la::networkInterface::Interface const& /*intfc*/) noexcept override {}
		/** Called when the isEnabled field of the specified Interface changed */
		virtual void onInterfaceEnabledStateChanged(la::networkInterface::Interface const& /*intfc*/, bool const /*isEnabled*/) noexcept override {}
		/** Called when the isConnected field of the specified Interface changed */
		virtual void onInterfaceConnectedStateChanged(la::networkInterface::Interface const& /*intfc*/, bool const /*isConnected*/) noexcept override {}
		/** Called when the alias field of the specified Interface changed */
		virtual void onInterfaceAliasChanged(la::networkInterface::Interface const& /*intfc*/, std::string const& /*alias*/) noexcept override {}
		/** Called when the ipAddressInfos field of the specified Interface changed */
		virtual void onInterfaceIPAddressInfosChanged(la::networkInterface::Interface const& /*intfc*/, la::networkInterface::Interface::IPAddressInfos const& /*ipAddressInfos*/) noexcept override {}
		/** Called when the gateways field of the specified Interface changed */
		virtual void onInterfaceGateWaysChanged(la::networkInterface::Interface const& /*intfc*/, la::networkInterface::Interface::Gateways const& /*gateways*/) noexcept override {}
	};
	using EnumerateInterfacesHandler = std::function<void(la::networkInterface::Interface const&)>;

	static NetworkInterfaceHelper& getInstance() noexcept;

	/** Converts the specified MAC address to string (in the form: xx:xx:xx:xx:xx:xx, or any chosen separator which can be empty if \0 is given) */
	static std::string macAddressToString(MacAddress const& macAddress, bool const upperCase = true, char const separator = ':') noexcept;
	/** Converts the string representation of a MAC address to a MacAddress (from the form: xx:xx:xx:xx:xx:xx or XX:XX:XX:XX:XX:XX, or any chosen separator which can be empty if \0 is given) */
	static MacAddress stringToMacAddress(std::string const& macAddressAsString, char const separator = ':'); // Throws std::invalid_argument if the string cannot be parsed
	/** Returns true if specified MAC address is valid */
	static bool isMacAddressValid(MacAddress const& macAddress) noexcept;

	/** Enumerates network interfaces. The specified handler is called for each found interface */
	void enumerateInterfaces(EnumerateInterfacesHandler const& onInterface) const noexcept;
	/** Retrieve a copy of an interface from it's name. Throws std::invalid_argument if no interface exists with that name. */
	Interface getInterfaceByName(std::string const& name) const;
	/** Registers an observer to monitor changes in network interfaces. NetworkInterfaceObserver::onInterfaceAdded will be called before returning from the call, for all already discovered interfaces. */
	void registerObserver(Observer* const observer) noexcept;
	/** Unregisters a previously registered network interfaces change observer */
	void unregisterObserver(Observer* const observer) noexcept;

	// Deleted compiler auto-generated methods
	NetworkInterfaceHelper(NetworkInterfaceHelper const&) = delete;
	NetworkInterfaceHelper(NetworkInterfaceHelper&&) = delete;
	NetworkInterfaceHelper& operator=(NetworkInterfaceHelper const&) = delete;
	NetworkInterfaceHelper& operator=(NetworkInterfaceHelper&&) = delete;

protected:
	NetworkInterfaceHelper() noexcept = default;

private:
};

} // namespace networkInterface
} // namespace la
