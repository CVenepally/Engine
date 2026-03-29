#include "Engine/Core/StringUtils.hpp"
#include <stdarg.h>


//-----------------------------------------------------------------------------------------------
constexpr int STRINGF_STACK_LOCAL_TEMP_LENGTH = 2048;


//-----------------------------------------------------------------------------------------------
const std::string Stringf( char const* format, ... )
{
	char textLiteral[ STRINGF_STACK_LOCAL_TEMP_LENGTH ];
	va_list variableArgumentList;
	va_start( variableArgumentList, format );
	vsnprintf_s( textLiteral, STRINGF_STACK_LOCAL_TEMP_LENGTH, _TRUNCATE, format, variableArgumentList );	
	va_end( variableArgumentList );
	textLiteral[ STRINGF_STACK_LOCAL_TEMP_LENGTH - 1 ] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

	return std::string( textLiteral );
}


//-----------------------------------------------------------------------------------------------
const std::string Stringf( int maxLength, char const* format, ... )
{
	char textLiteralSmall[ STRINGF_STACK_LOCAL_TEMP_LENGTH ];
	char* textLiteral = textLiteralSmall;
	if( maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH )
		textLiteral = new char[ maxLength ];

	va_list variableArgumentList;
	va_start( variableArgumentList, format );
	vsnprintf_s( textLiteral, maxLength, _TRUNCATE, format, variableArgumentList );	
	va_end( variableArgumentList );
	textLiteral[ maxLength - 1 ] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

	std::string returnValue( textLiteral );
	if( maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH )
		delete[] textLiteral;

	return returnValue;
}

//------------------------------------------------------------------------------------------------------------------
const Strings SplitStringOnDelimiter(std::string const& originalString, char delimiterSplitOn, bool trimTrailingSpace, bool trimLeadingSpace)
{
	Strings strings;

	strings.reserve(originalString.size() / 10);
	
	int startIndex = 0;
	int endIndex = 0;

	while(((endIndex = static_cast<int>(originalString.find(delimiterSplitOn, startIndex))) != -1))
	{
		int partStartIndex = startIndex;
		int partEndIndex = endIndex;

		if(trimLeadingSpace)
		{
			while(partStartIndex < partEndIndex && originalString[partStartIndex] == ' ')
			{
				partStartIndex += 1;
			}
		}

		if(trimTrailingSpace)
		{
			while(partEndIndex > partStartIndex && originalString[partEndIndex - 1] == ' ')
			{
				partEndIndex -= 1;
			}
		}

		if(partStartIndex < partEndIndex)
		{
			strings.emplace_back(originalString, partStartIndex, partEndIndex - partStartIndex);
		}

		startIndex = endIndex + 1;
	}

	if(startIndex < static_cast<int>(originalString.size()))
	{
		int partStartIndex = startIndex;
		int partEndIndex = static_cast<int>(originalString.size());

		if(trimLeadingSpace)
		{
			while(partStartIndex < partEndIndex && originalString[partStartIndex] == ' ')
			{
				partStartIndex += 1;
			}
		}

		if(trimTrailingSpace)
		{
			while(partEndIndex > partStartIndex && originalString[partEndIndex] == ' ')
			{
				partEndIndex -= 1;
			}
		}

		if(partStartIndex < partEndIndex)
		{
			strings.emplace_back(originalString, partStartIndex, partEndIndex - partStartIndex);
		}
	}
	return strings;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void SplitStringOnDelimiter(std::string const& originalString, Strings& out_strings, char delimiterSplitOn, bool trimTrailingSpace, bool trimLeadingSpace)
{
	out_strings.reserve(originalString.size() / 10);

	int startIndex = 0;
	int endIndex = 0;

	while(((endIndex = static_cast<int>(originalString.find(delimiterSplitOn, startIndex))) != -1))
	{
		int partStartIndex = startIndex;
		int partEndIndex = endIndex;

		if(trimLeadingSpace)
		{
			while(partStartIndex < partEndIndex && originalString[partStartIndex] == ' ')
			{
				partStartIndex += 1;
			}
		}

		if(trimTrailingSpace)
		{
			while(partEndIndex > partStartIndex && originalString[partEndIndex - 1] == ' ')
			{
				partEndIndex -= 1;
			}
		}

		if(partStartIndex < partEndIndex)
		{
			out_strings.emplace_back(originalString, partStartIndex, partEndIndex - partStartIndex);
		}

		startIndex = endIndex + 1;
	}

	if(startIndex < static_cast<int>(originalString.size()))
	{
		int partStartIndex = startIndex;
		int partEndIndex = static_cast<int>(originalString.size());

		if(trimLeadingSpace)
		{
			while(partStartIndex < partEndIndex && originalString[partStartIndex] == ' ')
			{
				partStartIndex += 1;
			}
		}

		if(trimTrailingSpace)
		{
			while(partEndIndex > partStartIndex && originalString[partEndIndex] == ' ')
			{
				partEndIndex -= 1;
			}
		}

		if(partStartIndex < partEndIndex)
		{
			out_strings.emplace_back(originalString, partStartIndex, partEndIndex - partStartIndex);
		}
	}
}

