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

#include "dAudio.h"

// Stub AIDL backend: no Audio AIDL service exists yet; all methods return unavailable.
class dAudioAIDLImpl : public hal::dAudio::IPlatform {

    dAudioAIDLImpl(const dAudioAIDLImpl&) = delete;
    dAudioAIDLImpl& operator=(const dAudioAIDLImpl&) = delete;

public:
    dAudioAIDLImpl() {}
    virtual ~dAudioAIDLImpl() {}

    void InitialiseHAL() {}
    void DeInitialiseHAL() {}

    void setAllCallbacks(const CallbackBundle bundle) override {}
    void getPersistenceValue() override {}

    uint32_t GetAudioPort(const AudioPortType type, const int32_t index, int32_t& handle) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioCapabilities(const int32_t handle, int32_t& capabilities) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioMS12Capabilities(const int32_t handle, int32_t& capabilities) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioFormat(const int32_t handle, AudioFormat& audioFormat) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioEncoding(const int32_t handle, AudioEncoding& encoding) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetSupportedCompressions(const int32_t handle, IDeviceSettingsAudioCompressionIterator*& compressions) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioCompression(const int32_t handle, AudioCompression& compression) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioCompression(const int32_t handle, const AudioCompression compression) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioLevel(const int32_t handle, const float audioLevel) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioLevel(const int32_t handle, float& audioLevel) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioGain(const int32_t handle, const float gainLevel) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioGain(const int32_t handle, float& gainLevel) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioMute(const int32_t handle, const bool mute) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t IsAudioMuted(const int32_t handle, bool& muted) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioDucking(const int32_t handle, const AudioDuckingType duckingType, const AudioDuckingAction duckingAction, const uint8_t level) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetStereoMode(const int32_t handle, AudioStereoMode& mode) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetStereoMode(const int32_t handle, const AudioStereoMode mode, const bool persist) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAssociatedAudioMixing(const int32_t handle, const bool mixing) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAssociatedAudioMixing(const int32_t handle, bool& mixing) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioFaderControl(const int32_t handle, const int32_t mixerBalance) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioFaderControl(const int32_t handle, int32_t& mixerBalance) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioPrimaryLanguage(const int32_t handle, const std::string& primaryAudioLanguage) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioPrimaryLanguage(const int32_t handle, std::string& primaryAudioLanguage) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioSecondaryLanguage(const int32_t handle, const std::string& secondaryAudioLanguage) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioSecondaryLanguage(const int32_t handle, std::string& secondaryAudioLanguage) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t IsAudioOutputConnected(const int32_t handle, bool& isConnected) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioSinkDeviceAtmosCapability(const int32_t handle, DolbyAtmosCapability& atmosCapability) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioAtmosOutputMode(const int32_t handle, const bool enable) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t IsAudioPortEnabled(const int32_t handle, bool& enabled) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t EnableAudioPort(const int32_t handle, const bool enable) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetSupportedARCTypes(const int32_t handle, int32_t& types) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetSAD(const int32_t handle, const uint8_t sadList[], const uint8_t count) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t EnableARC(const int32_t handle, const WPEFramework::Exchange::IDeviceSettingsAudio::AudioARCStatus arcStatus) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioEnablePersist(const int32_t handle, bool& enabled, std::string& portName) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioEnablePersist(const int32_t handle, const bool enable, const std::string portName) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t IsAudioMSDecoded(const int32_t handle, bool& hasms11Decode) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t IsAudioMS12Decoded(const int32_t handle, bool& hasms12Decode) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioLEConfig(const int32_t handle, bool& enabled) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t EnableAudioLEConfig(const int32_t handle, const bool enable) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioDelay(const int32_t handle, const uint32_t audioDelay) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioDelay(const int32_t handle, uint32_t& audioDelay) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioDelayOffset(const int32_t handle, const uint32_t delayOffset) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioDelayOffset(const int32_t handle, uint32_t& delayOffset) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioCompression(const int32_t handle, const int32_t compressionLevel) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioCompression(const int32_t handle, int32_t& compressionLevel) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioDialogEnhancement(const int32_t handle, const int32_t level) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioDialogEnhancement(const int32_t handle, int32_t& level) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioDolbyVolumeMode(const int32_t handle, const bool enable) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioDolbyVolumeMode(const int32_t handle, bool& enabled) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioIntelligentEqualizerMode(const int32_t handle, const int32_t mode) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioIntelligentEqualizerMode(const int32_t handle, int32_t& mode) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioVolumeLeveller(const int32_t handle, const WPEFramework::Exchange::IDeviceSettingsAudio::VolumeLeveller volumeLeveller) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioVolumeLeveller(const int32_t handle, WPEFramework::Exchange::IDeviceSettingsAudio::VolumeLeveller& volumeLeveller) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioBassEnhancer(const int32_t handle, const int32_t boost) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioBassEnhancer(const int32_t handle, int32_t& boost) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t EnableAudioSurroudDecoder(const int32_t handle, const bool enable) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t IsAudioSurroudDecoderEnabled(const int32_t handle, bool& enabled) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioDRCMode(const int32_t handle, const int32_t drcMode) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioDRCMode(const int32_t handle, int32_t& drcMode) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioSurroudVirtualizer(const int32_t handle, const WPEFramework::Exchange::IDeviceSettingsAudio::SurroundVirtualizer surroundVirtualizer) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioSurroudVirtualizer(const int32_t handle, WPEFramework::Exchange::IDeviceSettingsAudio::SurroundVirtualizer& surroundVirtualizer) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioMISteering(const int32_t handle, const bool enable) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioMISteering(const int32_t handle, bool& enable) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioGraphicEqualizerMode(const int32_t handle, const int32_t mode) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioGraphicEqualizerMode(const int32_t handle, int32_t& mode) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioMS12ProfileList(const int32_t handle, WPEFramework::Exchange::IDeviceSettingsAudio::IDeviceSettingsAudioMS12AudioProfileIterator*& ms12ProfileList) const override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioMS12Profile(const int32_t handle, std::string& profile) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioMS12Profile(const int32_t handle, const std::string& profile) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioMixerLevels(const int32_t handle, const WPEFramework::Exchange::IDeviceSettingsAudio::AudioInput audioInput, const int32_t volume) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetAudioMS12SettingsOverride(const int32_t handle, const std::string profileName, const std::string profileSettingsName, const std::string profileSettingValue, const std::string profileState) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t ResetAudioDialogEnhancement(const int32_t handle) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t ResetAudioBassEnhancer(const int32_t handle) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t ResetAudioSurroundVirtualizer(const int32_t handle) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t ResetAudioVolumeLeveller(const int32_t handle) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetAudioHDMIARCPortId(const int32_t handle, int32_t& portId) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetStereoAuto(const int32_t handle, int32_t& mode) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetStereoAuto(const int32_t handle, const int32_t mode, const bool persist) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }

    static bool IsAIDLAvailable() { return false; }
};
