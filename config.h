#pragma once
#include <iostream>
#include <string>
#include <boost/flyweight.hpp>

#ifdef UNICODE
using OStream = std::wostream;
#define COUT std::wcout
using StdString = std::wstring;
#else
using OStream = std::ostream;
#define COUT std::cout
using StdString = std::string;
#endif

using CStr = TCHAR*;
using FlyweightString = boost::flyweight<StdString>;
