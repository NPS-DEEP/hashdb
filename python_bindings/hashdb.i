%module (docstring="The hashdb module provides interfaces to the hashdb block hash database.") hashdb
%include "std_string.i"
%include "stdint.i"
%include "std_set.i"
%include "std_pair.i"

%{
#include "hashdb.hpp"
%}

%feature("autodoc", "1");

%include "hashdb.hpp"
