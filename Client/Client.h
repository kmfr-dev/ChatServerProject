#pragma once

#include "DefineHeaders.h"

class CClient
{
private:
	CClient() {};
	~CClient() {};

public:
	void Start();
	void End();

public:
	void SetName(const char* _NewName) { mName = _NewName; }

public:
	const char* GetName_to_Cstr() const { return mName.c_str(); }
	const std::string& GetName() const { return mName; }

private:
	SOCKET mConnectedSocket = {};
	std::string mName = "";
};