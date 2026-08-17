/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "dHost.h"

// Stub AIDL backend: no Host AIDL service exists yet; all methods return unavailable.
class dHostAIDLImpl : public hal::dHost::IPlatform {

    dHostAIDLImpl(const dHostAIDLImpl&) = delete;
    dHostAIDLImpl& operator=(const dHostAIDLImpl&) = delete;

public:
    dHostAIDLImpl() {}
    virtual ~dHostAIDLImpl() {}

    void InitialiseHAL() {}
    void DeInitialiseHAL() {}

    void setAllCallbacks(const CallbackBundle& bundle) override {}
    void getPersistenceValue() override {}

    uint32_t GetEDID(uint8_t edId[], const uint16_t edIdLength) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetMS12ConfigType(string& ms12Config) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }

    static bool IsAIDLAvailable() { return false; }
};
