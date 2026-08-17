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

#include "dVideoDevice.h"

// Stub AIDL backend: no VideoDevice AIDL service exists yet; all methods return unavailable.
class dVideoDeviceAIDLImpl : public hal::dVideoDevice::IPlatform {

    dVideoDeviceAIDLImpl(const dVideoDeviceAIDLImpl&) = delete;
    dVideoDeviceAIDLImpl& operator=(const dVideoDeviceAIDLImpl&) = delete;

public:
    dVideoDeviceAIDLImpl() {}
    virtual ~dVideoDeviceAIDLImpl() {}

    void InitialiseHAL() {}
    void DeInitialiseHAL() {}

    void setAllCallbacks(const CallbackBundle& bundle) override {}
    void getPersistenceValue() override {}

    uint32_t GetVideoDeviceHandle(const int32_t index, int32_t& handle) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetVideoDeviceDFC(const int32_t handle, const VideoDeviceZoom zoomSetting) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetVideoDeviceDFC(const int32_t handle, VideoDeviceZoom& zoomSetting) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetHDRCapabilities(const int32_t handle, int32_t& capabilities) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetSupportedVideoCodingFormats(const int32_t handle, int32_t& supportedFormats) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetCodecInfo(const int32_t handle, const VideoDeviceCodec videoCodec, IDeviceSettingsVideoCodecProfileSupportIterator*& codecInfo) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t DisableHDR(const int32_t handle, const bool disable) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetFRFMode(const int32_t handle, const int32_t frfmode) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetFRFMode(const int32_t handle, int32_t& frfmode) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetCurrentDisplayFrameRate(const int32_t handle, string& framerate) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetDisplayFrameRate(const int32_t handle, const string framerate) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }

    static bool IsAIDLAvailable() { return false; }
};
