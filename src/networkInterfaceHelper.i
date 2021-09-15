%module(directors="1") la_networkInterfaceHelper

// Optimize code generation be enabling RVO
%typemap(out, optimal="1") SWIGTYPE
%{
    $result = new $1_ltype((const $1_ltype &)$1);
%}

%include "la/networkInterfaceHelper/networkInterfaceHelper.i"
