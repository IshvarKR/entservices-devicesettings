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

#include "dCompositeIn.h"

// Stub AIDL backend: no CompositeIn AIDL service exists yet; all methods return unavailable.
class dCompositeInAIDLImpl : public hal::dCompositeIn::IPlatform {

    dCompositeInAIDLImpl(const dCompositeInAIDLImpl&) = delete;
    dCompositeInAIDLImpl& operator=(const dCompositeInAIDLImpl&) = delete;

public:
    dCompositeInAIDLImpl() {}
    virtual ~dCompositeInAIDLImpl() {}

    void InitialiseHAL() {}
    void DeInitialiseHAL() {}

    void setAllCallbacks(const CallbackBundle& bundle) override {}
    void getPersistenceValue() override {}

    uint32_t GetNrOfCompositeInputs(int32_t& nrCompositeInputs) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t GetCompositeInStatus(CompositeInStatus& status) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t SelectCompositeInPort(const CompositeInPort port) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }
    uint32_t ScaleCompositeInVideo(const CompositeInVideoRectangle videoRect) override
        { return WPEFramework::Core::ERROR_UNAVAILABLE; }

    static bool IsAIDLAvailable() { return false; }
};
