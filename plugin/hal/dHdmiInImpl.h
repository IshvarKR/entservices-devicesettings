// Out-of-line virtual destructor definition for RTTI/typeinfo
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

#include <cstdint>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>
#include <cstring>

#include "dHdmiIn.h"
#include "dsHdmiIn.h"
#include "dsError.h"
#include "dsHdmiInTypes.h"
#include "dsVideoDeviceTypes.h"
#include "dsUtl.h"
#include "dsTypes.h"

#include <WPEFramework/interfaces/IDeviceSettingsHDMIIn.h>
#include "DeviceSettingsTypes.h"

#include <map>
#include <mutex>

#ifdef LOG_PRI
#undef LOG_PRI
#endif

// Binder/AIDL headers for IHDMIInput path
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <utils/String16.h>
#include <com/rdk/hal/hdmiinput/IHDMIInputManager.h>
#include <com/rdk/hal/hdmiinput/IHDMIInput.h>
#include <com/rdk/hal/hdmiinput/IHDMIInputController.h>
#include <com/rdk/hal/hdmiinput/IHDMIInputControllerListener.h>
#include <com/rdk/hal/hdmiinput/BnHDMIInputControllerListener.h>
#include <com/rdk/hal/hdmiinput/IHDMIInputEventListener.h>
#include <com/rdk/hal/hdmiinput/BnHDMIInputEventListener.h>
#include <com/rdk/hal/hdmiinput/Capabilities.h>
#include <com/rdk/hal/hdmiinput/PlatformCapabilities.h>
#include <com/rdk/hal/hdmiinput/SignalState.h>
#include <com/rdk/hal/hdmiinput/State.h>
#include <com/rdk/hal/hdmiinput/HDCPStatus.h>
#include <com/rdk/hal/hdmiinput/HDCPProtocolVersion.h>
#include <com/rdk/hal/hdmiinput/HDMIVersion.h>

using android::sp;
using android::defaultServiceManager;
using android::interface_cast;
using android::String16;
using android::ProcessState;
using namespace com::rdk::hal::hdmiinput;

static int m_hdmiInInitialized = 0;
static int m_hdmiInPlatInitialized = 0;
static bool isDalsEnabled = 0;
static dsHdmiInCap_t hdmiInCap_gs;
static bool m_edidallmsupport[dsHDMI_IN_PORT_MAX];
static bool m_vrrsupport[dsHDMI_IN_PORT_MAX];
static bool m_hdmiPortVrrCaps[dsHDMI_IN_PORT_MAX];

static tv_hdmi_edid_version_t m_edidversion[dsHDMI_IN_PORT_MAX];

static std::function<void(DeviceSettingsHDMIIn::HDMIInPort, bool)> g_HdmiInHotPlugCallback;
static std::function<void(DeviceSettingsHDMIIn::HDMIInPort, DeviceSettingsHDMIIn::HDMIInSignalStatus)> g_HdmiInSignalStatusCallback;
static std::function<void(DeviceSettingsHDMIIn::HDMIInPort, DeviceSettingsHDMIIn::HDMIVideoPortResolution)> g_HdmiInVideoModeUpdateCallback;
static std::function<void(DeviceSettingsHDMIIn::HDMIInPort, bool)> g_HdmiInAllmStatusCallback;
static std::function<void(DeviceSettingsHDMIIn::HDMIInPort, DeviceSettingsHDMIIn::HDMIInAviContentType)> g_HdmiInAviContentTypeCallback;
static std::function<void(int32_t, int32_t)> g_HdmiInAVLatencyCallback;
static std::function<void(DeviceSettingsHDMIIn::HDMIInPort, DeviceSettingsHDMIIn::HDMIInVRRType)> g_HdmiInVRRStatusCallback;
static std::function<void(DeviceSettingsHDMIIn::HDMIInPort, bool)> g_HdmiInStatusCallback;

// ---- AIDL per-port runtime state ----
struct AidlPortCtx {
    sp<IHDMIInput>                   hdmiInput;
    sp<IHDMIInputController>         controller;
    sp<IHDMIInputControllerListener> ctrlListener;
    sp<IHDMIInputEventListener>      evtListener;
    bool   isOpen{false};
    bool   isStarted{false};
    bool   connected{false};
    int    signalState{-1};
    int    lastVIC{0};
    bool   vrrActive{false};
    double vrrFrameRate{0.0};
};

static sp<IHDMIInputManager>      s_aidlHdmiMgr;
static std::mutex                  s_aidlMutex;
static std::map<int, AidlPortCtx>  s_aidlPorts;
static int                         s_aidlActivePort{-1};
static uint8_t                     s_aidlPortCount{0};
static bool                        s_aidlPortArcCapable[dsHDMI_IN_PORT_MAX] = {};

static sp<IHDMIInputManager> getAidlHdmiMgr()
{
    std::lock_guard<std::mutex> lk(s_aidlMutex);
    if (!s_aidlHdmiMgr) {
        ProcessState::self()->startThreadPool();
        sp<android::IServiceManager> sm = defaultServiceManager();
        if (sm) {
            s_aidlHdmiMgr = interface_cast<IHDMIInputManager>(
                sm->getService(String16(IHDMIInputManager::serviceName().c_str())));
        }
    }
    return s_aidlHdmiMgr;
}

// Map VIC code to dsVideoPortResolution_t for callback conversion.
static void aidlVicToRes(int vic, dsVideoPortResolution_t& res)
{
    memset(&res, 0, sizeof(res));
    switch (vic) {
        case 1: case 2: case 3:
            res.pixelResolution = dsVIDEO_PIXELRES_720x480;
            res.frameRate       = dsVIDEO_FRAMERATE_59dot94; break;
        case 4:
            res.pixelResolution = dsVIDEO_PIXELRES_1280x720;
            res.frameRate       = dsVIDEO_FRAMERATE_59dot94; break;
        case 5:
            res.pixelResolution = dsVIDEO_PIXELRES_1920x1080;
            res.interlaced      = true;
            res.frameRate       = dsVIDEO_FRAMERATE_59dot94; break;
        case 16:
            res.pixelResolution = dsVIDEO_PIXELRES_1920x1080;
            res.frameRate       = dsVIDEO_FRAMERATE_59dot94; break;
        case 17: case 18:
            res.pixelResolution = dsVIDEO_PIXELRES_720x576;
            res.frameRate       = dsVIDEO_FRAMERATE_50; break;
        case 19:
            res.pixelResolution = dsVIDEO_PIXELRES_1280x720;
            res.frameRate       = dsVIDEO_FRAMERATE_50; break;
        case 31:
            res.pixelResolution = dsVIDEO_PIXELRES_1920x1080;
            res.frameRate       = dsVIDEO_FRAMERATE_50; break;
        case 93: case 94: case 95: case 96: case 97:
            res.pixelResolution = dsVIDEO_PIXELRES_3840x2160;
            res.frameRate       = (vic >= 96) ? dsVIDEO_FRAMERATE_50 : dsVIDEO_FRAMERATE_25; break;
        default:
            res.pixelResolution = dsVIDEO_PIXELRES_1920x1080;
            res.frameRate       = dsVIDEO_FRAMERATE_60; break;
    }
}

// Parse AVI InfoFrame bytes to extract content type.
static bool aidlParseAviContentType(const std::vector<uint8_t>& infoFrame, dsAviContentType_t* contentType)
{
    static const uint8_t kAviType = 0x82;
    static const size_t  kMinLen  = 9;
    if (!contentType) return false;
    *contentType = dsAVICONTENT_TYPE_NOT_SIGNALLED;
    if (infoFrame.size() < kMinLen || infoFrame[0] != kAviType) return false;
    uint8_t payloadLen = infoFrame[2];
    if (payloadLen < 5 || infoFrame.size() < (size_t)(4 + payloadLen)) return false;
    if (!(infoFrame[6] & 0x80)) return true;
    switch ((infoFrame[8] >> 4) & 0x03) {
        case 0: *contentType = dsAVICONTENT_TYPE_GRAPHICS; break;
        case 1: *contentType = dsAVICONTENT_TYPE_PHOTO;    break;
        case 2: *contentType = dsAVICONTENT_TYPE_CINEMA;   break;
        case 3: *contentType = dsAVICONTENT_TYPE_GAME;     break;
        default: break;
    }
    return true;
}

// Infer EDID version from raw EDID bytes (HDMI Forum VSB presence → 2.0).
static tv_hdmi_edid_version_t aidlGetEdidVersion(const std::vector<uint8_t>& edidVec)
{
    static const uint8_t kHdr[]         = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
    static const uint8_t kHdmiForumOui[] = {0xD8, 0x5D, 0xC4};
    if (edidVec.size() < 128 || memcmp(edidVec.data(), kHdr, sizeof(kHdr)) != 0)
        return HDMI_EDID_VER_MAX;
    uint8_t extCnt = edidVec[126];
    if ((edidVec.size() / 128) == 0 || extCnt > (edidVec.size() / 128 - 1))
        return HDMI_EDID_VER_MAX;
    for (uint8_t e = 0; e < extCnt; ++e) {
        size_t base = (size_t)(e + 1) * 128;
        if (base + 128 > edidVec.size() || edidVec[base] != 0x02) continue;
        uint8_t dtdOff = edidVec[base + 2];
        if (dtdOff < 4 || dtdOff > 127) return HDMI_EDID_VER_MAX;
        size_t idx = base + 4, end = base + dtdOff;
        while (idx < end) {
            uint8_t tl = edidVec[idx];
            uint8_t tag = (tl >> 5) & 0x07, len = tl & 0x1F;
            if (idx + 1 + len > end) return HDMI_EDID_VER_MAX;
            if (tag == 0x03 && len >= 3 && memcmp(&edidVec[idx + 1], kHdmiForumOui, 3) == 0)
                return HDMI_EDID_VER_20;
            idx += 1 + len;
        }
    }
    return HDMI_EDID_VER_14;
}

static bool aidlMapEdidVersion(tv_hdmi_edid_version_t legacyVer, HDMIVersion* out)
{
    if (!out) return false;
    switch (legacyVer) {
        case HDMI_EDID_VER_14: *out = HDMIVersion::HDMI_1_4; return true;
        case HDMI_EDID_VER_20: *out = HDMIVersion::HDMI_2_0; return true;
        default: return false;
    }
}

static bool aidlEdidVersionSupported(const Capabilities& caps, HDMIVersion ver)
{
    return std::find(caps.supportedVersions.begin(), caps.supportedVersions.end(), ver)
           != caps.supportedVersions.end();
}

