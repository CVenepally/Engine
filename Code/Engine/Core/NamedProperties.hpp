#pragma once
#include <string>
#include <map>
#include "Engine/Core/HashedCaseInsensitiveString.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class TypedPropertyBase
{
public:
	virtual ~TypedPropertyBase() {}
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
template<typename T>
class TypedProperty : public TypedPropertyBase
{
public:
	explicit TypedProperty(T const& data) : m_data(data) {}
	T m_data;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class NamedProperties
{
public:
	~NamedProperties();

	template<typename T>
	void SetValue(std::string const& keyName, T const& newValue);
	void SetValue(std::string const& keyName, char const* newValue);

	template<typename T>
	T GetValue(std::string const& keyName, T defaultValue) const;

public:
	std::map<HashedCaseInsensitiveString, TypedPropertyBase*> m_keyValuePairs; 
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
// IMPLEMENTATION
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
NamedProperties::~NamedProperties()
{
	for(auto iter = m_keyValuePairs.begin(); iter != m_keyValuePairs.end(); ++iter)
	{
		if(iter->second)
		{
			delete iter->second;
			iter->second = nullptr;
		}
	}

	m_keyValuePairs.clear();

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
template<typename T>
void NamedProperties::SetValue(std::string const& keyName, T const& newValue)
{
	TypedProperty<T>* prop = new TypedProperty<T>(newValue);
	HashedCaseInsensitiveString hcis = HashedCaseInsensitiveString(keyName);
	m_keyValuePairs[hcis] = prop;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void NamedProperties::SetValue(std::string const& keyName, char const* newValue)
{
	TypedProperty<std::string>* prop = new TypedProperty<std::string>(newValue);
	HashedCaseInsensitiveString hcis = HashedCaseInsensitiveString(keyName);
	m_keyValuePairs[hcis] = prop;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
template<typename T>
inline T NamedProperties::GetValue(std::string const& keyName, T defaultValue) const
{
	HashedCaseInsensitiveString hcis = HashedCaseInsensitiveString(keyName);
	auto result = m_keyValuePairs.find(hcis);
	if(result == m_keyValuePairs.end())
	{
		return defaultValue;
	}

	// attempt a cast to the return types TP
	auto returnVal = dynamic_cast<TypedProperty<T>*>(result->second);
	if(returnVal)
	{
		return returnVal->m_data;
	}

	return defaultValue;
}