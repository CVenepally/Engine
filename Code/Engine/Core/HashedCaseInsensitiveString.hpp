#pragma once
#include <string>
#include <mutex>

struct HashedCaseInsensitiveString // HCIS
{
private:
	std::string		m_caseIntactText;
	unsigned int	m_lowerCaseHash = 0;
	mutable std::recursive_mutex m_hcisMutex;

public:
	HashedCaseInsensitiveString();
	HashedCaseInsensitiveString(HashedCaseInsensitiveString const& hcisToCopyFrom);
	HashedCaseInsensitiveString(char const* text);
	HashedCaseInsensitiveString(std::string const& text);
	
	unsigned int		GetHash() const				{ return m_lowerCaseHash; }
	std::string const&	GetOriginalString() const	{ return m_caseIntactText; }
	char const*			c_str() const				{ return m_caseIntactText.c_str(); }
	unsigned int		HashForText(char const* text) const;
// 	unsigned int		HashForText(std::string const& text) const;

	bool operator<(HashedCaseInsensitiveString const& comapre) const;
	bool operator==(HashedCaseInsensitiveString const& comapre) const;
	bool operator!=(HashedCaseInsensitiveString const& comapre) const;
	bool operator==(std::string const& comapre) const;
	bool operator!=(std::string const& comapre) const;
	bool operator==(char const* comapre) const;
	bool operator!=(char const* comapre) const;
	void operator=(HashedCaseInsensitiveString const& comapre);
	void operator=(std::string const& comapre);
	void operator=(char const* comapre);

};