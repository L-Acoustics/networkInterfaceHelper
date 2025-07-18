////////////////////////////////////////
// Network Interface Helper SWIG file
////////////////////////////////////////

%module(directors="1") la_networkInterfaceHelper

// C# Specifics
#if defined(SWIGCSHARP)
// Optimize code generation by enabling RVO
%typemap(out, optimal="1") SWIGTYPE
%{
	$result = new $1_ltype($1);
%}
#pragma SWIG nowarn=474
// Expose internal constructor and methods publicly, some dependant modules may need it
#	if !defined(SWIGIMPORTED)
#	define PUBLIC_BUT_HIDDEN [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)] public
	SWIG_CSBODY_PROXY(PUBLIC_BUT_HIDDEN, PUBLIC_BUT_HIDDEN, SWIGTYPE)
#	endif
#endif

// Common for all languages
// Use 64-bit size_t
#if defined(USE_SIZE_T_64)
%apply unsigned long long { size_t };
%apply const unsigned long long & { const size_t & };
#endif

// Include some SWIG typemaps
%include <stl.i>
%include <std_string.i>
%include <std_pair.i>
%include <stdint.i>
%include <std_array.i>
%include <std_vector.i>
%include <windows.i>
#ifdef SWIGCSHARP
%include <arrays_csharp.i>
#endif

// Generated wrapper file needs to include our header file (include as soon as possible using 'insert(runtime)' as target language exceptions are defined early in the generated wrapper file)
%insert(runtime) %{
	#include <la/networkInterfaceHelper/networkInterfaceHelper.hpp>
%}

#if defined(SWIGCSHARP)
// Marshal all std::string as UTF8Str
%typemap(imtype, outattributes="[return: System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPUTF8Str)]", inattributes="[System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPUTF8Str)] ") std::string, std::string const& "string"
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
%ignore la::networkInterface::IPAddress::operator value_type_packed_v6; // Ignore value_type_packed_v6 operator (equivalent to getIPV6Packed)
%ignore la::networkInterface::IPAddress::CompatibleV6; // Ignore CompatibleV6 (not needed)
%ignore la::networkInterface::IPAddress::MappedV6; // Ignore MappedV6 (not needed)
%ignore la::networkInterface::IPAddress::hash; // Ignore hash (not needed)
%rename("toString") la::networkInterface::IPAddress::operator std::string;
%typemap(csattributes) la::networkInterface::IPAddress "[System.Diagnostics.DebuggerDisplay(\"{toString()}\")]" // Better debug display
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
%csmethodmodifiers ToString "public override"; // Force override of object.ToString()
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
%template(IPAddressPackedV6) std::pair<std::uint64_t, std::uint64_t>;

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
%feature("director") la::networkInterface::NetworkInterfaceHelper::DefaultedObserver;

%include "la/networkInterfaceHelper/networkInterfaceHelper.hpp"
