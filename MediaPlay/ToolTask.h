#pragma once
#include "Poco/Task.h"
#include "Poco/Event.h"

using Poco::Task;
using Poco::Event;

class CToolTask : public Task
{
public:
	CToolTask(const std::string& name);
	~CToolTask(void);
};

class CParseTask : public CToolTask
{
public:
	CParseTask();
	~CParseTask();

	virtual void runTask();
};


class CPlayTask : public CToolTask
{
public:
	CPlayTask();
	~CPlayTask();

	virtual void runTask();
};

