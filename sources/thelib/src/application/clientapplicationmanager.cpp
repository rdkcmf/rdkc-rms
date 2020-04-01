/**
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2019 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
**/



#include "application/clientapplicationmanager.h"
#include "application/baseclientapplication.h"
#include "eventlogger/eventlogger.h"

map<uint32_t, BaseClientApplication *> ClientApplicationManager::_applicationsById;
map<string, BaseClientApplication *> ClientApplicationManager::_applicationsByName;
BaseClientApplication *ClientApplicationManager::_pDefaultApplication = NULL;

void ClientApplicationManager::Shutdown() {

	FOR_MAP(_applicationsById, uint32_t, BaseClientApplication *, i) {
		MAP_VAL(i)->GetEventLogger()->LogApplicationStop(MAP_VAL(i));
		delete MAP_VAL(i);
	}
	_applicationsById.clear();
	_applicationsByName.clear();
	_pDefaultApplication = NULL;
}

bool ClientApplicationManager::RegisterApplication(BaseClientApplication* pClientApplication) {
	if (pClientApplication == NULL) {
		FATAL("Try to register a NULL application");
		return false;
	}
	if (MAP_HAS1(_applicationsById, pClientApplication->GetId())) {
		FATAL("Client application with id %u already registered",
				pClientApplication->GetId());
		return false;
	}
	if (MAP_HAS1(_applicationsByName, pClientApplication->GetName())) {
		FATAL("Client application with name `%s` already registered",
				STR(pClientApplication->GetName()));
		return false;
	}

	vector<string> aliases = pClientApplication->GetAliases();

	FOR_VECTOR_ITERATOR(string, aliases, i) {
		if (MAP_HAS1(_applicationsByName, VECTOR_VAL(i))) {
			FATAL("Client application with alias `%s` already registered",
					STR(VECTOR_VAL(i)));
			return false;
		}
	}
	_applicationsById[pClientApplication->GetId()] = pClientApplication;
	_applicationsByName[pClientApplication->GetName()] = pClientApplication;

	FOR_VECTOR_ITERATOR(string, aliases, i) {
		_applicationsByName[VECTOR_VAL(i)] = pClientApplication;
	}

	if (pClientApplication->IsDefault())
		_pDefaultApplication = pClientApplication;
	return true;
}

void ClientApplicationManager::UnRegisterApplication(BaseClientApplication* pClientApplication) {
	if (pClientApplication == NULL) {
		WARN("Try to unregister a NULL application");
		return;
	}
	if (MAP_HAS1(_applicationsById, pClientApplication->GetId()))
		_applicationsById.erase(pClientApplication->GetId());
	if (MAP_HAS1(_applicationsByName, pClientApplication->GetName()))
		_applicationsByName.erase(pClientApplication->GetName());

	vector<string> aliases = pClientApplication->GetAliases();

	for (uint32_t i = 0; i < aliases.size(); i++) {
		if (MAP_HAS1(_applicationsByName, aliases[i]))
			_applicationsByName.erase(aliases[i]);
	}

	if (_pDefaultApplication != NULL) {
		if (_pDefaultApplication->GetId() == pClientApplication->GetId()) {
			_pDefaultApplication = NULL;
		}
	}

	FINEST("Application `%s` (%u) unregistered", STR(pClientApplication->GetName()),
			pClientApplication->GetId());

	pClientApplication->GetEventLogger()->LogApplicationStop(pClientApplication);
}

BaseClientApplication *ClientApplicationManager::GetDefaultApplication() {
	return _pDefaultApplication;
}

BaseClientApplication *ClientApplicationManager::FindAppByName(string appName) {
	if (MAP_HAS1(_applicationsByName, appName))
		return _applicationsByName[appName];
	return NULL;
}

BaseClientApplication *ClientApplicationManager::FindAppById(uint32_t id) {
	if (MAP_HAS1(_applicationsById, id))
		return _applicationsById[id];
	return NULL;
}

map<uint32_t, BaseClientApplication *> ClientApplicationManager::GetAllApplications() {
	return _applicationsById;
}

bool ClientApplicationManager::SendMessageToApplication(string appName, Variant &message) {
	BaseClientApplication *pApp = FindAppByName(appName);
	if (pApp == NULL) {
		WARN("Application %s not found. Message ignored:\n%s", STR(appName), STR(message.ToString()));
		return false;
	}
	pApp->SignalApplicationMessage(message);
	return true;
}

bool ClientApplicationManager::BroadcastMessageToAllApplications(Variant &message) {

	FOR_MAP(_applicationsById, uint32_t, BaseClientApplication *, i) {
		MAP_VAL(i)->SignalApplicationMessage(message);
	}
	return true;
}

void ClientApplicationManager::SignalServerBootstrapped() {

	FOR_MAP(_applicationsById, uint32_t, BaseClientApplication *, i) {
		MAP_VAL(i)->SignalServerBootstrapped();
	}
}
