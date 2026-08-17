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

#include "dVideoPort.h"

// Stub AIDL backend: no VideoPort AIDL service exists yet; all methods return unavailable.
class dVideoPortAIDLImpl : public hal::dVideoPort::IPlatform {

    dVideoPortAIDLImpl(const dVideoPortAIDLImpl&) = delete;
    dVideoPortAIDLImpl& operator=(const dVideoPortAIDLImpl&) = delete;

public:
    dVideoPortAIDLImpl() {}
    virtual ~dVideoPortAIDLImpl() {}

    void InitialiseHAL() {}
    void DeInitialiseHAL() {}

    void setAllCallbacks(const CallbackBundle& bundle) override {}
    void getPersistenceValue() override {}

    uint32_t GetVideoPort(const VideoPortType videoPort, const int32_t index, int32_t& handle) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t IsVideoPortEnabled(const int32_t handle, bool& enabled) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t EnableVideoPort(const int32_t handle, const bool enabled) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t IsVideoPortDisplayConnected(const int32_t handle, bool& connected) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t IsVideoPortActive(const int32_t handle, bool& active) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetVideoPortResolution(const int32_t handle, VideoPortResolution& resolution) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetVideoPortResolution(const int32_t handle, const VideoPortResolution resolution, const bool persist, const bool forceCompatibility) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetColorDepth(const int32_t handle, uint32_t& colorDepth) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetVideoPortColorDepth(const int32_t handle, const uint32_t colorDepth) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetQuantizationRange(const int32_t handle, VideoPortQuantizationRange& quantizationRange) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetVideoPortQuantizationRange(const int32_t handle, const VideoPortQuantizationRange quantizationRange) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetColorSpace(const int32_t handle, VideoPortColorSpace& colorSpace) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetColorSpace(const int32_t handle, const VideoPortColorSpace colorSpace) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetVideoPortFrameRate(const int32_t handle, uint32_t& frameRate) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetVideoPortFrameRate(const int32_t handle, const uint32_t frameRate) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetVideoPortHDCPStatus(const int32_t handle, VideoPortHdcpStatus& hdcpStatus) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetHDCPProtocolVersionOnVideoPort(const int32_t handle, VideoPortHdcpProtocolVersion& hdcpVersion) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetHDCPReceiverProtocolVersionOnVideoPort(const int32_t handle, VideoPortHdcpProtocolVersion& hdcpVersion) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetHDCPCurrentProtocolVersionOnVideoPort(const int32_t handle, VideoPortHdcpProtocolVersion& hdcpVersion) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t EnableHDCPOnVideoPort(const int32_t handle, const bool hdcpEnable, const uint8_t* hdcpKey, const uint16_t hdcpKeySize) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t IsHDCPEnabledOnVideoPort(const int32_t handle, bool& hdcpEnabled) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetTVHDRCapabilities(const int32_t handle, int32_t& capabilities) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetTVSupportedResolutions(const int32_t handle, int32_t& resolutions) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetForceDisable4K(const int32_t handle, const bool disable) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetForceDisable4K(const int32_t handle, bool& disabled) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t IsVideoPortOutputHDR(const int32_t handle, bool& isHDR) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t ResetVideoPortOutputToSDR() override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetHDMIPreference(const int32_t handle, VideoPortHdcpProtocolVersion& hdcpVersion) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetHDMIPreference(const int32_t handle, const VideoPortHdcpProtocolVersion hdcpVersion) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetVideoEOTF(const int32_t handle, HDRStandard& hdrStandard) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetMatrixCoefficients(const int32_t handle, DisplayMatrixCoefficients& matrixCoefficients) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t IsVideoPortDisplaySurround(const int32_t handle, bool& surround) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetVideoPortDisplaySurroundMode(const int32_t handle, VideoPortSurroundMode& surroundMode) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetCurrentOutputSettings(const int32_t handle, DSOutputSettings& outputSettings) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetBackgroundColor(const int32_t handle, const VideoBackgroundColor backgroundColor) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetForceHDRMode(const int32_t handle, const HDRStandard hdrMode) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetColorDepthCapabilities(const int32_t handle, uint32_t& colorDepthCapabilities) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetPreferredColorDepth(const int32_t handle, DisplayColorDepth& colorDepth, const bool persist) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetPreferredColorDepth(const int32_t handle, const DisplayColorDepth colorDepth, const bool persist) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }

    static bool IsAIDLAvailable() { return false; }
};
