#pragma once
#include <string>
#include <vector>
//-----------------------------------------------------------------------------------------------
typedef std::vector<std::string> Strings;

//-----------------------------------------------------------------------------------------------

const std::string	Stringf( char const* format, ... );
const std::string	Stringf( int maxLength, char const* format, ... );

const Strings		SplitStringOnDelimiter(std::string const& originalStrings, char delimiterSplitOn, bool trimTrailingSpace = false, bool trimLeadingSpace = false);
void				SplitStringOnDelimiter(std::string const& originalStrings, Strings& out_strings, char delimiterSplitOn = ',', bool trimTrailingSpace = false, bool trimLeadingSpace = false);
