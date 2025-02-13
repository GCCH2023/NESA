#pragma once
#include <iostream>
#include <string>

#ifdef UNICODE
using OStream = std::wostream;
#define COUT std::wcout
using String = std::wstring;
#else
using OStream = std::ostream;
#define COUT std::cout
using String = std::string;
#endif
