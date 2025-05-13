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
 * @file ipAddress.cpp
 * @author Christophe Calmejane
 */

#include "networkInterfaceHelper_common.hpp"

#include <sstream>
#include <stdexcept> // invalid_argument
#include <algorithm> // copy
#include <string>
#include <vector>
#include <cstring> // strncmp
#include <cassert>
#include <limits> // numeric_limits
#include <optional>

namespace la
{
namespace networkInterface
{
namespace utils
{
inline std::vector<std::string> tokenizeString(std::string const& inputString, char const separator, bool const emptyIsToken) noexcept
{
	auto const* const str = inputString.c_str();
	auto const pathLen = inputString.length();

	auto tokensArray = std::vector<std::string>{};
	auto startPos = size_t{ 0u };
	auto currentPos = size_t{ 0u };
	while (currentPos < pathLen)
	{
		// Check if we found our char
		while (str[currentPos] == separator && currentPos < pathLen)
		{
			auto const foundPos = currentPos;
			if (!emptyIsToken)
			{
				// And trim consecutive chars
				while (str[currentPos] == separator)
				{
					currentPos++;
				}
			}
			else
			{
				currentPos++;
			}
			auto const subStr = inputString.substr(startPos, foundPos - startPos);
			if (!subStr.empty() || emptyIsToken)
			{
				tokensArray.push_back(subStr);
			}
			startPos = currentPos;
		}
		currentPos++;
	}

	// Add what remains as a token (except if empty and !emptyIsToken)
	auto const subStr = inputString.substr(startPos, pathLen - startPos);
	if (!subStr.empty() || emptyIsToken)
	{
		tokensArray.push_back(subStr);
	}

	return tokensArray;
}

/** Useful template to convert the string representation of any integer to its underlying type. */
template<typename T>
T convertFromString(char const* const str)
{
	static_assert(sizeof(T) > 1, "convertFromString<T> must not be char type");

	if (std::strncmp(str, "0b", 2) == 0)
	{
		char* endptr = nullptr;
		auto c = std::strtoll(str + 2, &endptr, 2);
		if (endptr != nullptr && *endptr) // Conversion failed
		{
			throw std::invalid_argument(str);
		}
		return static_cast<T>(c);
	}
	std::stringstream ss;
	if (std::strncmp(str, "0x", 2) == 0 || std::strncmp(str, "0X", 2) == 0)
	{
		ss << std::hex;
	}

	ss << str;
	T out;
	ss >> out;
	if (ss.fail())
	{
		throw std::invalid_argument(str);
	}
	return out;
}
} // namespace utils

constexpr auto MaxTokensV6 = 8u;
constexpr auto EmbeddedIPv4Mask = IPAddress::value_type_packed_v6{ 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFF00000000 };
constexpr auto EmbeddedIPv4CompatibleValue = IPAddress::value_type_packed_v6{ 0x0000000000000000, 0x0000000000000000 };
constexpr auto EmbeddedIPv4MappedValue = IPAddress::value_type_packed_v6{ 0x0000000000000000, 0x0000FFFF00000000 };

static bool isEmbeddedIPv4Compatible(IPAddress::value_type_packed_v6 const& ip) noexcept
{
	if ((ip.first & EmbeddedIPv4Mask.first) == EmbeddedIPv4CompatibleValue.first && (ip.second & EmbeddedIPv4Mask.second) == EmbeddedIPv4CompatibleValue.second)
	{
		return true;
	}
	return false;
}

static bool isEmbeddedIPv4Mapped(IPAddress::value_type_packed_v6 const& ip) noexcept
{
	if ((ip.first & EmbeddedIPv4Mask.first) == EmbeddedIPv4MappedValue.first && (ip.second & EmbeddedIPv4Mask.second) == EmbeddedIPv4MappedValue.second)
	{
		return true;
	}
	return false;
}

IPAddress::IPAddress() noexcept
	: _ipString{ "Invalid IP" }
{
}

IPAddress::IPAddress(value_type_v4 const ipv4) noexcept
{
	setValue(ipv4);
}

IPAddress::IPAddress(value_type_v6 const ipv6) noexcept
{
	setValue(ipv6);
}

IPAddress::IPAddress(value_type_packed_v4 const ipv4) noexcept
{
	setValue(ipv4);
}

IPAddress::IPAddress(value_type_packed_v6 const ipv6) noexcept
{
	setValue(ipv6);
}

IPAddress::IPAddress(std::string const& ipString)
{
	auto const parseIPV4 = [](auto const& ipString)
	{
		auto tokens = utils::tokenizeString(ipString, '.', false);

		value_type_v4 ip{};
		if (tokens.size() != ip.size())
		{
			throw std::invalid_argument("Invalid IPV4 format");
		}
		for (auto i = 0u; i < ip.size(); ++i)
		{
			// Using std::uint16_t for convertFromString since it's not possible to use 'char' type for this method
			auto const tokenValue = utils::convertFromString<std::uint16_t>(tokens[i].c_str());
			// Check if parsed value doesn't exceed max value for a value_type_v4 single element
			if (tokenValue > std::integral_constant<decltype(tokenValue), std::numeric_limits<decltype(ip)::value_type>::max()>::value)
			{
				throw std::invalid_argument("Invalid IPV4 value");
			}
			ip[i] = static_cast<decltype(ip)::value_type>(tokenValue);
		}
		return ip;
	};

	auto tokensV6 = utils::tokenizeString(ipString, ':', true);
	// If we have at least 2 tokens, we have an IPV6: since IPV4 is at most 1 token (the separator is '.') and IPV6 is at least 2 tokens even if only '::' (which would count as 3 tokens)
	if (tokensV6.size() >= 2)
	{
		auto const packValue = [](value_type_packed_v6& packedIP, std::uint16_t const value, auto const index)
		{
			// Index 0 is the MSB
			// Check index doesn't go beyond the packedIP size
			if (index >= MaxTokensV6)
			{
				throw std::invalid_argument("Invalid IPV6 format");
			}
			// Compute which part of packedIP we need to update
			if (index < 4)
			{
				packedIP.first |= static_cast<value_type_packed_v6::first_type>(value) << (48 - (index * 16));
			}
			else
			{
				packedIP.second |= static_cast<value_type_packed_v6::second_type>(value) << (48 - ((index - 4) * 16));
			}
		};

		// Special case to handle where the first token was empty, immediatelly followed by another empty token (ie. "::xxx").
		if (tokensV6[0].empty() && tokensV6[1].empty())
		{
			// Remove the first empty token
			tokensV6.erase(tokensV6.begin());
		}

		// Special case to handle where the last token was empty, immediatelly preceeded by another empty token (ie. "xxx::").
		if (tokensV6[tokensV6.size() - 1].empty() && tokensV6[tokensV6.size() - 2].empty())
		{
			// Remove the last empty token
			tokensV6.erase(tokensV6.end() - 1);
		}

		// Compute the number of empty token sequences we have (get the index and length of the sequence) and check if we have an IPV4 embedded
		auto emptyTokenCount = 0u;
		auto emptyTokens = std::optional<std::pair<size_t, size_t>>{}; // First: index of the empty token, Second: number of empty tokens
		auto isLastTokenIPv4 = false;
		for (auto i = 0u; i < tokensV6.size(); ++i)
		{
			auto const& token = tokensV6[i];
			// Check if we have an empty token
			if (token.empty())
			{
				emptyTokenCount++;
				emptyTokens = { i, MaxTokensV6 - tokensV6.size() + 1 };
			}
			// Check if it looks like an IPv4
			if (tokensV6[i].find('.') != std::string::npos)
			{
				// Only allowed for the last token
				if (i == tokensV6.size() - 1)
				{
					isLastTokenIPv4 = true;
					// If we have a sequence of empty tokens, we need to decrement the empty tokens count (an IPv4 counts as 2 tokens, 32 bits)
					if (emptyTokens.has_value())
					{
						emptyTokens->second--;
					}
				}
				else
				{
					throw std::invalid_argument("Invalid IPV6 format");
				}
			}
		}

		// Sanity checks
		{
			// Only one empty token sequence is allowed
			if (emptyTokenCount > 1)
			{
				throw std::invalid_argument("Invalid IPV6 format");
			}
			// If we don't have any empty token check for tokens count
			if (!emptyTokens.has_value())
			{
				// If we don't have an embedded IPv4, we need to have exactly 8 tokens
				if (!isLastTokenIPv4 && tokensV6.size() != MaxTokensV6)
				{
					throw std::invalid_argument("Invalid IPV6 format");
				}
				// If we have an embedded IPv4, we need to have exactly 7 tokens
				if (isLastTokenIPv4 && tokensV6.size() != MaxTokensV6 - 1)
				{
					throw std::invalid_argument("Invalid IPV6 format");
				}
			}
		}

		auto ip = value_type_packed_v6{};
		auto packPosition = 0u;
		for (auto i = 0u; i < tokensV6.size(); ++i)
		{
			// Check if we are inside the empty tokens range
			if (emptyTokens.has_value() && i == emptyTokens->first)
			{
				// Nothing to do, we are in the empty tokens range ('ip' is already 0)
				packPosition += static_cast<decltype(packPosition)>(emptyTokens->second);
				continue;
			}
			// For the last token, check if we have an IPV4 embedded
			if ((i == tokensV6.size() - 1) && isLastTokenIPv4)
			{
				assert(packPosition == MaxTokensV6 - 2 && "Invalid pack position");
				auto const ipv4 = pack(parseIPV4(tokensV6[i]));
				// Pack the IPV4 into the last 2 elements of the IPV6
				packValue(ip, static_cast<std::uint16_t>(ipv4 >> 16), packPosition++);
				packValue(ip, static_cast<std::uint16_t>(ipv4 & 0xFFFF), packPosition++);
				continue;
			}
			// Read the token
			auto const tokenValue = utils::convertFromString<std::uint16_t>(("0x" + tokensV6[i]).c_str());
			packValue(ip, tokenValue, packPosition++);
		}
		setValue(ip);
	}
	// Try to parse IPV4
	else
	{
		setValue(parseIPV4(ipString));
	}
}

IPAddress::IPAddress(IPAddress const& ipv4, CompatibleV6Tag)
{
	_type = Type::V6;
	_ipv4 = {};
	_ipv6 = unpack(EmbeddedIPv4CompatibleValue);
	auto const packedV4 = ipv4.getIPV4Packed();
	_ipv6[6] = packedV4 >> 16;
	_ipv6[7] = packedV4 & 0xFFFF;

	buildIPString();
}

IPAddress::IPAddress(IPAddress const& ipv4, MappedV6Tag)
{
	_type = Type::V6;
	_ipv4 = {};
	_ipv6 = unpack(EmbeddedIPv4MappedValue);
	auto const packedV4 = ipv4.getIPV4Packed();
	_ipv6[6] = packedV4 >> 16;
	_ipv6[7] = packedV4 & 0xFFFF;

	buildIPString();
}

IPAddress::~IPAddress() noexcept {}

void IPAddress::setValue(value_type_v4 const ipv4) noexcept
{
	_type = Type::V4;
	_ipv4 = ipv4;
	_ipv6 = {};

	buildIPString();
}

void IPAddress::setValue(value_type_v6 const ipv6) noexcept
{
	_type = Type::V6;
	_ipv4 = {};
	_ipv6 = ipv6;

	buildIPString();
}

void IPAddress::setValue(value_type_packed_v4 const ipv4) noexcept
{
	setValue(unpack(ipv4));
}

void IPAddress::setValue(value_type_packed_v6 const ipv6) noexcept
{
	setValue(unpack(ipv6));
}

IPAddress::Type IPAddress::getType() const noexcept
{
	return _type;
}

IPAddress::value_type_v4 IPAddress::getIPV4() const
{
	if (_type != Type::V4)
	{
		throw std::invalid_argument("Not an IP V4");
	}
	return _ipv4;
}

IPAddress::value_type_v6 IPAddress::getIPV6() const
{
	if (_type != Type::V6)
	{
		throw std::invalid_argument("Not an IP V6");
	}
	return _ipv6;
}

IPAddress::value_type_packed_v4 IPAddress::getIPV4Packed() const
{
	if (_type != Type::V4)
	{
		throw std::invalid_argument("Not an IP V4");
	}

	return pack(_ipv4);
}

IPAddress::value_type_packed_v6 IPAddress::getIPV6Packed() const
{
	if (_type != Type::V6)
	{
		throw std::invalid_argument("Not an IP V6");
	}

	return pack(_ipv6);
}

bool IPAddress::isValid() const noexcept
{
	return _type != Type::None;
}

bool IPAddress::isIPV4Compatible() const noexcept
{
	if (_type != Type::V6)
	{
		return false;
	}
	return isEmbeddedIPv4Compatible(getIPV6Packed());
}

bool IPAddress::isIPV4Mapped() const noexcept
{
	if (_type != Type::V6)
	{
		return false;
	}
	return isEmbeddedIPv4Mapped(getIPV6Packed());
}

IPAddress IPAddress::getIPV4Compatible() const
{
	if (!isIPV4Compatible())
	{
		throw std::invalid_argument("Not V4 Compatible");
	}
	return IPAddress{ static_cast<IPAddress::value_type_packed_v4>(_ipv6[6]) << 16 | _ipv6[7] };
}

IPAddress IPAddress::getIPV4Mapped() const
{
	if (!isIPV4Mapped())
	{
		throw std::invalid_argument("Not V4 Mapped");
	}
	return IPAddress{ static_cast<IPAddress::value_type_packed_v4>(_ipv6[6]) << 16 | _ipv6[7] };
}

IPAddress::operator value_type_v4() const
{
	return getIPV4();
}

IPAddress::operator value_type_v6() const
{
	return getIPV6();
}

IPAddress::operator value_type_packed_v4() const
{
	return getIPV4Packed();
}

IPAddress::operator value_type_packed_v6() const
{
	return getIPV6Packed();
}

IPAddress::operator bool() const noexcept
{
	return isValid();
}

IPAddress::operator std::string() const noexcept
{
	return std::string(_ipString.data());
}

bool operator==(IPAddress const& lhs, IPAddress const& rhs) noexcept
{
	if (!lhs.isValid() && !rhs.isValid())
	{
		return true;
	}
	if (lhs._type != rhs._type)
	{
		return false;
	}
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
			return lhs._ipv4 == rhs._ipv4;
		case IPAddress::Type::V6:
			return lhs._ipv6 == rhs._ipv6;
		default:
			break;
	}
	return false;
}

bool operator!=(IPAddress const& lhs, IPAddress const& rhs) noexcept
{
	return !operator==(lhs, rhs);
}

bool operator<(IPAddress const& lhs, IPAddress const& rhs)
{
	if (lhs._type != rhs._type)
	{
		return lhs._type < rhs._type;
	}
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
			return lhs._ipv4 < rhs._ipv4;
		case IPAddress::Type::V6:
			return lhs._ipv6 < rhs._ipv6;
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

bool operator<=(IPAddress const& lhs, IPAddress const& rhs)
{
	if (lhs._type != rhs._type)
	{
		return lhs._type < rhs._type;
	}
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
			return lhs._ipv4 <= rhs._ipv4;
		case IPAddress::Type::V6:
			return lhs._ipv6 <= rhs._ipv6;
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

IPAddress operator+(IPAddress const& lhs, std::uint32_t const value)
{
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
			return IPAddress{ lhs.getIPV4Packed() + value };
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

IPAddress operator-(IPAddress const& lhs, std::uint32_t const value)
{
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
			return IPAddress{ lhs.getIPV4Packed() - value };
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

IPAddress& operator++(IPAddress& lhs)
{
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
			lhs.setValue(lhs.getIPV4Packed() + 1);
			break;
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}

	return lhs;
}

IPAddress& operator--(IPAddress& lhs)
{
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
			lhs.setValue(lhs.getIPV4Packed() - 1);
			break;
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}

	return lhs;
}

IPAddress operator&(IPAddress const& lhs, IPAddress const& rhs)
{
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
			return IPAddress{ lhs.getIPV4Packed() & rhs.getIPV4Packed() };
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

IPAddress operator|(IPAddress const& lhs, IPAddress const& rhs)
{
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
			return IPAddress{ lhs.getIPV4Packed() | rhs.getIPV4Packed() };
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

IPAddress::value_type_packed_v4 IPAddress::pack(value_type_v4 const ipv4) noexcept
{
	value_type_packed_v4 ip{ 0u };

	ip |= ipv4[0] << 24;
	ip |= ipv4[1] << 16;
	ip |= ipv4[2] << 8;
	ip |= ipv4[3];

	return ip;
}

IPAddress::value_type_v4 IPAddress::unpack(value_type_packed_v4 const ipv4) noexcept
{
	value_type_v4 ip{};

	ip[0] = (ipv4 >> 24) & 0xFF;
	ip[1] = (ipv4 >> 16) & 0xFF;
	ip[2] = (ipv4 >> 8) & 0xFF;
	ip[3] = ipv4 & 0xFF;

	return ip;
}

IPAddress::value_type_packed_v6 IPAddress::pack(value_type_v6 const ipv6) noexcept
{
	value_type_packed_v6 ip{ 0u, 0u };

	ip.first = (static_cast<value_type_packed_v6::first_type>(ipv6[0]) << 48) | (static_cast<value_type_packed_v6::first_type>(ipv6[1]) << 32) | (static_cast<value_type_packed_v6::first_type>(ipv6[2]) << 16) | static_cast<value_type_packed_v6::first_type>(ipv6[3]);
	ip.second = (static_cast<value_type_packed_v6::second_type>(ipv6[4]) << 48) | (static_cast<value_type_packed_v6::second_type>(ipv6[5]) << 32) | (static_cast<value_type_packed_v6::second_type>(ipv6[6]) << 16) | static_cast<value_type_packed_v6::second_type>(ipv6[7]);

	return ip;
}

IPAddress::value_type_v6 IPAddress::unpack(value_type_packed_v6 const ipv6) noexcept
{
	value_type_v6 ip{};

	ip[0] = (ipv6.first >> 48) & 0xFFFF;
	ip[1] = (ipv6.first >> 32) & 0xFFFF;
	ip[2] = (ipv6.first >> 16) & 0xFFFF;
	ip[3] = ipv6.first & 0xFFFF;
	ip[4] = (ipv6.second >> 48) & 0xFFFF;
	ip[5] = (ipv6.second >> 32) & 0xFFFF;
	ip[6] = (ipv6.second >> 16) & 0xFFFF;
	ip[7] = ipv6.second & 0xFFFF;

	return ip;
}

std::size_t IPAddress::hash::operator()(IPAddress const& ip) const
{
	std::size_t h{ 0u };
	switch (ip._type)
	{
		case Type::V4:
			h = ip.getIPV4Packed();
			break;
		case Type::V6:
		{
			// TODO: Improve this hash
			for (auto const v : ip._ipv6)
			{
				h = h * 0x10 + v;
			}
			break;
		}
		default:
			break;
	}
	return h;
}

static std::string buildIPV4String(IPAddress::value_type_v4 const& ipv4) noexcept
{
	auto ip = std::string{};

	for (auto const v : ipv4)
	{
		if (!ip.empty())
		{
			ip.append(".");
		}
		ip.append(std::to_string(v));
	}

	return ip;
}

#if defined(__GNUC__) && !defined(__clang__)
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
// For some reason, GCC thinks some variables in this method are not initialized
#endif
static std::string buildIPV6String(IPAddress::value_type_v6 const& ipv6, IPAddress::value_type_packed_v6 const& packedIP, bool const displayAsEmbeddedIPV4) noexcept
{
	// Special case for unspecified address
	if (packedIP.first == 0 && packedIP.second == 0)
	{
		return "::";
	}
	// Special case for loopback address
	if (packedIP.first == 0 && packedIP.second == 1)
	{
		return "::1";
	}

	// First pass - Get the longest sequence of 0 and its position
	auto longestZeroSeq = std::optional<std::pair<size_t, size_t>>{};
	auto currentZeroSeq = std::optional<std::pair<size_t, size_t>>{};
	for (auto i = 0u; i < ipv6.size(); ++i)
	{
		// We have a 0
		if (ipv6[i] == 0)
		{
			// Start a new sequence at this position if not already started
			if (!currentZeroSeq.has_value())
			{
				currentZeroSeq = { i, 0u };
			}
			// Increment the sequence length
			currentZeroSeq->second++;
		}
		// We don't have a 0
		else
		{
			// If we have an active sequence, check if it's (strictly) the longest
			if (currentZeroSeq.has_value() && (!longestZeroSeq.has_value() || currentZeroSeq->second > longestZeroSeq->second))
			{
				longestZeroSeq = currentZeroSeq;
			}
			currentZeroSeq = std::nullopt;
		}
	}
	// Check if the last sequence is the longest
	if (currentZeroSeq.has_value() && (!longestZeroSeq.has_value() || currentZeroSeq->second > longestZeroSeq->second))
	{
		longestZeroSeq = currentZeroSeq;
	}
	// If the longest sequence is only 1 in length, it's not a sequence
	if (longestZeroSeq.has_value() && longestZeroSeq->second == 1)
	{
		longestZeroSeq = std::nullopt;
	}

	// Second pass - Print the IPV6 address
	auto mustAppendColon = false;
	std::stringstream ss;
	ss << std::hex;

	for (auto i = 0u; i < ipv6.size(); ++i)
	{
		// Check if we have a sequence at this position
		if (longestZeroSeq.has_value() && i == longestZeroSeq->first)
		{
			ss << "::";
			i += static_cast<decltype(i)>(longestZeroSeq->second - 1);
			mustAppendColon = false; // Reset the flag, we already added the double colon
			continue;
		}

		if (!mustAppendColon)
		{
			mustAppendColon = true;
		}
		else
		{
			ss << ":";
		}

		// Check if we must display the embedded IPV4 (the last 2 elements)
		if (displayAsEmbeddedIPV4 && i == ipv6.size() - 2)
		{
			ss << std::dec;
			ss << (ipv6[6] >> 8) << "." << (ipv6[6] & 0xFF) << "." << (ipv6[7] >> 8) << "." << (ipv6[7] & 0xFF);
			break; // We are done
		}
		// Print the element
		ss << ipv6[i];
	}

	return ss.str();
}
#if defined(__GNUC__) && !defined(__clang__)
#	pragma GCC diagnostic pop
#endif

void IPAddress::buildIPString() noexcept
{
	std::string ip;

	switch (_type)
	{
		case Type::V4:
		{
			ip = buildIPV4String(_ipv4);
			break;
		}
		case Type::V6:
		{
			auto const packedIP = pack(_ipv6);
			auto const displayAsEmbeddedIPV4 = isEmbeddedIPv4Compatible(packedIP) || isEmbeddedIPv4Mapped(packedIP);
			ip = buildIPV6String(_ipv6, packedIP, displayAsEmbeddedIPV4);
			break;
		}
		default:
			ip = "Invalid IPAddress";
			break;
	}

	auto len = ip.size();
	if (len >= _ipString.size())
	{
		assert(len < _ipString.size() && "String length greater than IPStringMaxLength");
		len = _ipString.size() - 1;
		ip.resize(len);
	}
	std::copy(ip.begin(), ip.end(), _ipString.begin());
	_ipString[len] = 0;
}


} // namespace networkInterface
} // namespace la
