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

#include "dFPD.h"

// Stub AIDL backend: no FPD AIDL service exists yet; all methods return unavailable.
class dFPDAIDLImpl : public hal::dFPD::IPlatform {

    dFPDAIDLImpl(const dFPDAIDLImpl&) = delete;
    dFPDAIDLImpl& operator=(const dFPDAIDLImpl&) = delete;

public:
    dFPDAIDLImpl() {}
    virtual ~dFPDAIDLImpl() {}

    void InitialiseHAL() {}
    void DeInitialiseHAL() {}

    uint32_t SetFPDTime(const FPDTimeFormat timeFormat, const uint32_t minutes, const uint32_t seconds) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetFPDScroll(const uint32_t scrollHoldDuration, const uint32_t nHorizontalScrollIterations, const uint32_t nVerticalScrollIterations) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetFPDBlink(const FPDIndicator indicator, const uint32_t blinkDuration, const uint32_t blinkIterations) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetFPDBrightness(const FPDIndicator indicator, const uint32_t brightNess, const bool persist) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetFPDBrightness(const FPDIndicator indicator, uint32_t& brightNess) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetFPDState(const FPDIndicator indicator, const FPDState state) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetFPDState(const FPDIndicator indicator, FPDState& state) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetFPDColor(const FPDIndicator indicator, uint32_t& color) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetFPDColor(const FPDIndicator indicator, const uint32_t color) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetFPDTextBrightness(const FPDTextDisplay textDisplay, const uint32_t brightNess) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetFPDTextBrightness(const FPDTextDisplay textDisplay, uint32_t& brightNess) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t EnableFPDClockDisplay(const bool enable) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetFPDTimeFormat(FPDTimeFormat& fpdTimeFormat) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetFPDTimeFormat(const FPDTimeFormat fpdTimeFormat) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SetFPDMode(const FPDMode fpdMode) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }

    static bool IsAIDLAvailable() { return false; }
};
