////////////////////////////////////////
// Network Interface Helper SWIG file
////////////////////////////////////////

%module(directors="1") la_networkInterfaceHelper

%include <stl.i>
%include <std_string.i>
%include <stdint.i>
%include <std_array.i>
%include <std_vector.i>
%include <windows.i>
#ifdef SWIGCSHARP
%include <arrays_csharp.i>
#endif

// Generated wrapper file needs to include our header file
%{
		#include <la/networkInterfaceHelper/networkInterfaceHelper.hpp>
%}

#if defined(USE_SIZE_T_64)
// Use 64-bit size_t
%apply unsigned long long { size_t };
%apply const unsigned long long & { const size_t & };
#endif

// C# Specifics
#if defined(SWIGCSHARP)
// Optimize code generation by enabling RVO
%typemap(out, optimal="1") SWIGTYPE
%{
		$result = new $1_ltype(($1_ltype const&)$1);
%}
// Marshal all std::string as UTF8Str
%typemap(imtype, outattributes="[return: System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPUTF8Str)]", inattributes="[System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPUTF8Str)] ") std::string, std::string const& "string"
// Better debug display
%typemap(csattributes) la::networkInterface::IPAddress "[System.Diagnostics.DebuggerDisplay(\"{toString()}\")]"
#endif

////////////////////////////////////////
// Global functions
////////////////////////////////////////
//%nspace la::networkInterface::getLibraryVersion; // Put into moduleName.functionName https://github.com/swig/swig/issues/591
//%nspace la::networkInterface::getLibraryName; // Put into moduleName.functionName https://github.com/swig/swig/issues/591
//%nspace la::networkInterface::getLibraryCopyright; // Put into moduleName.functionName https://github.com/swig/swig/issues/591

////////////////////////////////////////
// IPAddress
////////////////////////////////////////
%nspace la::networkInterface::IPAddress;
%ignore la::networkInterface::IPAddress::IPAddress(IPAddress&&); // Ignore move constructor
%ignore la::networkInterface::IPAddress::operator bool; // Ignore bool operator (equivalent to isValid)
%ignore la::networkInterface::IPAddress::operator value_type_v4; // Ignore value_type_v4 operator (equivalent to getIPV4)
%ignore la::networkInterface::IPAddress::operator value_type_v6; // Ignore value_type_v6 operator (equivalent to getIPV6)
%ignore la::networkInterface::IPAddress::operator value_type_packed_v4; // Ignore value_type_packed_v4 operator (equivalent to getIPV4Packed)
%ignore la::networkInterface::IPAddress::hash; // Ignore hash (not needed)
%rename("toString") la::networkInterface::IPAddress::operator std::string;
%ignore operator++(IPAddress& lhs); // Redefined in %extend
%ignore operator--(IPAddress& lhs); // Redefined in %extend
%ignore operator&(IPAddress const& lhs, IPAddress const& rhs); // Redefined in %extend
%ignore operator|(IPAddress const& lhs, IPAddress const& rhs); // Redefined in %extend
// Extend the class
%extend la::networkInterface::IPAddress
{
		IPAddress& increment()
		{
				++(*$self);
				return *$self;
		}
		IPAddress& decrement()
		{
				--(*$self);
				return *$self;
		}
		static IPAddress Add(IPAddress const& lhs, std::uint32_t const value)
		{
				return lhs + value;
		}
		static IPAddress Sub(IPAddress const& lhs, std::uint32_t const value)
		{
				return lhs - value;
		}
		static IPAddress And(IPAddress const& lhs, IPAddress const& rhs)
		{
				return lhs & rhs;
		}
		static IPAddress Or(IPAddress const& lhs, IPAddress const& rhs)
		{
				return lhs | rhs;
		}
#if defined(SWIGCSHARP)
	// Provide a more native ToString() method
	std::string ToString() const noexcept
	{
		return static_cast<std::string>(*$self);
	}
	// Provide a more native Equals() method
	bool Equals(la::networkInterface::IPAddress const& other) const noexcept
	{
		return *$self == other;
	}
#endif
};
// Enable some templates
%template(IPAddressV4) std::array<std::uint8_t, 4>;
%template(IPAddressV6) std::array<std::uint16_t, 8>;

////////////////////////////////////////
// IPAddressInfo
////////////////////////////////////////
%nspace la::networkInterface::IPAddressInfo;

////////////////////////////////////////
// Interface
////////////////////////////////////////
%nspace la::networkInterface::Interface;
// Extend the struct
%extend la::networkInterface::Interface
{
	// Add default constructor
	Interface()
	{
		return new la::networkInterface::Interface();
	}
	// Add a copy-constructor
	Interface(la::networkInterface::Interface const& other)
	{
		return new la::networkInterface::Interface(other);
	}
#if defined(SWIGCSHARP)
	// Provide a more native Equals() method
	bool Equals(la::networkInterface::Interface const& other) const noexcept
	{
		return $self->id == other.id && $self->description == other.description && $self->alias == other.alias && $self->macAddress == other.macAddress && $self->ipAddressInfos == other.ipAddressInfos && $self->gateways == other.gateways && $self->type == other.type && $self->isEnabled == other.isEnabled && $self->isConnected == other.isConnected && $self->isVirtual == other.isVirtual;
	}
#endif
};

// Enable some templates
%template(IPAddressInfos) std::vector<la::networkInterface::IPAddressInfo>;
%template(Gateways) std::vector<la::networkInterface::IPAddress>;
%template(MacAddress) std::array<std::uint8_t, 6>;

// Ignore MacAddressHash
%ignore la::networkInterface::MacAddressHash;

////////////////////////////////////////
// NetworkInterfaceHelper
////////////////////////////////////////
%nspace la::networkInterface::NetworkInterfaceHelper;
%nspace la::networkInterface::NetworkInterfaceHelper::Observer;
%ignore la::networkInterface::NetworkInterfaceHelper::enumerateInterfaces; // Disable this method, use Observer instead
%feature("director") la::networkInterface::NetworkInterfaceHelper::Observer;

#define final // Final keyword not properly parsed by SWIG when used on a class
%include "la/networkInterfaceHelper/networkInterfaceHelper.hpp"
#undef final
