#include "Engine/Core/HashedCaseInsensitiveString.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
HashedCaseInsensitiveString::HashedCaseInsensitiveString()
	: m_lowerCaseHash(0)
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
HashedCaseInsensitiveString::HashedCaseInsensitiveString(HashedCaseInsensitiveString const& hcisToCopyFrom)
	:m_caseIntactText(hcisToCopyFrom.m_caseIntactText)
	, m_lowerCaseHash(hcisToCopyFrom.m_lowerCaseHash)
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
HashedCaseInsensitiveString::HashedCaseInsensitiveString(char const* text)
	: m_caseIntactText(text)
{
	m_lowerCaseHash = HashForText(text);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
HashedCaseInsensitiveString::HashedCaseInsensitiveString(std::string const& text)
	:m_caseIntactText(text)
{
	m_lowerCaseHash = HashForText(text.c_str());
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int HashedCaseInsensitiveString::HashForText(char const* text) const
{
	std::scoped_lock<std::recursive_mutex> lock(m_hcisMutex);
	unsigned int hash = 0;
	for(char const* scan = text; *scan != '\0'; ++scan)
	{
		hash *= 31;
		hash += static_cast<unsigned int>(tolower(*scan));
	}

	return hash;
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------
// unsigned int HashedCaseInsensitiveString::HashForText(std::string const& text) const
// {
// 	return 0;
// }

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool HashedCaseInsensitiveString::operator<(HashedCaseInsensitiveString const& compare) const
{
	if(m_lowerCaseHash < compare.m_lowerCaseHash)
	{
		return true;
	}
	else if(m_lowerCaseHash > compare.m_lowerCaseHash)
	{
		return false;
	}
	else
	{
		bool isLessThan = _stricmp(m_caseIntactText.c_str(), compare.m_caseIntactText.c_str()) < 0;
		return isLessThan;
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool HashedCaseInsensitiveString::operator==(HashedCaseInsensitiveString const& compare) const
{
	if(m_lowerCaseHash != compare.m_lowerCaseHash)
	{
		return false;
	}
	else
	{
		bool isEqual = _stricmp(m_caseIntactText.c_str(), compare.m_caseIntactText.c_str()) == 0;
		return isEqual;
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool HashedCaseInsensitiveString::operator!=(HashedCaseInsensitiveString const& compare) const
{
	if(m_lowerCaseHash == compare.m_lowerCaseHash)
	{
		return false;
	}
	else
	{
		bool isNotEqual = _stricmp(m_caseIntactText.c_str(), compare.m_caseIntactText.c_str()) != 0;
		return isNotEqual;
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool HashedCaseInsensitiveString::operator==(std::string const& compare) const
{
	unsigned int hashToCompare = HashForText(compare.c_str());

	if(m_lowerCaseHash != hashToCompare)
	{
		return false;
	}
	else
	{
		bool isEqual = _stricmp(m_caseIntactText.c_str(), compare.c_str()) == 0;
		return isEqual;
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool HashedCaseInsensitiveString::operator!=(std::string const& compare) const
{
	unsigned int hashToCompare = HashForText(compare.c_str());

	if(m_lowerCaseHash == hashToCompare)
	{
		return false;
	}
	else
	{
		bool isEqual = _stricmp(m_caseIntactText.c_str(), compare.c_str()) != 0;
		return isEqual;
	}
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool HashedCaseInsensitiveString::operator==(char const* compare) const
{
	unsigned int hashToCompare = HashForText(compare);

	if(m_lowerCaseHash != hashToCompare)
	{
		return false;
	}
	else
	{
		bool isEqual = _stricmp(m_caseIntactText.c_str(), compare) == 0;
		return isEqual;
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool HashedCaseInsensitiveString::operator!=(char const* compare) const
{
	unsigned int hashToCompare = HashForText(compare);

	if(m_lowerCaseHash == hashToCompare)
	{
		return false;
	}
	else
	{
		bool isEqual = _stricmp(m_caseIntactText.c_str(), compare) != 0;
		return isEqual;
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HashedCaseInsensitiveString::operator=(HashedCaseInsensitiveString const& replace)
{
	std::scoped_lock<std::recursive_mutex> lock(m_hcisMutex);
	m_caseIntactText = replace.m_caseIntactText;
	m_lowerCaseHash = replace.m_lowerCaseHash;
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HashedCaseInsensitiveString::operator=(std::string const& replace)
{
	std::scoped_lock<std::recursive_mutex> lock(m_hcisMutex);
	m_caseIntactText = replace;
	m_lowerCaseHash = HashForText(replace.c_str());
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void HashedCaseInsensitiveString::operator=(char const* replace)
{
	std::scoped_lock<std::recursive_mutex> lock(m_hcisMutex);
	m_caseIntactText = replace;
	m_lowerCaseHash = HashForText(replace);
}
