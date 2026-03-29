#pragma once

#include "Engine/Core/EngineCommon.hpp"

#include <vector>
#include <string>
#include <map>
#include <mutex>

//------------------------------------------------------------------------------------------------------------------
typedef bool (*EventCallbackFunction)(EventArgs& args);

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct EventSubscriptionBase
{
	virtual bool Execute(NamedStrings& args) = 0;
};

//------------------------------------------------------------------------------------------------------------------
struct EventFunctionSubscription : public EventSubscriptionBase
{
	EventFunctionSubscription(EventCallbackFunction functionPtr)
		: m_functionPtr(functionPtr)
	{}

	EventCallbackFunction m_functionPtr = nullptr;

	virtual bool Execute(NamedStrings& args) override
	{
		return m_functionPtr(args);
	}
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
template<typename T>
struct EventObjectMethodSubscription : public EventSubscriptionBase
{
	typedef bool (T::*EventObjectMethodPtr)(NamedStrings& args);

	EventObjectMethodSubscription(T& object, EventObjectMethodPtr method)
	: m_object(object)
	, m_objectMethod(method)
	{

	}

	T* m_object = nullptr;
	EventObjectMethodPtr m_objectMethod = nullptr;

	virtual bool Execute(NamedStrings& args) override
	{
		return (m_object->*m_objectMethod)(args);
	}

};

//------------------------------------------------------------------------------------------------------------------
struct EventSystemConfig
{

};

//------------------------------------------------------------------------------------------------------------------
typedef std::vector<EventSubscriptionBase*> SubscriptionList;

//------------------------------------------------------------------------------------------------------------------
class EventSystem
{
	friend class DevConsole;

public:

	EventSystem(EventSystemConfig const& config);
	~EventSystem();

	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();


	void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction functionPtr);
	void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction functionPtr);

// 	template <typename T>
// 	void SubscribeEventCallbackObjectMethod(std::string const& eventName, T* objectPtr, bool (T::* method)(NamedStrings& args))
// 	{
// 		std::vector<>
// 	}

	void FireEvent(std::string const& eventName, EventArgs& args);
	void FireEvent(std::string const& eventName);


protected:
	EventSystemConfig							m_config;
	std::vector<std::string>					m_eventList;
	std::map<std::string, SubscriptionList>		m_subscriptionListByEventName;
	std::recursive_mutex						m_eventMutex;
};

void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction functionPtr);
void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFunction functionPtr);
void FireEvent(std::string const& eventName, EventArgs& args);
void FireEvent(std::string const& eventName);

