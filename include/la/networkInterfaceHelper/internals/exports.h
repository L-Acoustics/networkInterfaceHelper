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
* @file exports.h
* @author Christophe Calmejane
* @brief OS specific defines for importing and exporting dynamic symbols.
*/

#pragma once

#ifdef __cplusplus
#	define LA_NIH_CPP_EXPORT extern "C"
#else // !__cplusplus
#	define LA_NIH_CPP_EXPORT
#endif // __cplusplus

#ifdef _WIN32

#	define LA_NIH_BINDINGS_C_CALL_CONVENTION __stdcall

#	if defined(la_networkInterfaceHelper_c_EXPORTS)
#		define LA_NIH_BINDINGS_C_API LA_NIH_CPP_EXPORT __declspec(dllexport)
#	elif defined(la_networkInterfaceHelper_c_static_STATICS)
#		define LA_NIH_BINDINGS_C_API LA_NIH_CPP_EXPORT
#	else
#		define LA_NIH_BINDINGS_C_API LA_NIH_CPP_EXPORT __declspec(dllimport)
#	endif

#else // !_WIN32

#	define LA_NIH_BINDINGS_C_CALL_CONVENTION

#	if defined(la_networkInterfaceHelper_c_EXPORTS)
#		define LA_NIH_BINDINGS_C_API LA_NIH_CPP_EXPORT __attribute__((visibility("default")))
#	elif defined(la_networkInterfaceHelper_c_static_STATICS)
#		define LA_NIH_BINDINGS_C_API LA_NIH_CPP_EXPORT
#	else
#		define LA_NIH_BINDINGS_C_API LA_NIH_CPP_EXPORT __attribute__((visibility("default")))
#	endif

#endif // _WIN32
