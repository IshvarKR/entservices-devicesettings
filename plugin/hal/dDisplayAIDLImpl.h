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

#include "dDisplay.h"

// Stub AIDL backend: no Display AIDL service exists yet; all methods return unavailable.
class dDisplayAIDLImpl : public hal::dDisplay::IPlatform {

    dDisplayAIDLImpl(const dDisplayAIDLImpl&) = delete;
    dDisplayAIDLImpl& operator=(const dDisplayAIDLImpl&) = delete;

public:
    dDisplayAIDLImpl() {}
    virtual ~dDisplayAIDLImpl() {}

    void InitialiseHAL() {}
    void DeInitialiseHAL() {}

    void setAllCallbacks(const CallbackBundle& bundle) override {}
    void getPersistenceValue() override {}

    uint32_t GetConnectedVideoDisplay(const int32_t videoPortHandle, bool& isConnected) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetDisplaySurroundMode(const int32_t videoPortHandle, VideoPortSurroundMode& surroundMode) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetDisplayEDID(const int32_t videoPortHandle, uint8_t edidBytes[], const uint16_t edidBytesLength) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetDisplay(const int32_t type, const int32_t index, int32_t& handle) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetDisplayAspectRatio(const int32_t handle, WPEFramework::Exchange::IDeviceSettingsDisplay::DisplayVideoAspectRatio& aspectRatio) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetDisplayEdid(const int32_t handle, WPEFramework::Exchange::IDeviceSettingsDisplay::DisplayEDID& edId) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetDisplayEdidBytes(const int32_t handle, uint8_t edIdBytes[], const uint16_t edidLength) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAllmEnabled(const int32_t handle, const bool enabled) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAVIContentType(const int32_t handle, const int32_t contentType) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAVIScanInformation(const int32_t handle, const int32_t scanInfo) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }

    static bool IsAIDLAvailable() { return false; }
};
