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

IPAddress::IPAddress(std::string const& ipString)
{
	// TODO: Handle IPV6 later: https://tools.ietf.org/html/rfc5952
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
	setValue(ip);
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

bool IPAddress::isValid() const noexcept
{
	return _type != Type::None;
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

void IPAddress::buildIPString() noexcept
{
	std::string ip;

	switch (_type)
	{
		case Type::V4:
		{
			for (auto const v : _ipv4)
			{
				if (!ip.empty())
				{
					ip.append(".");
				}
				ip.append(std::to_string(v));
			}
			break;
		}
		case Type::V6:
		{
			bool first{ true };
			std::stringstream ss;
			ss << std::hex;

			for (auto const v : _ipv6)
			{
				if (first)
				{
					first = false;
				}
				else
				{
					ss << ":";
				}
				ss << v;
			}

			ip = ss.str();
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
