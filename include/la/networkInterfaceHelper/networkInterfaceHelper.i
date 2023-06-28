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

// C# Specifics
#if defined(SWIGCSHARP)
// Optimize code generation by enabling RVO
%typemap(out, optimal="1") SWIGTYPE
%{
    $result = new $1_ltype(($1_ltype const&)$1);
%}
// Marshal all std::string as UTF8Str
%typemap(imtype, outattributes="[return: System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPUTF8Str)]", inattributes="[System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPUTF8Str)] ") std::string, std::string const& "string"
#endif

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
// Add Clone() method to MacAddress
%typemap(csbase) std::array<std::uint8_t, 6> "global::System.ICloneable";
%typemap(cscode) std::array<std::uint8_t, 6> %{
	public object Clone()
	{
		var copy = new $csclassname();
        int i = 0;
		foreach (byte elem in this)
		{
			copy[i++] = elem;
		}
		return copy;
	}
%}
// Add Clone() method to la::networkInterface::Interface
%typemap(csbase) la::networkInterface::Interface "global::System.ICloneable";
%typemap(cscode) la::networkInterface::Interface %{
	public object Clone()
	{
		var copy = new $csclassname();
		copy.id = this.id;
		copy.description = this.description;
		copy.alias = this.alias;
		copy.macAddress = (MacAddress)this.macAddress.Clone();
		//copy.ipAddressInfos = this.ipAddressInfos.Clone(); // Not implemented yet
		//copy.gateways = this.gateways.Clone(); // Not implemented yet
		copy.type = this.type;
		copy.isEnabled = this.isEnabled;
		copy.isConnected = this.isConnected;
		copy.isVirtual = this.isVirtual;
		return copy;
	}
%}

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
