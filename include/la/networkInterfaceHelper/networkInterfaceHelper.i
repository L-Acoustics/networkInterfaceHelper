////////////////////////////////////////
// Network Interface Helper SWIG file
////////////////////////////////////////

%include <stl.i>
%include <std_string.i>
%include <stdint.i>
%include <std_array.i>
%include <windows.i>

// Generated wrapper file needs to include our header file
%{
    #include <la/networkInterfaceHelper/networkInterfaceHelper.hpp>
%}

// IPAddress
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

// Ignore IPAddressInfo
%ignore la::networkInterface::IPAddressInfo;
// Ignore Interface
%ignore la::networkInterface::Interface;
// Ignore MacAddressHash
%ignore la::networkInterface::MacAddressHash;
// Ignore NetworkInterfaceHelper
%ignore la::networkInterface::NetworkInterfaceHelper;

#define final // Final keyword not properly parsed by SWIG when used on a class
%include "la/networkInterfaceHelper/networkInterfaceHelper.hpp"
#undef final