// ---- AIDL binder listener: per-port IHDMIInputController callbacks ----
class AidlHdmiCtrlListener
    : public ::com::rdk::hal::hdmiinput::BnHDMIInputControllerListener
{
public:
    explicit AidlHdmiCtrlListener(int portId) : m_portId(portId) {}

    ::android::binder::Status onConnectionStateChanged(bool connected) override {
        {
            std::lock_guard<std::mutex> lk(s_aidlMutex);
            auto it = s_aidlPorts.find(m_portId);
            if (it != s_aidlPorts.end()) it->second.connected = connected;
        }
        if (g_HdmiInHotPlugCallback)
            g_HdmiInHotPlugCallback(static_cast<DeviceSettingsHDMIIn::HDMIInPort>(m_portId), connected);
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status onSignalStateChanged(
            ::com::rdk::hal::hdmiinput::SignalState signalState) override {
        {
            std::lock_guard<std::mutex> lk(s_aidlMutex);
            auto it = s_aidlPorts.find(m_portId);
            if (it != s_aidlPorts.end()) it->second.signalState = (int)signalState;
        }
        if (g_HdmiInSignalStatusCallback)
            g_HdmiInSignalStatusCallback(
                static_cast<DeviceSettingsHDMIIn::HDMIInPort>(m_portId),
                static_cast<DeviceSettingsHDMIIn::HDMIInSignalStatus>((int)signalState));
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status onVIChanged(::com::rdk::hal::hdmiinput::VIC vic) override {
        {
            std::lock_guard<std::mutex> lk(s_aidlMutex);
            auto it = s_aidlPorts.find(m_portId);
            if (it != s_aidlPorts.end()) it->second.lastVIC = (int)vic;
        }
        if (g_HdmiInVideoModeUpdateCallback) {
            dsVideoPortResolution_t dsRes;
            aidlVicToRes((int)vic, dsRes);
            DeviceSettingsHDMIIn::HDMIVideoPortResolution res;
            res.name             = "";
            res.pixelResolution  = static_cast<DeviceSettingsHDMIIn::HDMIInVideoResolution>(dsRes.pixelResolution);
            res.aspectRatio      = static_cast<DeviceSettingsHDMIIn::HDMIVideoAspectRatio>(dsRes.aspectRatio);
            res.stereoScopicMode = static_cast<DeviceSettingsHDMIIn::HDMIInVideoStereoScopicMode>(dsRes.stereoScopicMode);
            res.frameRate        = static_cast<DeviceSettingsHDMIIn::HDMIInVideoFrameRate>(dsRes.frameRate);
            res.interlaced       = dsRes.interlaced;
            g_HdmiInVideoModeUpdateCallback(static_cast<DeviceSettingsHDMIIn::HDMIInPort>(m_portId), res);
        }
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status onVRRChanged(
            bool vrrActive, bool /*mConst*/, bool /*fastV*/, double frameRate) override {
        dsVRRType_t vrrType = vrrActive
            ? (frameRate > 0.0 ? dsVRR_AMD_FREESYNC : dsVRR_HDMI_VRR)
            : dsVRR_NONE;
        {
            std::lock_guard<std::mutex> lk(s_aidlMutex);
            auto it = s_aidlPorts.find(m_portId);
            if (it != s_aidlPorts.end()) {
                it->second.vrrActive    = vrrActive;
                it->second.vrrFrameRate = frameRate;
            }
        }
        if (g_HdmiInVRRStatusCallback)
            g_HdmiInVRRStatusCallback(
                static_cast<DeviceSettingsHDMIIn::HDMIInPort>(m_portId),
                static_cast<DeviceSettingsHDMIIn::HDMIInVRRType>(vrrType));
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status onAVIInfoFrame(const std::vector<uint8_t>& data) override {
        dsAviContentType_t ct = dsAVICONTENT_TYPE_NOT_SIGNALLED;
        if (!aidlParseAviContentType(data, &ct)) return ::android::binder::Status::ok();
        if (g_HdmiInAviContentTypeCallback)
            g_HdmiInAviContentTypeCallback(
                static_cast<DeviceSettingsHDMIIn::HDMIInPort>(m_portId),
                static_cast<DeviceSettingsHDMIIn::HDMIInAviContentType>(ct));
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status onAudioInfoFrame(const std::vector<uint8_t>&) override { return ::android::binder::Status::ok(); }
    ::android::binder::Status onSPDInfoFrame(const std::vector<uint8_t>&) override { return ::android::binder::Status::ok(); }
    ::android::binder::Status onDRMInfoFrame(const std::vector<uint8_t>&) override { return ::android::binder::Status::ok(); }
    ::android::binder::Status onVendorSpecificInfoFrame(const std::vector<uint8_t>&) override { return ::android::binder::Status::ok(); }
    ::android::binder::Status onHDCPStatusChanged(
            ::com::rdk::hal::hdmiinput::HDCPStatus,
            ::com::rdk::hal::hdmiinput::HDCPProtocolVersion) override { return ::android::binder::Status::ok(); }
private:
    int m_portId;
};

// ---- AIDL binder listener: port-level state / EDID change events ----
class AidlHdmiEvtListener
    : public ::com::rdk::hal::hdmiinput::BnHDMIInputEventListener
{
public:
    explicit AidlHdmiEvtListener(int portId) : m_portId(portId) {}

    ::android::binder::Status onStateChanged(
            ::com::rdk::hal::hdmiinput::State /*oldState*/,
            ::com::rdk::hal::hdmiinput::State newState) override {
        bool presented = (newState == ::com::rdk::hal::hdmiinput::State::STARTED);
        {
            std::lock_guard<std::mutex> lk(s_aidlMutex);
            if (presented) s_aidlActivePort = m_portId;
            else if (s_aidlActivePort == m_portId) s_aidlActivePort = -1;
        }
        if (g_HdmiInStatusCallback)
            g_HdmiInStatusCallback(static_cast<DeviceSettingsHDMIIn::HDMIInPort>(m_portId), presented);
        return ::android::binder::Status::ok();
    }
    ::android::binder::Status onEDIDChange(const std::vector<uint8_t>&) override { return ::android::binder::Status::ok(); }
private:
    int m_portId;
};

// ---- AIDL init/term: replace dsHdmiInInit/Term when AIDL manager is available ----
static void aidlHdmiInInit()
{
    sp<IHDMIInputManager> mgr = getAidlHdmiMgr();
    if (!mgr) { LOGERR("IHDMIInputManager unavailable"); return; }

    std::vector<IHDMIInput::Id> portIds;
    if (!mgr->getHDMIInputIds(&portIds).isOk()) { LOGERR("getHDMIInputIds failed"); return; }
    s_aidlPortCount = (uint8_t)portIds.size();

    for (const auto& id : portIds) {
        int portIdx = id.value;
        sp<IHDMIInput> hdmiInput;
        if (!mgr->getHDMIInput(id, &hdmiInput).isOk() || !hdmiInput) {
            LOGERR("getHDMIInput failed for port %d", portIdx);
            continue;
        }
        AidlPortCtx ctx;
        ctx.hdmiInput = hdmiInput;

        Capabilities caps;
        if (hdmiInput->getCapabilities(&caps).isOk() && portIdx < dsHDMI_IN_PORT_MAX) {
            s_aidlPortArcCapable[portIdx] = caps.supportsARC;
            m_hdmiPortVrrCaps[portIdx]    = caps.supportsVRR;
        }

        ctx.evtListener = sp<AidlHdmiEvtListener>::make(portIdx);
        bool regOk = false;
        hdmiInput->registerEventListener(ctx.evtListener, &regOk);

        ctx.ctrlListener = sp<AidlHdmiCtrlListener>::make(portIdx);
        sp<IHDMIInputController> ctrl;
        if (hdmiInput->open(ctx.ctrlListener, &ctrl).isOk() && ctrl) {
            ctx.controller = ctrl;
            ctx.isOpen     = true;
        }

        std::lock_guard<std::mutex> lk(s_aidlMutex);
        s_aidlPorts[portIdx] = std::move(ctx);
        LOGINFO("AIDL port %d initialised", portIdx);
    }
}

static void aidlHdmiInTerm()
{
    std::lock_guard<std::mutex> lk(s_aidlMutex);
    for (auto& kv : s_aidlPorts) {
        AidlPortCtx& ctx = kv.second;
        if (ctx.isStarted && ctx.controller) { ctx.controller->stop(); ctx.isStarted = false; }
        if (ctx.isOpen && ctx.hdmiInput && ctx.controller) {
            bool ok = false;
            ctx.hdmiInput->close(ctx.controller, &ok);
            ctx.isOpen = false;
        }
        if (ctx.hdmiInput && ctx.evtListener) {
            bool ok = false;
            ctx.hdmiInput->unregisterEventListener(ctx.evtListener, &ok);
        }
    }
    s_aidlPorts.clear();
    s_aidlHdmiMgr   = nullptr;
    s_aidlPortCount = 0;
}

class dHdmiInImpl : public hal::dHdmiIn::IPlatform {

    // delete copy constructor and assignment operator
    dHdmiInImpl(const dHdmiInImpl&) = delete;
    dHdmiInImpl& operator=(const dHdmiInImpl&) = delete;

public:
    dHdmiInImpl()
    {
        LOGINFO("dHdmiInImpl Constructor");
        InitialiseHAL();
    }

    virtual ~dHdmiInImpl()
    {
        LOGERR("dHdmiInImpl Destructor");
        DeInitialiseHAL();
    }

    void InitialiseHAL()
    {
        getDynamicAutoLatencyConfig();

        profileType = searchRdkProfile();
        LOGINFO("profileType %d", profileType);

        if (TV == profileType)
        {
            if (!m_hdmiInPlatInitialized)
            {
                aidlHdmiInInit();
                if (s_aidlPorts.empty()) {
                    // AIDL service unavailable — fall back to legacy HAL
                    dsError_t eError = dsHdmiInInit();
                    if (eError != dsERR_NONE) {
                        LOGERR("dsHdmiInInit failed: %d", eError);
                    } else {
                        LOGINFO("dsHdmiInInit succeeded");
                    }
                }
            }
            m_hdmiInPlatInitialized++;
        }
    }

    void DeInitialiseHAL()
    {
        LOGINFO("profileType %d", profileType);
        getDynamicAutoLatencyConfig();

        if (TV == profileType)
        {
            if (m_hdmiInPlatInitialized)
            {
                m_hdmiInPlatInitialized--;
                if (!m_hdmiInPlatInitialized)
                {
                    if (!s_aidlPorts.empty()) {
                        aidlHdmiInTerm();
                    } else {
                        dsHdmiInTerm();
                    }
                }
                m_hdmiInPlatInitialized = 0;
            }
        }
    }

    static void* resolve(const std::string& libName, const std::string& symbolName) {
        return WPEFramework::Plugin::DeviceSettingsHALLoader::ResolveSymbol(libName, symbolName);
    }

    bool getHdmiInPortPersistValue(const std::string& propertyName, int portIndex) {
        try {
            // Use HostPersistence from DeviceSettingsTypes.h with default value support
            std::string value = device::HostPersistence::getInstance().getProperty(propertyName, "TRUE");
            bool support = (value == "TRUE");
            LOGINFO("Port property %s: Value: %s, Parsed: %d", propertyName.c_str(), value.c_str(), support);
            return support;
        } catch(...) {
            LOGERR("Port property %s: Exception in getting property from persistence storage, using default TRUE", propertyName.c_str());
            return true;
        }
    }

    static dsError_t getVRRSupport (dsHdmiInPort_t iHdmiPort, bool *vrrSupport) {
        if (!s_aidlPorts.empty()) {
            if ((int)iHdmiPort >= 0 && (int)iHdmiPort < dsHDMI_IN_PORT_MAX) {
                *vrrSupport = m_hdmiPortVrrCaps[(int)iHdmiPort];
                LOGINFO("getVRRSupport port %d vrr=%d (AIDL)", (int)iHdmiPort, *vrrSupport);
                return dsERR_NONE;
            }
            return dsERR_INVALID_PARAM;
        }
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsHdmiInGetVRRSupport_t)(dsHdmiInPort_t iHdmiPort, bool *vrrSupport);
        static dsHdmiInGetVRRSupport_t dsHdmiInGetVRRSupportFunc = 0;

        if (dsHdmiInGetVRRSupportFunc == 0) {
            dsHdmiInGetVRRSupportFunc = (dsHdmiInGetVRRSupport_t)resolve(RDK_DSHAL_NAME, "dsHdmiInGetVRRSupport");
            if(dsHdmiInGetVRRSupportFunc == 0) {
                LOGWARN("dsHdmiInGetVRRSupport is not defined");
            }
            else {
                LOGINFO("dsHdmiInGetVRRSupport loaded");
            }
        }
        if (0 != dsHdmiInGetVRRSupportFunc) {
            eRet = dsHdmiInGetVRRSupportFunc (iHdmiPort, vrrSupport);
            LOGINFO("dsHdmiInGetVRRSupportFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("dsHdmiInGetVRRSupportFunc = %p", dsHdmiInGetVRRSupportFunc);
        }
        return eRet;
    }

    static dsError_t setVRRSupport (dsHdmiInPort_t iHdmiPort, bool vrrSupport) {
        dsError_t eRet = dsERR_GENERAL;
        if (!m_hdmiPortVrrCaps[iHdmiPort]) {
            return dsERR_OPERATION_NOT_SUPPORTED;
        }
        typedef dsError_t (*dsHdmiInSetVRRSupport_t)(dsHdmiInPort_t iHdmiPort, bool vrrSupport);
        static dsHdmiInSetVRRSupport_t dsHdmiInSetVRRSupportFunc = 0;

        if (dsHdmiInSetVRRSupportFunc == 0) {
            dsHdmiInSetVRRSupportFunc = (dsHdmiInSetVRRSupport_t)resolve(RDK_DSHAL_NAME, "dsHdmiInSetVRRSupport");
            if(dsHdmiInSetVRRSupportFunc == 0) {
                LOGERR("dsHdmiInSetVRRSupport is not defined");
            }
            else {
                LOGINFO("dsHdmiInSetVRRSupport loaded");
            }
        }
        LOGINFO("setVRRSupport to ds-hal:  EDID VRR Bit: %d", vrrSupport);
        if (0 != dsHdmiInSetVRRSupportFunc) {
            eRet = dsHdmiInSetVRRSupportFunc (iHdmiPort, vrrSupport);
            LOGINFO("[srv] %s: dsHdmiInSetVRRSupportFunc eRet: %d", __FUNCTION__, eRet);
        }
        else {
            LOGINFO("%s:  dsHdmiInSetVRRSupportFunc = %p\n", __FUNCTION__, dsHdmiInSetVRRSupportFunc);
        }
        LOGINFO("setVRRSupport to ds-hal:  EDID VRR Bit: %d\n", vrrSupport);
        if (0 != dsHdmiInSetVRRSupportFunc) {
            eRet = dsHdmiInSetVRRSupportFunc (iHdmiPort, vrrSupport);
            LOGINFO("dsHdmiInSetVRRSupportFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("dsHdmiInSetVRRSupportFunc = %p", dsHdmiInSetVRRSupportFunc);
        }
        return eRet;
    }

    static dsError_t setEdid2AllmSupport (dsHdmiInPort_t iHdmiPort, bool allmSupport) {
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsSetEdid2AllmSupport_t)(dsHdmiInPort_t iHdmiPort, bool allmSupport);
        static dsSetEdid2AllmSupport_t dsSetEdid2AllmSupportFunc = 0;

        if (dsSetEdid2AllmSupportFunc == 0) {
            dsSetEdid2AllmSupportFunc = (dsSetEdid2AllmSupport_t)resolve(RDK_DSHAL_NAME, "dsSetEdid2AllmSupport");
            if(dsSetEdid2AllmSupportFunc == 0) {
                LOGERR("dsSetEdid2AllmSupport is not defined");
            }
            else {
                LOGINFO("dsSetEdid2AllmSupport loaded");
            }
        }
        LOGINFO("setEdid2AllmSupport to ds-hal:  EDID Allm Bit: %d", allmSupport);
        if (0 != dsSetEdid2AllmSupportFunc) {
            eRet = dsSetEdid2AllmSupportFunc (iHdmiPort, allmSupport);
            LOGINFO("dsSetEdid2AllmSupportFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("dsSetEdid2AllmSupportFunc = %p", dsSetEdid2AllmSupportFunc);
        }
        return eRet;
    }

    static dsError_t isHdmiARCPort (int iPort, bool* isArcEnabled) {
        if (!s_aidlPorts.empty()) {
            if (iPort >= 0 && iPort < dsHDMI_IN_PORT_MAX) {
                *isArcEnabled = s_aidlPortArcCapable[iPort];
                LOGINFO("isHdmiARCPort port %d arc=%d (AIDL)", iPort, *isArcEnabled);
                return dsERR_NONE;
            }
            return dsERR_INVALID_PARAM;
        }
        dsError_t eRet = dsERR_GENERAL; 

        typedef bool (*dsIsHdmiARCPort_t)(int iPortArg, bool *boolArg);
        static dsIsHdmiARCPort_t dsIsHdmiARCPortFunc = 0;
        if (dsIsHdmiARCPortFunc == 0) {
            dsIsHdmiARCPortFunc = (dsIsHdmiARCPort_t)resolve(RDK_DSHAL_NAME, "dsIsHdmiARCPort");
            if(dsIsHdmiARCPortFunc == 0) {
                LOGERR("dsIsHdmiARCPort is not defined");
                eRet = dsERR_GENERAL;
            }
            else {
                LOGINFO("dsIsHdmiARCPort loaded");
            }
        }
        if (0 != dsIsHdmiARCPortFunc) { 
            dsIsHdmiARCPortFunc (iPort, isArcEnabled);
            LOGINFO("dsIsHdmiARCPort port %d isArcEnabled:%d", iPort, *isArcEnabled);
        }
        else {
            LOGINFO("dsIsHdmiARCPort  dsIsHdmiARCPortFunc = %p", dsIsHdmiARCPortFunc);
        }
        return eRet;
    }

    static dsError_t setEdidVersion (dsHdmiInPort_t iHdmiPort, tv_hdmi_edid_version_t iEdidVersion) {
        if (!s_aidlPorts.empty()) {
            HDMIVersion aidlVersion;
            if (!aidlMapEdidVersion(iEdidVersion, &aidlVersion)) return dsERR_INVALID_PARAM;
            sp<IHDMIInput> hi;
            sp<IHDMIInputController> ctrl;
            {
                std::lock_guard<std::mutex> lk(s_aidlMutex);
                auto it = s_aidlPorts.find((int)iHdmiPort);
                if (it == s_aidlPorts.end() || !it->second.hdmiInput || !it->second.controller)
                    return dsERR_INVALID_PARAM;
                hi   = it->second.hdmiInput;
                ctrl = it->second.controller;
            }
            Capabilities caps;
            if (!hi->getCapabilities(&caps).isOk()) return dsERR_GENERAL;
            if (!aidlEdidVersionSupported(caps, aidlVersion)) return dsERR_OPERATION_NOT_SUPPORTED;
            std::vector<uint8_t> edidVec;
            bool ok = false;
            if (!hi->getDefaultEDID(aidlVersion, &edidVec, &ok).isOk() || !ok || edidVec.size() < 128)
                return dsERR_GENERAL;
            ok = false;
            if (!ctrl->setEDID(edidVec, &ok).isOk() || !ok) return dsERR_GENERAL;
            int port_no = (int)iHdmiPort;
            if (port_no >= 0 && port_no < dsHDMI_IN_PORT_MAX) {
                char edidVer[2];
                sprintf(edidVer, "%d", iEdidVersion);
                std::string key = "HDMI" + std::to_string(port_no) + ".edidversion";
                device::HostPersistence::getInstance().persistHostProperty(key, edidVer);
                m_edidversion[port_no] = iEdidVersion;
            }
            LOGINFO("setEdidVersion port %d version=%d (AIDL)", (int)iHdmiPort, iEdidVersion);
            return dsERR_NONE;
        }
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsSetEdidVersion_t)(dsHdmiInPort_t iHdmiPort, tv_hdmi_edid_version_t iEdidVersion);
        static dsSetEdidVersion_t dsSetEdidVersionFunc = 0;
        char edidVer[2];
        sprintf(edidVer,"%d",iEdidVersion);

        if (dsSetEdidVersionFunc == 0) {
            dsSetEdidVersionFunc = (dsSetEdidVersion_t)resolve(RDK_DSHAL_NAME, "dsSetEdidVersion");
            if(dsSetEdidVersionFunc == 0) {
                LOGERR("dsSetEdidVersion is not defined");
            }
            else {
                LOGINFO("dsSetEdidVersion loaded");
            }
        }

        if (0 != dsSetEdidVersionFunc) {
            eRet = dsSetEdidVersionFunc (iHdmiPort, iEdidVersion);
            if (eRet == dsERR_NONE) {
                switch (iHdmiPort) {
                    case dsHDMI_IN_PORT_0:
                        device::HostPersistence::getInstance().persistHostProperty("HDMI0.edidversion", edidVer);
                        LOGINFO("Port %s: Persist EDID Version: %d", "HDMI0", iEdidVersion);
                        break;
                    case dsHDMI_IN_PORT_1:
                        device::HostPersistence::getInstance().persistHostProperty("HDMI1.edidversion", edidVer);
                        LOGINFO("Port %s: Persist EDID Version: %d", "HDMI1", iEdidVersion);
                        break;
                    case dsHDMI_IN_PORT_2:
                        device::HostPersistence::getInstance().persistHostProperty("HDMI2.edidversion", edidVer);
                        LOGINFO("Port %s: Persist EDID Version: %d", "HDMI2", iEdidVersion);
                        break;
                    case dsHDMI_IN_PORT_3:
                        device::HostPersistence::getInstance().persistHostProperty("HDMI3.edidversion", edidVer);
                        LOGINFO("Port %s: Persist EDID Version: %d", "HDMI3", iEdidVersion);
                        break;
                    case dsHDMI_IN_PORT_NONE:
                    case dsHDMI_IN_PORT_4:
                    case dsHDMI_IN_PORT_MAX:
                        break;
                }
            // Whenever there is a change in edid version to 2.0, ensure the edid allm support and edid vrr support is updated with latest value
        if(iEdidVersion == HDMI_EDID_VER_20)
            {
            LOGINFO("As the version is changed to 2.0, we are updating the allm bit and the vrr bit in edid");
            setEdid2AllmSupport(iHdmiPort,m_edidallmsupport[iHdmiPort]);
                    setVRRSupport(iHdmiPort,m_vrrsupport[iHdmiPort]);
            }
            }
            LOGINFO("dsSetEdidVersionFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("dsSetEdidVersionFunc = %p", dsSetEdidVersionFunc);
        }
        return eRet;
    }

    static dsError_t getEdidVersion (dsHdmiInPort_t iHdmiPort, int *iEdidVersion) {
        if (!s_aidlPorts.empty()) {
            if (!iEdidVersion) return dsERR_INVALID_PARAM;
            sp<IHDMIInput> hi;
            {
                std::lock_guard<std::mutex> lk(s_aidlMutex);
                auto it = s_aidlPorts.find((int)iHdmiPort);
                if (it == s_aidlPorts.end() || !it->second.hdmiInput) return dsERR_INVALID_PARAM;
                hi = it->second.hdmiInput;
            }
            std::vector<uint8_t> edidVec;
            bool ok = false;
            if (!hi->getEDID(&edidVec, &ok).isOk() || !ok || edidVec.size() < 128)
                return dsERR_GENERAL;
            *iEdidVersion = static_cast<int>(aidlGetEdidVersion(edidVec));
            LOGINFO("getEdidVersion port %d version=%d (AIDL)", (int)iHdmiPort, *iEdidVersion);
            return dsERR_NONE;
        }
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsGetEdidVersion_t)(dsHdmiInPort_t iHdmiPort, tv_hdmi_edid_version_t *iEdidVersion);
        static dsGetEdidVersion_t dsGetEdidVersionFunc = 0;
        if (dsGetEdidVersionFunc == 0) {
            dsGetEdidVersionFunc = (dsGetEdidVersion_t)resolve(RDK_DSHAL_NAME, "dsGetEdidVersion");
            if(dsGetEdidVersionFunc == 0) {
                LOGERR("dsGetEdidVersion is not defined");
            }
            else {
                LOGINFO("dsGetEdidVersion loaded");
            }
        }
        if (0 != dsGetEdidVersionFunc) {
            tv_hdmi_edid_version_t EdidVersion;
            eRet = dsGetEdidVersionFunc (iHdmiPort, &EdidVersion);
            int EdidVer = static_cast<int>(EdidVersion);
            *iEdidVersion = EdidVer;
            LOGINFO("dsGetEdidVersionFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("%s:  dsGetEdidVersionFunc = %p", __FUNCTION__, dsGetEdidVersionFunc);
        }
        return eRet;
    }

    static dsError_t getAllmStatus (dsHdmiInPort_t iHdmiPort, bool *allmStatus) {
        if (!s_aidlPorts.empty()) {
            sp<IHDMIInput> hi;
            {
                std::lock_guard<std::mutex> lk(s_aidlMutex);
                auto it = s_aidlPorts.find((int)iHdmiPort);
                if (it == s_aidlPorts.end() || !it->second.hdmiInput) return dsERR_INVALID_PARAM;
                hi = it->second.hdmiInput;
            }
            Capabilities caps;
            if (!hi->getCapabilities(&caps).isOk()) return dsERR_GENERAL;
            *allmStatus = caps.supportsALLM;
            LOGINFO("getAllmStatus port %d allm=%d (AIDL)", (int)iHdmiPort, *allmStatus);
            return dsERR_NONE;
        }
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsGetAllmStatus_t)(dsHdmiInPort_t iHdmiPort, bool *allmStatus);
        static dsGetAllmStatus_t dsGetAllmStatusFunc = 0;
        if (dsGetAllmStatusFunc == 0) {
            dsGetAllmStatusFunc = (dsGetAllmStatus_t)resolve(RDK_DSHAL_NAME, "dsGetAllmStatus");
            if(dsGetAllmStatusFunc == 0) {
                LOGERR("dsGetAllmStatus is not defined");
            }
            else {
                LOGINFO("dsGetAllmStatus loaded");
            }
        }
        if (0 != dsGetAllmStatusFunc) {
            eRet = dsGetAllmStatusFunc (iHdmiPort, allmStatus);
            LOGINFO("dsGetAllmStatusFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("dsGetAllmStatusFunc = %p", dsGetAllmStatusFunc);
        }
        return eRet;
    }

    static dsError_t getSupportedGameFeaturesList (dsSupportedGameFeatureList_t *fList) {
        if (!s_aidlPorts.empty()) {
            if (!fList) return dsERR_INVALID_PARAM;
            std::vector<sp<IHDMIInput>> inputs;
            {
                std::lock_guard<std::mutex> lk(s_aidlMutex);
                for (const auto& kv : s_aidlPorts)
                    if (kv.second.hdmiInput) inputs.push_back(kv.second.hdmiInput);
            }
            if (inputs.empty()) return dsERR_INVALID_PARAM;
            bool supAllm = false, supVrr = false, supFreeSync = false;
            for (const auto& hi : inputs) {
                Capabilities caps;
                if (!hi->getCapabilities(&caps).isOk()) return dsERR_GENERAL;
                supAllm     = supAllm     || caps.supportsALLM;
                supVrr      = supVrr      || caps.supportsVRR;
                supFreeSync = supFreeSync || caps.supportsFreeSync;
            }
            FreeSync freeSyncTier = FreeSync::UNSUPPORTED;
            if (supFreeSync) {
                sp<IHDMIInputManager> mgr = getAidlHdmiMgr();
                if (mgr) {
                    PlatformCapabilities pCaps;
                    if (mgr->getCapabilities(&pCaps).isOk()) freeSyncTier = pCaps.freeSync;
                }
            }
            std::vector<std::string> features;
            if (supAllm)     features.emplace_back("allm");
            if (supVrr)      features.emplace_back("vrr_hdmi");
            if (supFreeSync) {
                switch (freeSyncTier) {
                    case FreeSync::FREESYNC_PREMIUM:     features.emplace_back("vrr_amd_freesync_premium"); break;
                    case FreeSync::FREESYNC_PREMIUM_PRO: features.emplace_back("vrr_amd_freesync_premium_pro"); break;
                    default:                             features.emplace_back("vrr_amd_freesync"); break;
                }
            }
            memset(fList, 0, sizeof(*fList));
            fList->gameFeatureCount = (int)features.size();
            std::string csv;
            for (size_t i = 0; i < features.size(); ++i) { if (i) csv += ","; csv += features[i]; }
            strncpy(fList->gameFeatureList, csv.c_str(), sizeof(fList->gameFeatureList) - 1);
            LOGINFO("getSupportedGameFeaturesList count=%d (AIDL)", fList->gameFeatureCount);
            return dsERR_NONE;
        }
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsGetSupportedGameFeaturesList_t)(dsSupportedGameFeatureList_t *fList);
        static dsGetSupportedGameFeaturesList_t dsGetSupportedGameFeaturesListFunc = 0;
        if (dsGetSupportedGameFeaturesListFunc == 0) {
            dsGetSupportedGameFeaturesListFunc = (dsGetSupportedGameFeaturesList_t)resolve(RDK_DSHAL_NAME, "dsGetSupportedGameFeaturesList");
            if(dsGetSupportedGameFeaturesListFunc == 0) {
                LOGERR("dsGetSupportedGameFeaturesList is not defined");
            }
            else {
                LOGINFO("dsGetSupportedGameFeaturesList loaded");
            }
        }
        if (0 != dsGetSupportedGameFeaturesListFunc) {
            eRet = dsGetSupportedGameFeaturesListFunc (fList);
            LOGINFO("dsGetSupportedGameFeaturesListFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("dsGetSupportedGameFeaturesListFunc = %p", dsGetSupportedGameFeaturesListFunc);
        }
        return eRet;
    }

    static dsError_t getAVLatency_hal (int *audio_latency, int *video_latency)
    {
        if (!s_aidlPorts.empty()) {
            *audio_latency = 0;
            *video_latency = 0;
            LOGINFO("getAVLatency: returning 0/0 (AIDL, no PlaneControl)");
            return dsERR_NONE;
        }
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsGetAVLatency_t)(int *audio_latency, int *video_latency);
        static dsGetAVLatency_t dsGetAVLatencyFunc = 0;
        if (dsGetAVLatencyFunc == 0) {
            dsGetAVLatencyFunc = (dsGetAVLatency_t)resolve(RDK_DSHAL_NAME, "dsGetAVLatency");
            if(dsGetAVLatencyFunc == 0) {
                LOGERR("dsGetAVLatency is not defined");
            }
            else {
                LOGINFO("dsGetAVLatency loaded");
            }
        }
        if (0 != dsGetAVLatencyFunc) {
            eRet = dsGetAVLatencyFunc (audio_latency, video_latency);
            LOGINFO("dsGetAVLatencyFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("dsGetAVLatencyFunc = %p", dsGetAVLatencyFunc);
        }
        return eRet;
    }

    static dsError_t getHdmiVersion (dsHdmiInPort_t iHdmiPort, dsHdmiMaxCapabilityVersion_t  *capversion) {
        if (!s_aidlPorts.empty()) {
            sp<IHDMIInput> hi;
            {
                std::lock_guard<std::mutex> lk(s_aidlMutex);
                auto it = s_aidlPorts.find((int)iHdmiPort);
                if (it == s_aidlPorts.end() || !it->second.hdmiInput) return dsERR_INVALID_PARAM;
                hi = it->second.hdmiInput;
            }
            Capabilities caps;
            if (!hi->getCapabilities(&caps).isOk()) return dsERR_GENERAL;
            *capversion = HDMI_COMPATIBILITY_VERSION_14;
            for (const auto& v : caps.supportedVersions) {
                if (v == HDMIVersion::HDMI_2_1 && *capversion < HDMI_COMPATIBILITY_VERSION_21)
                    *capversion = HDMI_COMPATIBILITY_VERSION_21;
                else if (v == HDMIVersion::HDMI_2_0 && *capversion < HDMI_COMPATIBILITY_VERSION_20)
                    *capversion = HDMI_COMPATIBILITY_VERSION_20;
            }
            LOGINFO("getHdmiVersion port %d version=%d (AIDL)", (int)iHdmiPort, *capversion);
            return dsERR_NONE;
        }
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsGetHdmiVersion_t)(dsHdmiInPort_t iHdmiPort, dsHdmiMaxCapabilityVersion_t  *capversion);
        static dsGetHdmiVersion_t dsGetHdmiVersionFunc = 0;
        if (dsGetHdmiVersionFunc == 0) {
            dsGetHdmiVersionFunc = (dsGetHdmiVersion_t)resolve(RDK_DSHAL_NAME, "dsGetHdmiVersion");
            if(dsGetHdmiVersionFunc == 0) {
                LOGERR("dsGetHdmiVersion is not defined");
                eRet = dsERR_GENERAL;
            }
            else {
                LOGINFO("dsGetHdmiVersion loaded");
            }
        }
        if (0 != dsGetHdmiVersionFunc) {
            eRet = dsGetHdmiVersionFunc (iHdmiPort, capversion);
            LOGINFO("dsGetHdmiVersionFunc eRet: %d", eRet);
        }
        return eRet;
    }

    void setAllCallbacks(const CallbackBundle bundle) override
    {
        ENTRY_LOG;
        LOGINFO("setAllCallbacks: profileType %d", profileType);
        if (!m_hdmiInInitialized && m_hdmiInPlatInitialized) {
            LOGINFO("HdmiIn platform callback Initialization");
            if (TV == profileType)
            {
                LOGINFO("setAllCallbacks: its TV Profile");
                if (!s_aidlPorts.empty()) {
                    // AIDL listeners already registered in aidlHdmiInInit; just store the callbacks.
                    if (bundle.OnHDMIInHotPlugEvent)         g_HdmiInHotPlugCallback         = bundle.OnHDMIInHotPlugEvent;
                    if (bundle.OnHDMIInSignalStatusEvent)    g_HdmiInSignalStatusCallback    = bundle.OnHDMIInSignalStatusEvent;
                    if (bundle.OnHDMIInStatusEvent)          g_HdmiInStatusCallback          = bundle.OnHDMIInStatusEvent;
                    if (bundle.OnHDMIInVideoModeUpdateEvent) g_HdmiInVideoModeUpdateCallback = bundle.OnHDMIInVideoModeUpdateEvent;
                    if (bundle.OnHDMIInAllmStatusEvent)      g_HdmiInAllmStatusCallback      = bundle.OnHDMIInAllmStatusEvent;
                    if (bundle.OnHDMIInAVIContentTypeEvent)  g_HdmiInAviContentTypeCallback  = bundle.OnHDMIInAVIContentTypeEvent;
                    if (bundle.OnHDMIInAVLatencyEvent)       g_HdmiInAVLatencyCallback       = bundle.OnHDMIInAVLatencyEvent;
                    if (bundle.OnHDMIInVRRStatusEvent)       g_HdmiInVRRStatusCallback       = bundle.OnHDMIInVRRStatusEvent;
                    EXIT_LOG;
                    return;
                }
                if (bundle.OnHDMIInHotPlugEvent) {
                    LOGINFO("HDMI In Hot Plug Event Callback Registered");
                    g_HdmiInHotPlugCallback = bundle.OnHDMIInHotPlugEvent;
                    dsHdmiInRegisterConnectCB(DS_OnHDMIInHotPlugEvent);
                }

                typedef dsError_t (*dsHdmiInRegisterSignalChangeCB_t)(dsHdmiInSignalChangeCB_t CBFunc);
                static dsHdmiInRegisterSignalChangeCB_t signalChangeCBFunc = 0;
                if (bundle.OnHDMIInSignalStatusEvent) {
                    LOGINFO("HDMI In Signal Status Event Callback Registered");
                    g_HdmiInSignalStatusCallback = bundle.OnHDMIInSignalStatusEvent;
                    if (!signalChangeCBFunc) {
                        signalChangeCBFunc = (dsHdmiInRegisterSignalChangeCB_t)resolve(RDK_DSHAL_NAME, "dsHdmiInRegisterSignalChangeCB");
                    }
                    if (signalChangeCBFunc) {
                        signalChangeCBFunc(DS_OnHDMIInSignalStatusEvent);
                    } else {
                        LOGERR("Failed to resolve dsHdmiInRegisterSignalChangeCB");
                    }
                }

                typedef dsError_t (*dsHdmiInRegisterStatusChangeCB_t)(dsHdmiInStatusChangeCB_t CBFunc);
                static dsHdmiInRegisterStatusChangeCB_t StatusCBFunc = 0;
                if (bundle.OnHDMIInStatusEvent) {
                    LOGINFO("HDMI In Status Event Callback Registered");
                    g_HdmiInStatusCallback = bundle.OnHDMIInStatusEvent;
                    if (!StatusCBFunc) {
                        StatusCBFunc = (dsHdmiInRegisterStatusChangeCB_t)resolve(RDK_DSHAL_NAME, "dsHdmiInRegisterStatusChangeCB");
                    }
                    if (StatusCBFunc) {
                        StatusCBFunc(DS_OnHDMIInStatusEvent);
                    } else {
                        LOGERR("Failed to resolve dsHdmiInRegisterStatusChangeCB");
                    }
                }

                typedef dsError_t (*dsHdmiInRegisterVideoModeUpdateCB_t)(dsHdmiInVideoModeUpdateCB_t CBFunc);
                static dsHdmiInRegisterVideoModeUpdateCB_t videoModeUpdateCBFunc = 0;
                if (bundle.OnHDMIInVideoModeUpdateEvent) {
                    LOGINFO("HDMI In Video Mode Update Event Callback Registered");
                    g_HdmiInVideoModeUpdateCallback = bundle.OnHDMIInVideoModeUpdateEvent;
                    if (!videoModeUpdateCBFunc) {
                        videoModeUpdateCBFunc = (dsHdmiInRegisterVideoModeUpdateCB_t)resolve(RDK_DSHAL_NAME, "dsHdmiInRegisterVideoModeUpdateCB");
                    }
                    if (videoModeUpdateCBFunc) {
                        videoModeUpdateCBFunc(DS_OnHDMIInVideoModeUpdateEvent);
                    } else {
                        LOGERR("Failed to resolve dsHdmiInRegisterVideoModeUpdateCB");
                    }
                }

                typedef dsError_t (*dsHdmiInRegisterAllmChangeCB_t)(dsHdmiInAllmChangeCB_t CBFunc);
                static dsHdmiInRegisterAllmChangeCB_t allmChangeCBFunc = 0;
                if (bundle.OnHDMIInAllmStatusEvent) {
                    LOGINFO("HDMI In ALLM Status Event Callback Registered");
                    g_HdmiInAllmStatusCallback = bundle.OnHDMIInAllmStatusEvent;
                    if (!allmChangeCBFunc) {
                        allmChangeCBFunc = (dsHdmiInRegisterAllmChangeCB_t)resolve(RDK_DSHAL_NAME, "dsHdmiInRegisterAllmChangeCB");
                    }
                    if (allmChangeCBFunc) {
                        allmChangeCBFunc(DS_OnHDMIInAllmStatusEvent);
                    } else {
                        LOGERR("Failed to resolve dsHdmiInRegisterALLMChangeCB");
                    }
                }

                typedef dsError_t (*dsHdmiInRegisterVRRChangeCB_t)(dsHdmiInVRRChangeCB_t CBFunc);
                static dsHdmiInRegisterVRRChangeCB_t vrrChangeCBFunc = 0;
                if (bundle.OnHDMIInVRRStatusEvent) {
                    LOGINFO("HDMI In VRR Status Event Callback Registered");
                    g_HdmiInVRRStatusCallback = bundle.OnHDMIInVRRStatusEvent;
                    if (!vrrChangeCBFunc) {
                        vrrChangeCBFunc = (dsHdmiInRegisterVRRChangeCB_t)resolve(RDK_DSHAL_NAME, "dsHdmiInRegisterVRRChangeCB");
                    }
                    if (vrrChangeCBFunc) {
                        vrrChangeCBFunc(DS_OnHDMIInVRRStatusEvent);
                    } else {
                        LOGWARN("dsHdmiInRegisterVRRChangeCB not supported on this platform");
                    }
                }

                typedef dsError_t (*dsHdmiInRegisterAviContentTypeChangeCB_t)(dsHdmiInAviContentTypeChangeCB_t CBFunc);
                static dsHdmiInRegisterAviContentTypeChangeCB_t AviContentTypeChangeCBFunc = 0;
                if (bundle.OnHDMIInAVIContentTypeEvent) {
                    LOGINFO("HDMI In AVI Content Type Event Callback Registered");
                    g_HdmiInAviContentTypeCallback = bundle.OnHDMIInAVIContentTypeEvent;
                    if (!AviContentTypeChangeCBFunc) {
                        AviContentTypeChangeCBFunc = (dsHdmiInRegisterAviContentTypeChangeCB_t)resolve(RDK_DSHAL_NAME, "dsHdmiInRegisterAviContentTypeChangeCB");
                    }
                    if (AviContentTypeChangeCBFunc) {
                        AviContentTypeChangeCBFunc(DS_OnHDMIInAVIContentTypeEvent);
                    } else {
                        LOGERR("Failed to resolve dsHdmiInRegisterAviContentTypeChangeCB");
                    }
                }

                typedef dsError_t (*dsHdmiInRegisterAVLatencyChangeCB_t)(dsAVLatencyChangeCB_t CBFunc);
                static dsHdmiInRegisterAVLatencyChangeCB_t AVLatencyChangeCBFunc = 0;
                if (bundle.OnHDMIInAVLatencyEvent) {
                    LOGINFO("HDMI In AV Latency Event Callback Registered");
                    g_HdmiInAVLatencyCallback = bundle.OnHDMIInAVLatencyEvent;
                    if (!AVLatencyChangeCBFunc) {
                        AVLatencyChangeCBFunc = (dsHdmiInRegisterAVLatencyChangeCB_t)resolve(RDK_DSHAL_NAME, "dsHdmiInRegisterAVLatencyChangeCB");
                    }
                    if (AVLatencyChangeCBFunc && isDalsEnabled) {
                        AVLatencyChangeCBFunc(DS_OnHDMIInAVLatencyEvent);
                    } else {
                        LOGWARN("dsHdmiInRegisterAVLatencyChangeCB not supported or DALS disabled");
                    }
                }
            }
        }
        EXIT_LOG;
    }

    void getPersistenceValue() override
    {
        if (!m_hdmiInInitialized && m_hdmiInPlatInitialized) {
            int itr = 0;
            bool isARCCapable = false;
            for (itr = 0; itr < dsHDMI_IN_PORT_MAX; itr++) {
                isARCCapable = false;
                isHdmiARCPort (itr, &isARCCapable);
                hdmiInCap_gs.isPortArcCapable[itr] = isARCCapable; 
            }

            std::string _EdidAllmSupport("TRUE");
            m_edidallmsupport[dsHDMI_IN_PORT_0] = getHdmiInPortPersistValue("HDMI0.edidallmEnable", dsHDMI_IN_PORT_0);
            m_edidallmsupport[dsHDMI_IN_PORT_1] = getHdmiInPortPersistValue("HDMI1.edidallmEnable", dsHDMI_IN_PORT_1);
            m_edidallmsupport[dsHDMI_IN_PORT_2] = getHdmiInPortPersistValue("HDMI2.edidallmEnable", dsHDMI_IN_PORT_2);
            m_edidallmsupport[dsHDMI_IN_PORT_3] = getHdmiInPortPersistValue("HDMI3.edidallmEnable", dsHDMI_IN_PORT_3);

            std::string _VRRSupport("TRUE");
            m_vrrsupport[dsHDMI_IN_PORT_0] = getHdmiInPortPersistValue("HDMI0.vrrEnable", dsHDMI_IN_PORT_0);
            m_vrrsupport[dsHDMI_IN_PORT_1] = getHdmiInPortPersistValue("HDMI1.vrrEnable", dsHDMI_IN_PORT_1);
            m_vrrsupport[dsHDMI_IN_PORT_2] = getHdmiInPortPersistValue("HDMI2.vrrEnable", dsHDMI_IN_PORT_2);
            m_vrrsupport[dsHDMI_IN_PORT_3] = getHdmiInPortPersistValue("HDMI3.vrrEnable", dsHDMI_IN_PORT_3);

            std::string _EdidVersion("1");
            try {
                _EdidVersion = device::HostPersistence::getInstance().getProperty("HDMI0.edidversion");
                m_edidversion[dsHDMI_IN_PORT_0] = static_cast<tv_hdmi_edid_version_t>(atoi (_EdidVersion.c_str()));
            } catch(...) {
                try {
                    LOGERR("Port %s: Exception in Getting the HDMI0 EDID version from persistence storage. Try system default...", "HDMI0");
                    _EdidVersion = device::HostPersistence::getInstance().getDefaultProperty("HDMI0.edidversion");
                    m_edidversion[dsHDMI_IN_PORT_0] = static_cast<tv_hdmi_edid_version_t>(atoi (_EdidVersion.c_str()));
                }
                catch(...) {
                    LOGERR("Port %s: Exception in Getting the HDMI0 EDID version from system default.....", "HDMI0");
                    m_edidversion[dsHDMI_IN_PORT_0] = HDMI_EDID_VER_20;
                }
            }

            try {
                _EdidVersion = device::HostPersistence::getInstance().getProperty("HDMI1.edidversion");
                m_edidversion[dsHDMI_IN_PORT_1] = static_cast<tv_hdmi_edid_version_t>(atoi (_EdidVersion.c_str()));
            } catch(...) {
                try {
                    LOGERR("Port %s: Exception in Getting the HDMI1 EDID version from persistence storage. Try system default...", "HDMI1");
                    _EdidVersion = device::HostPersistence::getInstance().getDefaultProperty("HDMI1.edidversion");
                    m_edidversion[dsHDMI_IN_PORT_1] = static_cast<tv_hdmi_edid_version_t>(atoi (_EdidVersion.c_str()));
                }
                catch(...) {
                    LOGERR("Port %s: Exception in Getting the HDMI1 EDID version from system default.....", "HDMI1");
                    m_edidversion[dsHDMI_IN_PORT_1] = HDMI_EDID_VER_20;
                }
            }

            try {
                _EdidVersion = device::HostPersistence::getInstance().getProperty("HDMI2.edidversion");
                m_edidversion[dsHDMI_IN_PORT_2] = static_cast<tv_hdmi_edid_version_t>(atoi (_EdidVersion.c_str()));
            } catch(...) {
                try {
                    LOGERR("Port %s: Exception in Getting the HDMI2 EDID version from persistence storage. Try system default...", "HDMI2");
                    _EdidVersion = device::HostPersistence::getInstance().getDefaultProperty("HDMI2.edidversion");
                    m_edidversion[dsHDMI_IN_PORT_2] = static_cast<tv_hdmi_edid_version_t>(atoi (_EdidVersion.c_str()));
                }
                catch(...) {
                    LOGERR("Port %s: Exception in Getting the HDMI2 EDID version from system default.....", "HDMI2");
                    m_edidversion[dsHDMI_IN_PORT_2] = HDMI_EDID_VER_20;
                }
            }

            for (itr = 0; itr < dsHDMI_IN_PORT_MAX; itr++) {
                if (getVRRSupport(static_cast<dsHdmiInPort_t>(itr), &m_hdmiPortVrrCaps[itr]) >= 0) {
                    LOGINFO("Port HDMI%d: VRR capability : %d", itr, m_hdmiPortVrrCaps[itr]);
                }
            }
            for (itr = 0; itr < dsHDMI_IN_PORT_MAX; itr++) {
                if (setEdidVersion (static_cast<dsHdmiInPort_t>(itr), m_edidversion[itr]) >= 0) {
                    LOGINFO("Port HDMI%d: Initialized EDID Version : %d", itr, m_edidversion[itr]);
                }
            }
            m_hdmiInInitialized = 1;
        }

        LOGINFO("Set Callbacks");
    }

    #if 0
    profile_t searchRdkProfile(void) {
        LOGINFO("Entering searchRdkProfile");
        const char* devPropPath = "/etc/device.properties";
        char line[256], *rdkProfile = NULL;
        profile_t ret = PROFILE_INVALID;
        FILE* file;

        file = fopen(devPropPath, "r");
        if (file == NULL) {
            LOGINFO("searchRdkProfile: device.properties file not found.");
            return PROFILE_INVALID;
        }

        while (fgets(line, sizeof(line), file)) {
            rdkProfile = strstr(line, RDK_PROFILE);
            if (rdkProfile != NULL) {
                rdkProfile = strchr(line, '=');
                LOGINFO("searchRdkProfile: Found RDK_PROFILE");
                break;
            }
        }
        if(rdkProfile != NULL)
        {
            rdkProfile++; // Move past the '=' character
            if(0 == strncmp(rdkProfile, PROFILE_STR_TV, strlen(PROFILE_STR_TV))) {
                ret = PROFILE_TV;
            } else if (0 == strncmp(rdkProfile, PROFILE_STR_STB, strlen(PROFILE_STR_STB))) {
                ret = PROFILE_STB;
            }
        }
        else
        {
            LOGINFO("searchRdkProfile: NOT FOUND RDK_PROFILE in device properties file");
            ret = PROFILE_INVALID;
        }

        fclose(file);
        LOGINFO("Exit searchRdkProfile: RDK_PROFILE = %d", ret);
        return ret;
    }
    #endif

    void getDynamicAutoLatencyConfig()
    {
        RFC_ParamData_t param = {0};
        WDMP_STATUS status = getRFCParameter((char*)"dssrv", TVSETTINGS_DALS_RFC_PARAM, &param);
        LOGINFO("DALS Feature Enable = [ %s ]", param.value);
        if(WDMP_SUCCESS == status && (strncasecmp(param.value,"true",4) == 0)) {
            isDalsEnabled = true;
            LOGINFO("Value of isDalsEnabled = [ %d ]", isDalsEnabled);
        }
        else {
            LOGERR("Fetching RFC for DALS failed or DALS is disabled: %d", status);
        }
    }

    // Missing functions from dsHdmiIn.c
    void updateEdidAllmBitValuesInPersistence(dsHdmiInPort_t iHdmiPort, bool allmSupport)
    {
        LOGINFO("Updating values of edid allm bit in persistence");
        switch(iHdmiPort){
            case dsHDMI_IN_PORT_0:
                device::HostPersistence::getInstance().persistHostProperty("HDMI0.edidallmEnable", allmSupport ? "TRUE" : "FALSE");
                LOGINFO("Port %s: Persist EDID Allm Bit: %d", "HDMI0", allmSupport);
                break;
            case dsHDMI_IN_PORT_1:
                device::HostPersistence::getInstance().persistHostProperty("HDMI1.edidallmEnable", allmSupport ? "TRUE" : "FALSE");
                LOGINFO("Port %s: Persist EDID Allm Bit: %d", "HDMI1", allmSupport);
                break;
            case dsHDMI_IN_PORT_2:
                device::HostPersistence::getInstance().persistHostProperty("HDMI2.edidallmEnable", allmSupport ? "TRUE" : "FALSE");
                LOGINFO("Port %s: Persist EDID Allm Bit: %d", "HDMI2", allmSupport);
                break;
            case dsHDMI_IN_PORT_3:
                device::HostPersistence::getInstance().persistHostProperty("HDMI3.edidallmEnable", allmSupport ? "TRUE" : "FALSE");
                LOGINFO("Port %s: Persist EDID Allm Bit: %d", "HDMI3", allmSupport);
                break;
            default:
                LOGWARN("Invalid HDMI port %d for ALLM persistence update", iHdmiPort);
                break;
        }
    }

    void updateVRRBitValuesInPersistence(dsHdmiInPort_t iHdmiPort, bool vrrSupport)
    {
        LOGINFO("Updating values of vrr bit in persistence");
        switch(iHdmiPort){
            case dsHDMI_IN_PORT_0:
                device::HostPersistence::getInstance().persistHostProperty("HDMI0.vrrEnable", vrrSupport ? "TRUE" : "FALSE");
                LOGINFO("Port %s: Persist EDID VRR Bit: %d", "HDMI0", vrrSupport);
                break;
            case dsHDMI_IN_PORT_1:
                device::HostPersistence::getInstance().persistHostProperty("HDMI1.vrrEnable", vrrSupport ? "TRUE" : "FALSE");
                LOGINFO("Port %s: Persist EDID VRR Bit: %d", "HDMI1", vrrSupport);
                break;
            case dsHDMI_IN_PORT_2:
                device::HostPersistence::getInstance().persistHostProperty("HDMI2.vrrEnable", vrrSupport ? "TRUE" : "FALSE");
                LOGINFO("Port %s: Persist EDID VRR Bit: %d", "HDMI2", vrrSupport);
                break;
            case dsHDMI_IN_PORT_3:
                device::HostPersistence::getInstance().persistHostProperty("HDMI3.vrrEnable", vrrSupport ? "TRUE" : "FALSE");
                LOGINFO("Port %s: Persist EDID VRR Bit: %d", "HDMI3", vrrSupport);
                break;
            default:
                LOGWARN("Invalid HDMI port %d for VRR persistence update", iHdmiPort);
                break;
        }
    }

    static void DS_OnHDMIInHotPlugEvent(const dsHdmiInPort_t port, const bool isConnected)
    {
        LOGINFO("DS_OnHDMIInHotPlugEvent event Received: port=%d, isConnected=%s", port, isConnected ? "true" : "false");
        if (g_HdmiInHotPlugCallback) {
            g_HdmiInHotPlugCallback(static_cast<DeviceSettingsHDMIIn::HDMIInPort>(port), isConnected);
        }
    }

    static void DS_OnHDMIInSignalStatusEvent(const dsHdmiInPort_t port, const dsHdmiInSignalStatus_t signalStatus)
    {
        LOGINFO("DS_OnHDMIInSignalStatusEvent event Received: port=%d, signalStatus=%d", port, signalStatus);
        if (g_HdmiInSignalStatusCallback) {
            g_HdmiInSignalStatusCallback(static_cast<HDMIInPort>(port), static_cast<HDMIInSignalStatus>(signalStatus));
        }
    }

    static void DS_OnHDMIInStatusEvent(const dsHdmiInStatus_t status)
    {
        LOGINFO("DS_OnHDMIInStatusEvent event Received: Port=%d, isPresented=%s", status.activePort, status.isPresented ? "true" : "false");
        
        if (g_HdmiInStatusCallback) {
            g_HdmiInStatusCallback(static_cast<HDMIInPort>(status.activePort), status.isPresented);
        }
    }

    static void DS_OnHDMIInVideoModeUpdateEvent(const dsHdmiInPort_t port, const dsVideoPortResolution_t videoPortResolution)
    {
        LOGINFO("DS_OnHDMIInVideoModeUpdateEvent event Received: port=%d", port); // adjust as needed
        LOGINFO("Video Mode: %s pixelResolution %d aspectRatio %d stereoScopicMode %d frameRate %d", videoPortResolution.name, videoPortResolution.pixelResolution, videoPortResolution.aspectRatio, videoPortResolution.stereoScopicMode, videoPortResolution.frameRate);

        if (g_HdmiInVideoModeUpdateCallback) {
            HDMIVideoPortResolution res;
            res.name              = std::string(videoPortResolution.name);
            res.pixelResolution   = static_cast<HDMIInVideoResolution>(videoPortResolution.pixelResolution);
            res.aspectRatio       = static_cast<HDMIVideoAspectRatio>(videoPortResolution.aspectRatio);
            res.stereoScopicMode  = static_cast<HDMIInVideoStereoScopicMode>(videoPortResolution.stereoScopicMode);
            res.frameRate         = static_cast<HDMIInVideoFrameRate>(videoPortResolution.frameRate);
            res.interlaced        = videoPortResolution.interlaced;
            g_HdmiInVideoModeUpdateCallback(static_cast<HDMIInPort>(port), res);
        }
    }

    static void DS_OnHDMIInAllmStatusEvent(const dsHdmiInPort_t port, const bool allmStatus)
    {
        LOGINFO("DS_OnHDMIInAllmStatusEvent event Received: port=%d, allmStatus=%s", port, allmStatus ? "true" : "false");
        if (g_HdmiInAllmStatusCallback) {
            g_HdmiInAllmStatusCallback(static_cast<HDMIInPort>(port), allmStatus);
        }
    }

    static void DS_OnHDMIInAVIContentTypeEvent(const dsHdmiInPort_t port, const dsAviContentType_t aviContentType)
    {
        LOGINFO("DS_OnHDMIInAVIContentTypeEvent event Received: port=%d, aviContentType=%d", port, aviContentType);
        if (g_HdmiInAviContentTypeCallback) {
            g_HdmiInAviContentTypeCallback(static_cast<HDMIInPort>(port), static_cast<HDMIInAviContentType>(aviContentType));
        }
    }

    static void DS_OnHDMIInAVLatencyEvent(const int32_t audioDelay, const int32_t videoDelay)
    {
        LOGINFO("DS_OnHDMIInAVLatencyEvent event Received: audioDelay=%d, videoDelay=%d", audioDelay, videoDelay);
        if (g_HdmiInAVLatencyCallback) {
            g_HdmiInAVLatencyCallback(audioDelay, videoDelay);
        }
    }

    static void DS_OnHDMIInVRRStatusEvent(const dsHdmiInPort_t port, const dsVRRType_t vrrType)
    {
        LOGINFO("DS_OnHDMIInVRRStatusEvent event Received: port=%d, vrrType=%d", port, vrrType);
        if (g_HdmiInVRRStatusCallback) {
            g_HdmiInVRRStatusCallback(static_cast<HDMIInPort>(port), static_cast<HDMIInVRRType>(vrrType));
        }
    }

    virtual uint32_t GetHDMIInNumberOfInputs(int32_t &count) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        if (!s_aidlPorts.empty()) {
            count = static_cast<int32_t>(s_aidlPortCount);
            LOGINFO("GetHDMIInNumberOfInputs: count=%d (AIDL)", count);
            return WPEFramework::Core::ERROR_NONE;
        }
        uint8_t NumberofInputs = 0;

        if (dsHdmiInGetNumberOfInputs(&NumberofInputs) == dsERR_NONE) {
            count = static_cast<int32_t>(NumberofInputs);
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        LOGINFO("GetHDMIInNumberOfInputs: count=%d, retCode=%d", count, retCode);
        return retCode;
    }

    uint32_t GetHDMIInStatus(HDMIInStatus &hdmiStatus, IHDMIInPortConnectionStatusIterator*& portConnectionStatus) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        if (!s_aidlPorts.empty()) {
            std::vector<DeviceSettingsHDMIIn::HDMIPortConnectionStatus> portStatuses;
            {
                std::lock_guard<std::mutex> lk(s_aidlMutex);
                hdmiStatus.activePort  = static_cast<HDMIInPort>(s_aidlActivePort);
                hdmiStatus.isPresented = (s_aidlActivePort >= 0);
                for (int p = 0; p < dsHDMI_IN_PORT_MAX; p++) {
                    DeviceSettingsHDMIIn::HDMIPortConnectionStatus ps;
                    auto it = s_aidlPorts.find(p);
                    ps.isPortConnected = (it != s_aidlPorts.end()) ? it->second.connected : false;
                    portStatuses.push_back(ps);
                }
            }
            portConnectionStatus = WPEFramework::Core::Service<WPEFramework::RPC::IteratorType<IHDMIInPortConnectionStatusIterator>>::Create<IHDMIInPortConnectionStatusIterator>(portStatuses);
            LOGINFO("GetHDMIInStatus (AIDL): activePort=%d isPresented=%s", (int)s_aidlActivePort, hdmiStatus.isPresented ? "true" : "false");
            return WPEFramework::Core::ERROR_NONE;
        }
        dsHdmiInStatus_t status;
        if (dsHdmiInGetStatus(&status) == dsERR_NONE) {
            hdmiStatus.activePort = static_cast<HDMIInPort>(status.activePort);
            hdmiStatus.isPresented = status.isPresented;
            LOGINFO("GetHDMIInStatus: activePort=%d, isPresented=%s", status.activePort, status.isPresented ? "true" : "false");

            /* Build per-port connection status iterator from dsHdmiInStatus_t.isPortConnected[]. */
            std::vector<DeviceSettingsHDMIIn::HDMIPortConnectionStatus> portStatuses;
            for (int p = 0; p < dsHDMI_IN_PORT_MAX; p++) {
                DeviceSettingsHDMIIn::HDMIPortConnectionStatus ps;
                ps.isPortConnected = status.isPortConnected[p];
                portStatuses.push_back(ps);
                LOGINFO("GetHDMIInStatus: port[%d] isPortConnected=%s", p, ps.isPortConnected ? "true" : "false");
            }
            portConnectionStatus = WPEFramework::Core::Service<WPEFramework::RPC::IteratorType<IHDMIInPortConnectionStatusIterator>>::Create<IHDMIInPortConnectionStatusIterator>(portStatuses);

            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t GetHDMIInAVLatency(uint32_t &videoLatency, uint32_t &audioLatency) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        int vLatency = 0;
        int aLatency = 0;
        if (getAVLatency_hal(&aLatency, &vLatency) == dsERR_NONE) {
            audioLatency = static_cast<uint32_t>(aLatency);
            videoLatency = static_cast<uint32_t>(vLatency);
            LOGINFO("GetHDMIInAVLatency: audioLatency=%d, videoLatency=%d", audioLatency, videoLatency);
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t GetHDMIInAllmStatus(const HDMIInPort port, bool &allmStatus) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        bool status = false;
        if (getAllmStatus(hdmiPort, &status) == dsERR_NONE) {
            allmStatus = status;
            LOGINFO("GetHDMIInAllmStatus: port=%d, allmStatus=%s", hdmiPort, allmStatus ? "true" : "false");
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t GetHDMIInEdid2AllmSupport(const HDMIInPort port, bool &allmSupport) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        if (hdmiPort < dsHDMI_IN_PORT_MAX) {
            allmSupport = m_edidallmsupport[hdmiPort];
            LOGINFO("GetHDMIInEdid2AllmSupport: port=%d, allmSupport=%s", hdmiPort, allmSupport ? "true" : "false");
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t SetHDMIInEdid2AllmSupport(const HDMIInPort port, bool allmSupport) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        if (hdmiPort < dsHDMI_IN_PORT_MAX) {
            LOGINFO("In SetHDMIInEdid2AllmSupport, checking m_edidversion of port %d : %d", hdmiPort, m_edidversion[hdmiPort]);
            if(m_edidversion[hdmiPort] == HDMI_EDID_VER_20) { // if the edidver is 2.0, then only set the allm bit in edid
                if (setEdid2AllmSupport(hdmiPort, allmSupport) == dsERR_NONE) {
                    updateEdidAllmBitValuesInPersistence(hdmiPort, allmSupport);
                    m_edidallmsupport[hdmiPort] = allmSupport;
                    LOGINFO("SetHDMIInEdid2AllmSupport: port=%d, allmSupport=%s", hdmiPort, allmSupport ? "true" : "false");
                    retCode = WPEFramework::Core::ERROR_NONE;
                }
            } else {
                LOGINFO("EDID version is not 2.0, cannot set ALLM support for port %d", hdmiPort);
                retCode = WPEFramework::Core::ERROR_UNAVAILABLE;
            }
        }
        return retCode;
    }

    uint32_t GetSupportedGameFeaturesList(IHDMIInGameFeatureListIterator *& gameFeatureList) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsSupportedGameFeatureList_t fList;

        // Initialize the structure
        memset(&fList, 0, sizeof(fList));

        dsError_t dsResult = getSupportedGameFeaturesList(&fList);
        LOGINFO("GetSupportedGameFeaturesList: dsGetSupportedGameFeaturesList returned: %d", dsResult);

        if (dsResult == dsERR_NONE) {
            LOGINFO("GetSupportedGameFeaturesList: Raw HAL data - gameFeatureList='%s', count=%d", 
                    fList.gameFeatureList, fList.gameFeatureCount);

            try {
                // Parse the comma-separated game features string
                std::vector<DeviceSettingsHDMIIn::HDMIInGameFeatureList> features;

                if (strlen(fList.gameFeatureList) > 0) {
                    std::string featureStr(fList.gameFeatureList);
                    std::stringstream ss(featureStr);
                    std::string feature;

                    // Split by comma and create feature entries
                    while (std::getline(ss, feature, ',')) {
                        // Remove quotes and whitespace
                        feature.erase(std::remove(feature.begin(), feature.end(), '"'), feature.end());
                        feature.erase(std::remove(feature.begin(), feature.end(), ' '), feature.end());

                        if (!feature.empty()) {
                            DeviceSettingsHDMIIn::HDMIInGameFeatureList gameFeature;
                            gameFeature.gameFeature = feature;
                            features.push_back(gameFeature);
                            LOGINFO("GetSupportedGameFeaturesList: Added feature: '%s'", feature.c_str());
                        }
                    }
                }

                LOGINFO("GetSupportedGameFeaturesList: Parsed %zu features from HAL data", features.size());

                // Create iterator using the GameFeatureListIteratorImpl type already defined in dHdmiIn.h
                // This uses WPEFramework's standard iterator pattern with explicit interface template parameter
                //gameFeatureList = GameFeatureListIteratorImpl::Create<IHDMIInGameFeatureListIterator>(features);

                if (gameFeatureList != nullptr) {
                    LOGINFO("GetSupportedGameFeaturesList: Successfully created iterator with %zu features", features.size());
                    retCode = WPEFramework::Core::ERROR_NONE;

                    // Log all parsed features for debugging
                    LOGINFO("GetSupportedGameFeaturesList: Feature summary:");
                    for (size_t i = 0; i < features.size(); i++) {
                        LOGINFO("  Feature[%zu]: '%s'", i, features[i].gameFeature.c_str());
                    }
                } else {
                    // Empty feature list or RPC iterator allocation failure — treat as no features
                    LOGWARN("GetSupportedGameFeaturesList: iterator creation failed (features=%zu), returning empty list", features.size());
                    retCode = WPEFramework::Core::ERROR_NONE;
                    gameFeatureList = nullptr;
                }
            } catch (const std::exception& e) {
                LOGERR("GetSupportedGameFeaturesList: Exception while parsing features: %s", e.what());
                gameFeatureList = nullptr;
                retCode = WPEFramework::Core::ERROR_GENERAL;
            }
        } else {
            LOGERR("GetSupportedGameFeaturesList: dsGetSupportedGameFeaturesList failed with error: %d", dsResult);
            gameFeatureList = nullptr;
        }

        return retCode;
    }

    uint32_t SelectHDMIInPort(const HDMIInPort port, const bool requestAudioMix, const bool topMostPlane, const HDMIVideoPlaneType videoPlaneType) override
    {
        if (!s_aidlPorts.empty()) {
            LOGINFO("SelectHDMIInPort: on hold pending PlaneControl/AudioMixer (AIDL)");
            return WPEFramework::Core::ERROR_UNAVAILABLE;
        }
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        dsVideoPlaneType_t videoType = static_cast<dsVideoPlaneType_t>(videoPlaneType);
        if (dsHdmiInSelectPort(hdmiPort, requestAudioMix, videoType, topMostPlane) == dsERR_NONE) {
            LOGINFO("SelectHDMIInPort: port=%d, requestAudioMix=%s, topMostPlane=%s, videoPlaneType=%d", hdmiPort, requestAudioMix ? "true" : "false", topMostPlane ? "true" : "false", videoPlaneType);
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t ScaleHDMIInVideo(const HDMIInVideoRectangle videoPosition) override
    {
        if (!s_aidlPorts.empty()) {
            LOGINFO("ScaleHDMIInVideo: on hold pending PlaneControl (AIDL)");
            return WPEFramework::Core::ERROR_UNAVAILABLE;
        }
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        if (dsHdmiInScaleVideo(videoPosition.x, videoPosition.y, videoPosition.width, videoPosition.height) == dsERR_NONE) {
            LOGINFO("Successfully set the video position x=%d, y=%d, width=%d, height=%d",
                    videoPosition.x, videoPosition.y, videoPosition.width, videoPosition.height);
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t SelectHDMIZoomMode(const HDMIInVideoZoom zoomMode) override
    {
        if (!s_aidlPorts.empty()) {
            LOGINFO("SelectHDMIZoomMode: on hold pending PlaneControl (AIDL)");
            return WPEFramework::Core::ERROR_UNAVAILABLE;
        }
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsVideoZoom_t zoom = static_cast<dsVideoZoom_t>(zoomMode);
        if ((retCode = dsHdmiInSelectZoomMode(zoom)) == dsERR_NONE) {
            LOGINFO("Successfully set the zoom mode: %d", zoom);
            retCode = WPEFramework::Core::ERROR_NONE;
        } else {
            LOGINFO("Failed to select zoom %d and return errorcode %d", zoom, retCode);
        }
        return retCode;
    }

    static dsError_t getEDIDBytesInfo (dsHdmiInPort_t iHdmiPort, unsigned char *edid, int *length) {
        if (!s_aidlPorts.empty()) {
            sp<IHDMIInput> hi;
            {
                std::lock_guard<std::mutex> lk(s_aidlMutex);
                auto it = s_aidlPorts.find((int)iHdmiPort);
                if (it == s_aidlPorts.end() || !it->second.hdmiInput) return dsERR_INVALID_PARAM;
                hi = it->second.hdmiInput;
            }
            std::vector<uint8_t> edidVec;
            bool ok = false;
            if (!hi->getEDID(&edidVec, &ok).isOk() || !ok || edidVec.empty()) {
                LOGERR("getEDID failed for port %d", (int)iHdmiPort);
                return dsERR_GENERAL;
            }
            *length = (int)edidVec.size();
            memcpy(edid, edidVec.data(), *length);
            LOGINFO("getEDIDBytesInfo port %d len=%d (AIDL)", (int)iHdmiPort, *length);
            return dsERR_NONE;
        }
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsGetEDIDBytesInfo_t)(dsHdmiInPort_t iHdmiPort, unsigned char *edid, int *length);
        static dsGetEDIDBytesInfo_t dsGetEDIDBytesInfoFunc = 0;
        if (dsGetEDIDBytesInfoFunc == 0) {
            dsGetEDIDBytesInfoFunc = (dsGetEDIDBytesInfo_t)resolve(RDK_DSHAL_NAME, "dsGetEDIDBytesInfo");
            if(dsGetEDIDBytesInfoFunc == 0) {
                LOGERR("dsGetEDIDBytesInfo is not defined");
                eRet = dsERR_GENERAL;
            } else {
                LOGINFO("dsGetEDIDBytesInfo loaded");
            }
        }
        if (0 != dsGetEDIDBytesInfoFunc) {
            LOGINFO("Entering dsGetEDIDBytesInfoFunc");
            eRet = dsGetEDIDBytesInfoFunc (iHdmiPort, edid, length);
            LOGINFO("dsGetEDIDBytesInfoFunc eRet: %d data len: %d", eRet, *length);
        }
        return eRet;
    }

    uint32_t GetEdidBytes(const HDMIInPort port, const uint16_t edidBytesLength, uint8_t edidBytes[]) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        int length = static_cast<int>(edidBytesLength);
        LOGINFO("GetEdidBytes");
        if (getEDIDBytesInfo(hdmiPort, edidBytes, &length) == dsERR_NONE) {
            LOGINFO("GetEdidBytes: port=%d, edidBytesLength=%d, actualLength=%d", hdmiPort, edidBytesLength, length);
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    static dsError_t getHDMISPDInfo (dsHdmiInPort_t iHdmiPort, unsigned char *spd) {
        if (!s_aidlPorts.empty()) {
            sp<IHDMIInput> hi;
            {
                std::lock_guard<std::mutex> lk(s_aidlMutex);
                auto it = s_aidlPorts.find((int)iHdmiPort);
                if (it == s_aidlPorts.end() || !it->second.hdmiInput) return dsERR_INVALID_PARAM;
                hi = it->second.hdmiInput;
            }
            std::vector<uint8_t> spdVec;
            if (!hi->getSPDInfoFrame(&spdVec).isOk() || spdVec.empty()) {
                LOGERR("getSPDInfoFrame failed for port %d", (int)iHdmiPort);
                return dsERR_GENERAL;
            }
            memset(spd, 0, sizeof(struct dsSpd_infoframe_st));
            size_t copyLen = std::min(spdVec.size(), sizeof(struct dsSpd_infoframe_st));
            memcpy(spd, spdVec.data(), copyLen);
            LOGINFO("getHDMISPDInfo port %d len=%zu (AIDL)", (int)iHdmiPort, copyLen);
            return dsERR_NONE;
        }
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsGetHDMISPDInfo_t)(dsHdmiInPort_t iHdmiPort, unsigned char *data);
        static dsGetHDMISPDInfo_t dsGetHDMISPDInfoFunc = 0;
        if (dsGetHDMISPDInfoFunc == 0) {
            dsGetHDMISPDInfoFunc = (dsGetHDMISPDInfo_t)resolve(RDK_DSHAL_NAME, "dsGetHDMISPDInfo");
            if(dsGetHDMISPDInfoFunc == 0) {
                LOGERR("dsGetHDMISPDInfo is not defined");
                eRet = dsERR_GENERAL;
            } else {
                LOGINFO("dsGetHDMISPDInfo loaded");
            }
        }
        if (0 != dsGetHDMISPDInfoFunc) {
            eRet = dsGetHDMISPDInfoFunc (iHdmiPort, spd);
            LOGINFO("dsGetHDMISPDInfoFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("dsGetHDMISPDInfoFunc = %p", dsGetHDMISPDInfoFunc);
        }
        return eRet;
    }

    uint32_t GetHDMISPDInformation(const HDMIInPort port, const uint16_t spdBytesLength, uint8_t spdBytes[]) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);

        if (getHDMISPDInfo(hdmiPort, spdBytes) == dsERR_NONE) {
            LOGINFO("GetHDMISPDInformation: port=%d, spdBytesLength=%d", hdmiPort, spdBytesLength);
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t GetHDMIEdidVersion(const HDMIInPort port, HDMIInEdidVersion &edidVersion) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        int edidVer = 0;
        if (getEdidVersion(hdmiPort, &edidVer) == dsERR_NONE) {
            edidVersion = static_cast<HDMIInEdidVersion>(edidVer);
            LOGINFO("GetHDMIEdidVersion: port=%d, edidVersion=%d", hdmiPort, edidVer);
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t SetHDMIEdidVersion(const HDMIInPort port, const HDMIInEdidVersion edidVersion) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        tv_hdmi_edid_version_t edidVer = static_cast<tv_hdmi_edid_version_t>(edidVersion);
        if (setEdidVersion(hdmiPort, edidVer) == dsERR_NONE) {
            m_edidversion[hdmiPort] = edidVer;
            LOGINFO("SetHDMIEdidVersion: port=%d, edidVersion=%d", hdmiPort, edidVer);
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t GetHDMIVideoMode(HDMIVideoPortResolution &videoPortResolution) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        if (!s_aidlPorts.empty()) {
            int vic = 0;
            {
                std::lock_guard<std::mutex> lk(s_aidlMutex);
                if (s_aidlActivePort >= 0) {
                    auto it = s_aidlPorts.find(s_aidlActivePort);
                    if (it != s_aidlPorts.end()) vic = it->second.lastVIC;
                }
            }
            dsVideoPortResolution_t dsRes;
            aidlVicToRes(vic, dsRes);
            videoPortResolution.name             = "";
            videoPortResolution.pixelResolution  = static_cast<HDMIInVideoResolution>(dsRes.pixelResolution);
            videoPortResolution.aspectRatio      = static_cast<HDMIVideoAspectRatio>(dsRes.aspectRatio);
            videoPortResolution.stereoScopicMode = static_cast<HDMIInVideoStereoScopicMode>(dsRes.stereoScopicMode);
            videoPortResolution.frameRate        = static_cast<HDMIInVideoFrameRate>(dsRes.frameRate);
            videoPortResolution.interlaced       = dsRes.interlaced;
            LOGINFO("GetHDMIVideoMode (AIDL): VIC=%d", vic);
            return WPEFramework::Core::ERROR_NONE;
        }
        dsVideoPortResolution_t videoRes;

        memset(&videoRes, 0, sizeof(videoRes));

        if (dsHdmiInGetCurrentVideoMode(&videoRes) == dsERR_NONE) {
            // Validate that we have reasonable data before logging
                LOGINFO("GetHDMIVideoMode: Raw HAL data - name=<invalid>, pixelRes=%u, aspectRatio=%u, stereoScopicMode=%u, frameRate=%u, interlaced=%d",
                        videoRes.pixelResolution, videoRes.aspectRatio, videoRes.stereoScopicMode, videoRes.frameRate, videoRes.interlaced);

            if (videoRes.name[0] != '\0' && strlen(videoRes.name) < sizeof(videoRes.name)) {
                videoPortResolution.name = std::string(videoRes.name);
            } else {
                videoPortResolution.name = "UNKNOWN";
                LOGWARN("GetHDMIVideoMode: Invalid video mode name, using 'UNKNOWN'");
            }

            videoPortResolution.pixelResolution = static_cast<HDMIInVideoResolution>(videoRes.pixelResolution);
            videoPortResolution.aspectRatio = static_cast<HDMIVideoAspectRatio>(videoRes.aspectRatio);
            videoPortResolution.stereoScopicMode = static_cast<HDMIInVideoStereoScopicMode>(videoRes.stereoScopicMode);
            videoPortResolution.frameRate = static_cast<HDMIInVideoFrameRate>(videoRes.frameRate);
            videoPortResolution.interlaced = videoRes.interlaced;

            // Debug print all the assigned data
            LOGINFO("GetHDMIVideoMode: Assigned data - name='%s', pixelResolution=%u, aspectRatio=%u, stereoScopicMode=%u, frameRate=%u, interlaced=%d", 
                    videoPortResolution.name.c_str(), 
                    videoPortResolution.pixelResolution,
                    videoPortResolution.aspectRatio,
                    videoPortResolution.stereoScopicMode,
                    videoPortResolution.frameRate,
                    videoPortResolution.interlaced);

            retCode = WPEFramework::Core::ERROR_NONE;
        } else {
            LOGERR("GetHDMIVideoMode: dsHdmiInGetCurrentVideoMode failed");
            // Initialize output with safe defaults
            videoPortResolution.name = "ERROR";
            videoPortResolution.pixelResolution = static_cast<HDMIInVideoResolution>(0);
            videoPortResolution.aspectRatio = static_cast<HDMIVideoAspectRatio>(0);
            videoPortResolution.stereoScopicMode = static_cast<HDMIInVideoStereoScopicMode>(0);
            videoPortResolution.frameRate = static_cast<HDMIInVideoFrameRate>(0);
            videoPortResolution.interlaced = false;
        }
        return retCode;
    }

    uint32_t GetHDMIVersion(const HDMIInPort port, HDMIInCapabilityVersion &capabilityVersion) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        dsHdmiMaxCapabilityVersion_t capversion;
        if (getHdmiVersion(hdmiPort, &capversion) == dsERR_NONE) {
            capabilityVersion = static_cast<HDMIInCapabilityVersion>(capversion);
            LOGINFO("GetHDMIVersion: port=%d, capabilityVersion=%d", hdmiPort, capversion);
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t SetVRRSupport(const HDMIInPort port, const bool vrrSupport) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        if (hdmiPort < dsHDMI_IN_PORT_MAX) {
            LOGINFO("In SetVRRSupport, checking m_edidversion of port %d : %d", hdmiPort, m_edidversion[hdmiPort]);
            if(m_edidversion[hdmiPort] == HDMI_EDID_VER_20) { // if the edidver is 2.0, then only set the vrr bit in edid
                if (setVRRSupport(hdmiPort, vrrSupport) == dsERR_NONE) {
                    updateVRRBitValuesInPersistence(hdmiPort, vrrSupport);
                    m_vrrsupport[hdmiPort] = vrrSupport;
                    LOGINFO("SetVRRSupport: port=%d, vrrSupport=%d", hdmiPort, vrrSupport);
                    retCode = WPEFramework::Core::ERROR_NONE;
                }
            } else {
                LOGINFO("EDID version is not 2.0, cannot set VRR support for port %d", hdmiPort);
                retCode = WPEFramework::Core::ERROR_UNAVAILABLE;
            }
        }
        return retCode;
    }

    uint32_t GetVRRSupport(const HDMIInPort port, bool &vrrSupport) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        if (hdmiPort < dsHDMI_IN_PORT_MAX) {
            vrrSupport = m_vrrsupport[hdmiPort];
            LOGINFO("GetVRRSupport: port=%d, vrrSupport=%d", hdmiPort, vrrSupport);
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    static dsError_t getVRRStatus (dsHdmiInPort_t iHdmiPort, dsHdmiInVrrStatus_t *vrrStatus) {
        if (!s_aidlPorts.empty()) {
            std::lock_guard<std::mutex> lk(s_aidlMutex);
            auto it = s_aidlPorts.find((int)iHdmiPort);
            if (it == s_aidlPorts.end()) return dsERR_INVALID_PARAM;
            const AidlPortCtx& ctx = it->second;
            vrrStatus->vrrType = ctx.vrrActive
                ? (ctx.vrrFrameRate > 0.0 ? dsVRR_AMD_FREESYNC : dsVRR_HDMI_VRR)
                : dsVRR_NONE;
            vrrStatus->vrrAmdfreesyncFramerate_Hz = ctx.vrrActive ? ctx.vrrFrameRate : 0.0;
            LOGINFO("getVRRStatus port %d type=%d (AIDL)", (int)iHdmiPort, vrrStatus->vrrType);
            return dsERR_NONE;
        }
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsHdmiInGetVRRStatus_t)(dsHdmiInPort_t iHdmiPort, dsHdmiInVrrStatus_t *vrrStatus);
        static dsHdmiInGetVRRStatus_t dsHdmiInGetVRRStatusFunc = 0;
        if (dsHdmiInGetVRRStatusFunc == 0) {
            dsHdmiInGetVRRStatusFunc = (dsHdmiInGetVRRStatus_t)resolve(RDK_DSHAL_NAME, "dsHdmiInGetVRRStatus");
            if(dsHdmiInGetVRRStatusFunc == 0) {
                LOGERR("dsHdmiInGetVRRStatus is not defined");
            } else {
                LOGINFO("dsHdmiInGetVRRStatus loaded");
            }
        }
        if (0 != dsHdmiInGetVRRStatusFunc) {
            eRet = dsHdmiInGetVRRStatusFunc (iHdmiPort, vrrStatus);
            LOGINFO("dsHdmiInGetVRRStatusFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("dsHdmiInGetVRRStatusFunc = %p", dsHdmiInGetVRRStatusFunc);
        }
        return eRet;
    }

    uint32_t GetVRRStatus(const HDMIInPort port, HDMIInVRRStatus &vrrStatus) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        dsHdmiInVrrStatus_t status;
        if (getVRRStatus(hdmiPort, &status) == dsERR_NONE) {
            vrrStatus.vrrType = static_cast<HDMIInVRRType>(status.vrrType);
            vrrStatus.vrrFreeSyncFramerateHz = status.vrrAmdfreesyncFramerate_Hz;
            LOGINFO("GetVRRStatus: port=%d, vrrType=%d, vrrFreeSyncFramerateHz=%f", hdmiPort, vrrStatus.vrrType, vrrStatus.vrrFreeSyncFramerateHz);
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    // Helper function to convert dsError_t to WPEFramework error codes
    static uint32_t convertDsErrorToWPEError(dsError_t dsErr) {
        switch (dsErr) {
            case dsERR_NONE:
                return WPEFramework::Core::ERROR_NONE;
            case dsERR_GENERAL:
                return WPEFramework::Core::ERROR_GENERAL;
            case dsERR_INVALID_PARAM:
                return WPEFramework::Core::ERROR_BAD_REQUEST;
            case dsERR_INVALID_STATE:
                return WPEFramework::Core::ERROR_ILLEGAL_STATE;
            case dsERR_OPERATION_NOT_SUPPORTED:
                return WPEFramework::Core::ERROR_UNAVAILABLE;
            default:
                return WPEFramework::Core::ERROR_GENERAL;
        }
    }

    private:
};
