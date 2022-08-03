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
* @file helper_win32.cpp
* @author Christophe Calmejane
*/

#include "la/networkInterfaceHelper/windowsHelper.hpp"

#include <Windows.h>

#include <stdexcept> // invalid_argument

namespace la
{
namespace networkInterface
{
namespace windows
{
std::wstring utf8ToWideChar(std::string const& str)
{
	auto const sizeHint = str.size(); // WideChar size cannot exceed the number of multi-bytes
	auto result = std::wstring(static_cast<std::wstring::size_type>(sizeHint), std::wstring::value_type{ 0 }); // Brace-initialization constructor prevents the use of {}

	// Try to convert
	auto const convertedLength = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), static_cast<int>(sizeHint));
	if (convertedLength == 0 || static_cast<size_t>(convertedLength) > sizeHint)
	{
		throw std::invalid_argument("Failed to convert from MultiByte to WideChar");
	}

	// Adjust size
	result.resize(convertedLength);

	return result;
}

std::string wideCharToUtf8(std::wstring const& str, size_t const sizeHint)
{
	auto const maxConvertedBytes = sizeHint == 0 ? str.size() * 4 : sizeHint; // 4 MultiBytes "character" per WideChar should be enough, otherwise use the provided sizeHint
	auto result = std::string(static_cast<std::string::size_type>(maxConvertedBytes), std::string::value_type{ 0 }); // Brace-initialization constructor prevents the use of {}

	// Try to convert
	auto const convertedLength = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), static_cast<int>(result.size()), nullptr, nullptr);
	if (convertedLength == 0 || static_cast<size_t>(convertedLength) > maxConvertedBytes)
	{
		throw std::invalid_argument("Failed to convert from WideChar to MultiByte");
	}

	// Adjust size
	result.resize(convertedLength);

	return result;
}

} // namespace windows
} // namespace networkInterface
} // namespace la
