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
* @file windowsHelper.hpp
* @author Christophe Calmejane
* @brief Windows specific helper.
*/

#pragma once

#include <string>

#if defined(_WIN32)
#	if !defined(_NATIVE_WCHAR_T_DEFINED)
#		error "NetworkInterfaceHelper requires _NATIVE_WCHAR_T_DEFINED to be defined"
#	endif // _NATIVE_WCHAR_T_DEFINED
#endif // _WIN32

namespace la
{
namespace networkInterface
{
#if defined(_WIN32)
namespace windows
{
/** Converts a string from UTF8 encoding to WideChar encoding. Throws std::invalid_argument if conversion cannot be achieved. */
std::wstring utf8ToWideChar(std::string const& str);

/** Converts a string from WideChar encoding to UTF8 encoding. You may provide a size hint for conversion or 0 for automatic). Throws std::invalid_argument if conversion cannot be achieved. */
std::string wideCharToUtf8(std::wstring const& str, size_t const sizeHint = 0);

} // namespace windows
#endif // _WIN32

} // namespace networkInterface
} // namespace la