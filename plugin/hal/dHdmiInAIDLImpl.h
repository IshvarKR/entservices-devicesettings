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
#include <algorithm>
#include <functional>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <strings.h>
#include <map>
#include <mutex>
#include <string>

#include "dHdmiIn.h"
#include "dsHdmiIn.h"
#include "dsError.h"
#include "dsHdmiInTypes.h"
#include "dsVideoDeviceTypes.h"
#include "dsUtl.h"
#include "dsTypes.h"

#include "rfcapi.h"

#include <WPEFramework/interfaces/IDeviceSettingsHDMIIn.h>
#include "DeviceSettingsTypes.h"

#ifdef LOG_PRI
#undef LOG_PRI
#endif

#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <utils/StrongPointer.h>
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

class dHdmiInAIDLImpl : public hal::dHdmiIn::IPlatform {

    dHdmiInAIDLImpl(const dHdmiInAIDLImpl&) = delete;
private:

    // ---- Per-port AIDL runtime state ----
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

    // ---- Instance state (replacing file-scope statics from dHdmiInImpl) ----
    bool   m_hdmiInInitialized{false};
    bool   m_isDalsEnabled{false};
    dsHdmiInCap_t m_hdmiInCap{};
    bool   m_edidallmsupport[dsHDMI_IN_PORT_MAX]{};
    bool   m_vrrsupport[dsHDMI_IN_PORT_MAX]{};
    bool   m_hdmiPortVrrCaps[dsHDMI_IN_PORT_MAX]{};
    tv_hdmi_edid_version_t m_edidversion[dsHDMI_IN_PORT_MAX]{};

    // ---- Event callbacks (replacing g_Hdmi* file-scope statics) ----
    std::function<void(DeviceSettingsHDMIIn::HDMIInPort, bool)>                                           m_HotPlugCallback;
    std::function<void(DeviceSettingsHDMIIn::HDMIInPort, DeviceSettingsHDMIIn::HDMIInSignalStatus)>       m_SignalStatusCallback;
    std::function<void(DeviceSettingsHDMIIn::HDMIInPort, DeviceSettingsHDMIIn::HDMIVideoPortResolution)>  m_VideoModeUpdateCallback;
    std::function<void(DeviceSettingsHDMIIn::HDMIInPort, bool)>                                           m_AllmStatusCallback;
    std::function<void(DeviceSettingsHDMIIn::HDMIInPort, DeviceSettingsHDMIIn::HDMIInAviContentType)>     m_AviContentTypeCallback;
    std::function<void(int32_t, int32_t)>                                                                 m_AVLatencyCallback;
    std::function<void(DeviceSettingsHDMIIn::HDMIInPort, DeviceSettingsHDMIIn::HDMIInVRRType)>            m_VRRStatusCallback;
    std::function<void(DeviceSettingsHDMIIn::HDMIInPort, bool)>                                           m_StatusCallback;

    // ---- AIDL service/port state (replacing s_aidl* file-scope statics) ----
    sp<IHDMIInputManager>      m_aidlHdmiMgr;
    std::mutex                  m_aidlMutex;
    std::map<int, AidlPortCtx>  m_aidlPorts;
    int                         m_aidlActivePort{-1};
    uint8_t                     m_aidlPortCount{0};
    bool                        m_aidlPortArcCapable[dsHDMI_IN_PORT_MAX]{};

    // ---- Inner listener: per-port IHDMIInputController callbacks ----
    class CtrlListener : public ::com::rdk::hal::hdmiinput::BnHDMIInputControllerListener {
    public:
        CtrlListener(int portId, dHdmiInAIDLImpl* impl) : m_portId(portId), m_impl(impl) {}

        ::android::binder::Status onConnectionStateChanged(bool connected) override {
            {
                std::lock_guard<std::mutex> lk(m_impl->m_aidlMutex);
                auto it = m_impl->m_aidlPorts.find(m_portId);
                if (it != m_impl->m_aidlPorts.end()) it->second.connected = connected;
            }
            if (m_impl->m_HotPlugCallback)
                m_impl->m_HotPlugCallback(
                    static_cast<DeviceSettingsHDMIIn::HDMIInPort>(m_portId), connected);
            return ::android::binder::Status::ok();
        }

        ::android::binder::Status onSignalStateChanged(
                ::com::rdk::hal::hdmiinput::SignalState signalState) override {
            {
                std::lock_guard<std::mutex> lk(m_impl->m_aidlMutex);
                auto it = m_impl->m_aidlPorts.find(m_portId);
                if (it != m_impl->m_aidlPorts.end()) it->second.signalState = (int)signalState;
            }
            if (m_impl->m_SignalStatusCallback)
                m_impl->m_SignalStatusCallback(
                    static_cast<DeviceSettingsHDMIIn::HDMIInPort>(m_portId),
                    static_cast<DeviceSettingsHDMIIn::HDMIInSignalStatus>((int)signalState));
            return ::android::binder::Status::ok();
        }

        ::android::binder::Status onVIChanged(::com::rdk::hal::hdmiinput::VIC vic) override {
            {
                std::lock_guard<std::mutex> lk(m_impl->m_aidlMutex);
                auto it = m_impl->m_aidlPorts.find(m_portId);
                if (it != m_impl->m_aidlPorts.end()) it->second.lastVIC = (int)vic;
            }
            if (m_impl->m_VideoModeUpdateCallback) {
                dsVideoPortResolution_t dsRes;
                dHdmiInAIDLImpl::aidlVicToRes((int)vic, dsRes);
                DeviceSettingsHDMIIn::HDMIVideoPortResolution res;
                res.name             = "";
                res.pixelResolution  = static_cast<DeviceSettingsHDMIIn::HDMIInVideoResolution>(dsRes.pixelResolution);
                res.aspectRatio      = static_cast<DeviceSettingsHDMIIn::HDMIVideoAspectRatio>(dsRes.aspectRatio);
                res.stereoScopicMode = static_cast<DeviceSettingsHDMIIn::HDMIInVideoStereoScopicMode>(dsRes.stereoScopicMode);
                res.frameRate        = static_cast<DeviceSettingsHDMIIn::HDMIInVideoFrameRate>(dsRes.frameRate);
                res.interlaced       = dsRes.interlaced;
                m_impl->m_VideoModeUpdateCallback(
                    static_cast<DeviceSettingsHDMIIn::HDMIInPort>(m_portId), res);
            }
            return ::android::binder::Status::ok();
        }

        ::android::binder::Status onVRRChanged(
                bool vrrActive, bool /*mConst*/, bool /*fastV*/, double frameRate) override {
            dsVRRType_t vrrType = vrrActive
                ? (frameRate > 0.0 ? dsVRR_AMD_FREESYNC : dsVRR_HDMI_VRR)
                : dsVRR_NONE;
            {
                std::lock_guard<std::mutex> lk(m_impl->m_aidlMutex);
                auto it = m_impl->m_aidlPorts.find(m_portId);
                if (it != m_impl->m_aidlPorts.end()) {
                    it->second.vrrActive    = vrrActive;
                    it->second.vrrFrameRate = frameRate;
                }
            }
            if (m_impl->m_VRRStatusCallback)
                m_impl->m_VRRStatusCallback(
                    static_cast<DeviceSettingsHDMIIn::HDMIInPort>(m_portId),
                    static_cast<DeviceSettingsHDMIIn::HDMIInVRRType>(vrrType));
            return ::android::binder::Status::ok();
        }

        ::android::binder::Status onAVIInfoFrame(const std::vector<uint8_t>& data) override {
            dsAviContentType_t ct = dsAVICONTENT_TYPE_NOT_SIGNALLED;
            if (!dHdmiInAIDLImpl::aidlParseAviContentType(data, &ct))
                return ::android::binder::Status::ok();
            if (m_impl->m_AviContentTypeCallback)
                m_impl->m_AviContentTypeCallback(
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
        dHdmiInAIDLImpl* m_impl;
    };

    // ---- Inner listener: port-level state / EDID change events ----
    class EvtListener : public ::com::rdk::hal::hdmiinput::BnHDMIInputEventListener {
    public:
        EvtListener(int portId, dHdmiInAIDLImpl* impl) : m_portId(portId), m_impl(impl) {}

        ::android::binder::Status onStateChanged(
                ::com::rdk::hal::hdmiinput::State /*oldState*/,
                ::com::rdk::hal::hdmiinput::State newState) override {
            bool presented = (newState == ::com::rdk::hal::hdmiinput::State::STARTED);
            {
                std::lock_guard<std::mutex> lk(m_impl->m_aidlMutex);
                if (presented) m_impl->m_aidlActivePort = m_portId;
                else if (m_impl->m_aidlActivePort == m_portId) m_impl->m_aidlActivePort = -1;
            }
            if (m_impl->m_StatusCallback)
                m_impl->m_StatusCallback(
                    static_cast<DeviceSettingsHDMIIn::HDMIInPort>(m_portId), presented);
            return ::android::binder::Status::ok();
        }
        ::android::binder::Status onEDIDChange(const std::vector<uint8_t>&) override { return ::android::binder::Status::ok(); }
    private:
        int m_portId;
        dHdmiInAIDLImpl* m_impl;
    };

    // ---- Private AIDL helper methods ----

    sp<IHDMIInputManager> getAidlHdmiMgr()
    {
        std::lock_guard<std::mutex> lk(m_aidlMutex);
        if (!m_aidlHdmiMgr) {
            ProcessState::self()->startThreadPool();
            sp<android::IServiceManager> sm = defaultServiceManager();
            if (sm) {
                m_aidlHdmiMgr = interface_cast<IHDMIInputManager>(
                    sm->getService(String16(IHDMIInputManager::serviceName().c_str())));
            }
        }
        return m_aidlHdmiMgr;
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
        static const uint8_t kHdr[]          = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
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

    void aidlInit()
    {
        sp<IHDMIInputManager> mgr = getAidlHdmiMgr();
        if (!mgr) { LOGERR("IHDMIInputManager unavailable"); return; }

        std::vector<IHDMIInput::Id> portIds;
        if (!mgr->getHDMIInputIds(&portIds).isOk()) { LOGERR("getHDMIInputIds failed"); return; }
        m_aidlPortCount = (uint8_t)portIds.size();

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
                m_aidlPortArcCapable[portIdx] = caps.supportsARC;
                m_hdmiPortVrrCaps[portIdx]    = caps.supportsVRR;
            }

            ctx.evtListener = sp<EvtListener>::make(portIdx, this);
            bool regOk = false;
            hdmiInput->registerEventListener(ctx.evtListener, &regOk);

            ctx.ctrlListener = sp<CtrlListener>::make(portIdx, this);
            sp<IHDMIInputController> ctrl;
            if (hdmiInput->open(ctx.ctrlListener, &ctrl).isOk() && ctrl) {
                ctx.controller = ctrl;
                ctx.isOpen     = true;
            }

            std::lock_guard<std::mutex> lk(m_aidlMutex);
            m_aidlPorts[portIdx] = std::move(ctx);
            LOGINFO("AIDL port %d initialised", portIdx);
        }
    }

    void aidlTerm()
    {
        std::lock_guard<std::mutex> lk(m_aidlMutex);
        for (auto& kv : m_aidlPorts) {
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
        m_aidlPorts.clear();
        m_aidlHdmiMgr   = nullptr;
        m_aidlPortCount = 0;
    }

    void getDynamicAutoLatencyConfig()
    {
        RFC_ParamData_t param = {0};
        WDMP_STATUS status = getRFCParameter((char*)"dssrv", TVSETTINGS_DALS_RFC_PARAM, &param);
        LOGINFO("DALS Feature Enable = [ %s ]", param.value);
        if (WDMP_SUCCESS == status && (strncasecmp(param.value, "true", 4) == 0)) {
            m_isDalsEnabled = true;
            LOGINFO("Value of isDalsEnabled = [ %d ]", m_isDalsEnabled);
        } else {
            LOGERR("Fetching RFC for DALS failed or DALS is disabled: %d", status);
        }
    }

    bool getHdmiInPortPersistValue(const std::string& propertyName, int /*portIndex*/)
    {
        try {
            std::string value = device::HostPersistence::getInstance().getProperty(propertyName, "TRUE");
            bool support = (value == "TRUE");
            LOGINFO("Port property %s: Value: %s, Parsed: %d", propertyName.c_str(), value.c_str(), support);
            return support;
        } catch (...) {
            LOGERR("Port property %s: Exception getting property, using default TRUE", propertyName.c_str());
            return true;
        }
    }

    void updateEdidAllmBitValuesInPersistence(dsHdmiInPort_t iHdmiPort, bool allmSupport)
    {
        switch (iHdmiPort) {
            case dsHDMI_IN_PORT_0:
                device::HostPersistence::getInstance().persistHostProperty("HDMI0.edidallmEnable", allmSupport ? "TRUE" : "FALSE");
                LOGINFO("Port HDMI0: Persist EDID Allm Bit: %d", allmSupport); break;
            case dsHDMI_IN_PORT_1:
                device::HostPersistence::getInstance().persistHostProperty("HDMI1.edidallmEnable", allmSupport ? "TRUE" : "FALSE");
                LOGINFO("Port HDMI1: Persist EDID Allm Bit: %d", allmSupport); break;
            case dsHDMI_IN_PORT_2:
                device::HostPersistence::getInstance().persistHostProperty("HDMI2.edidallmEnable", allmSupport ? "TRUE" : "FALSE");
                LOGINFO("Port HDMI2: Persist EDID Allm Bit: %d", allmSupport); break;
            case dsHDMI_IN_PORT_3:
                device::HostPersistence::getInstance().persistHostProperty("HDMI3.edidallmEnable", allmSupport ? "TRUE" : "FALSE");
                LOGINFO("Port HDMI3: Persist EDID Allm Bit: %d", allmSupport); break;
            default:
                LOGWARN("Invalid HDMI port %d for ALLM persistence update", iHdmiPort); break;
        }
    }

    void updateVRRBitValuesInPersistence(dsHdmiInPort_t iHdmiPort, bool vrrSupport)
    {
        switch (iHdmiPort) {
            case dsHDMI_IN_PORT_0:
                device::HostPersistence::getInstance().persistHostProperty("HDMI0.vrrEnable", vrrSupport ? "TRUE" : "FALSE");
                LOGINFO("Port HDMI0: Persist EDID VRR Bit: %d", vrrSupport); break;
            case dsHDMI_IN_PORT_1:
                device::HostPersistence::getInstance().persistHostProperty("HDMI1.vrrEnable", vrrSupport ? "TRUE" : "FALSE");
                LOGINFO("Port HDMI1: Persist EDID VRR Bit: %d", vrrSupport); break;
            case dsHDMI_IN_PORT_2:
                device::HostPersistence::getInstance().persistHostProperty("HDMI2.vrrEnable", vrrSupport ? "TRUE" : "FALSE");
                LOGINFO("Port HDMI2: Persist EDID VRR Bit: %d", vrrSupport); break;
            case dsHDMI_IN_PORT_3:
                device::HostPersistence::getInstance().persistHostProperty("HDMI3.vrrEnable", vrrSupport ? "TRUE" : "FALSE");
                LOGINFO("Port HDMI3: Persist EDID VRR Bit: %d", vrrSupport); break;
            default:
                LOGWARN("Invalid HDMI port %d for VRR persistence update", iHdmiPort); break;
        }
    }

public:
    dHdmiInAIDLImpl()
    {
        LOGINFO("dHdmiInAIDLImpl Constructor");
        InitialiseHAL();
    }

    virtual ~dHdmiInAIDLImpl()
    {
        LOGINFO("dHdmiInAIDLImpl Destructor");
        DeInitialiseHAL();
    }

    void InitialiseHAL()
    {
        getDynamicAutoLatencyConfig();
        profileType = searchRdkProfile();
        LOGINFO("profileType %d", profileType);

        if (TV == profileType) {
            aidlInit();
            LOGINFO("dHdmiInAIDLImpl: AIDL init complete, %d ports found", m_aidlPortCount);
        }
    }

    void DeInitialiseHAL()
    {
        LOGINFO("profileType %d", profileType);
        if (TV == profileType) {
            aidlTerm();
        }
    }

    void setAllCallbacks(const CallbackBundle bundle) override
    {
        ENTRY_LOG;
        LOGINFO("setAllCallbacks: profileType %d", profileType);
        if (!m_hdmiInInitialized && !m_aidlPorts.empty()) {
            if (TV == profileType) {
                // AIDL listeners registered in aidlInit; just store the callbacks.
                if (bundle.OnHDMIInHotPlugEvent)         m_HotPlugCallback         = bundle.OnHDMIInHotPlugEvent;
                if (bundle.OnHDMIInSignalStatusEvent)    m_SignalStatusCallback    = bundle.OnHDMIInSignalStatusEvent;
                if (bundle.OnHDMIInStatusEvent)          m_StatusCallback          = bundle.OnHDMIInStatusEvent;
                if (bundle.OnHDMIInVideoModeUpdateEvent) m_VideoModeUpdateCallback = bundle.OnHDMIInVideoModeUpdateEvent;
                if (bundle.OnHDMIInAllmStatusEvent)      m_AllmStatusCallback      = bundle.OnHDMIInAllmStatusEvent;
                if (bundle.OnHDMIInAVIContentTypeEvent)  m_AviContentTypeCallback  = bundle.OnHDMIInAVIContentTypeEvent;
                if (bundle.OnHDMIInAVLatencyEvent)       m_AVLatencyCallback       = bundle.OnHDMIInAVLatencyEvent;
                if (bundle.OnHDMIInVRRStatusEvent)       m_VRRStatusCallback       = bundle.OnHDMIInVRRStatusEvent;
            }
        }
        EXIT_LOG;
    }

    void getPersistenceValue() override
    {
        if (!m_hdmiInInitialized && !m_aidlPorts.empty()) {
            int itr = 0;

            for (itr = 0; itr < dsHDMI_IN_PORT_MAX; itr++) {
                m_hdmiInCap.isPortArcCapable[itr] = m_aidlPortArcCapable[itr];
            }

            m_edidallmsupport[dsHDMI_IN_PORT_0] = getHdmiInPortPersistValue("HDMI0.edidallmEnable", dsHDMI_IN_PORT_0);
            m_edidallmsupport[dsHDMI_IN_PORT_1] = getHdmiInPortPersistValue("HDMI1.edidallmEnable", dsHDMI_IN_PORT_1);
            m_edidallmsupport[dsHDMI_IN_PORT_2] = getHdmiInPortPersistValue("HDMI2.edidallmEnable", dsHDMI_IN_PORT_2);
            m_edidallmsupport[dsHDMI_IN_PORT_3] = getHdmiInPortPersistValue("HDMI3.edidallmEnable", dsHDMI_IN_PORT_3);

            m_vrrsupport[dsHDMI_IN_PORT_0] = getHdmiInPortPersistValue("HDMI0.vrrEnable", dsHDMI_IN_PORT_0);
            m_vrrsupport[dsHDMI_IN_PORT_1] = getHdmiInPortPersistValue("HDMI1.vrrEnable", dsHDMI_IN_PORT_1);
            m_vrrsupport[dsHDMI_IN_PORT_2] = getHdmiInPortPersistValue("HDMI2.vrrEnable", dsHDMI_IN_PORT_2);
            m_vrrsupport[dsHDMI_IN_PORT_3] = getHdmiInPortPersistValue("HDMI3.vrrEnable", dsHDMI_IN_PORT_3);

            auto loadEdidVer = [&](dsHdmiInPort_t port, const std::string& key) {
                try {
                    std::string val = device::HostPersistence::getInstance().getProperty(key);
                    m_edidversion[port] = static_cast<tv_hdmi_edid_version_t>(atoi(val.c_str()));
                } catch (...) {
                    try {
                        std::string val = device::HostPersistence::getInstance().getDefaultProperty(key);
                        m_edidversion[port] = static_cast<tv_hdmi_edid_version_t>(atoi(val.c_str()));
                    } catch (...) {
                        LOGERR("Port %s: Exception getting EDID version, defaulting to 2.0", key.c_str());
                        m_edidversion[port] = HDMI_EDID_VER_20;
                    }
                }
            };
            loadEdidVer(dsHDMI_IN_PORT_0, "HDMI0.edidversion");
            loadEdidVer(dsHDMI_IN_PORT_1, "HDMI1.edidversion");
            loadEdidVer(dsHDMI_IN_PORT_2, "HDMI2.edidversion");

            for (itr = 0; itr < dsHDMI_IN_PORT_MAX; itr++) {
                LOGINFO("Port HDMI%d: VRR capability: %d", itr, m_hdmiPortVrrCaps[itr]);
            }
            for (itr = 0; itr < dsHDMI_IN_PORT_MAX; itr++) {
                if (SetHDMIEdidVersion(static_cast<HDMIInPort>(itr), static_cast<HDMIInEdidVersion>(m_edidversion[itr])) == WPEFramework::Core::ERROR_NONE) {
                    LOGINFO("Port HDMI%d: Initialized EDID Version: %d", itr, m_edidversion[itr]);
                }
            }
            m_hdmiInInitialized = true;
        }
        LOGINFO("Set Callbacks");
    }

    virtual uint32_t GetHDMIInNumberOfInputs(int32_t& count) override
    {
        count = static_cast<int32_t>(m_aidlPortCount);
        LOGINFO("GetHDMIInNumberOfInputs: count=%d (AIDL)", count);
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetHDMIInStatus(HDMIInStatus& hdmiStatus, IHDMIInPortConnectionStatusIterator*& portConnectionStatus) override
    {
        std::vector<DeviceSettingsHDMIIn::HDMIPortConnectionStatus> portStatuses;
        {
            std::lock_guard<std::mutex> lk(m_aidlMutex);
            hdmiStatus.activePort  = static_cast<HDMIInPort>(m_aidlActivePort);
            hdmiStatus.isPresented = (m_aidlActivePort >= 0);
            for (int p = 0; p < dsHDMI_IN_PORT_MAX; p++) {
                DeviceSettingsHDMIIn::HDMIPortConnectionStatus ps;
                auto it = m_aidlPorts.find(p);
                ps.isPortConnected = (it != m_aidlPorts.end()) ? it->second.connected : false;
                portStatuses.push_back(ps);
            }
        }
        portConnectionStatus = WPEFramework::Core::Service<WPEFramework::RPC::IteratorType<IHDMIInPortConnectionStatusIterator>>::Create<IHDMIInPortConnectionStatusIterator>(portStatuses);
        LOGINFO("GetHDMIInStatus (AIDL): activePort=%d isPresented=%s", m_aidlActivePort, hdmiStatus.isPresented ? "true" : "false");
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t SelectHDMIInPort(const HDMIInPort port, const bool requestAudioMix, const bool topMostPlane, const HDMIVideoPlaneType videoPlaneType) override
    {
        LOGINFO("SelectHDMIInPort: on hold pending PlaneControl/AudioMixer (AIDL)");
        return WPEFramework::Core::ERROR_UNAVAILABLE;
    }

    uint32_t ScaleHDMIInVideo(const HDMIInVideoRectangle videoPosition) override
    {
        LOGINFO("ScaleHDMIInVideo: on hold pending PlaneControl (AIDL)");
        return WPEFramework::Core::ERROR_UNAVAILABLE;
    }

    uint32_t SelectHDMIZoomMode(const HDMIInVideoZoom zoomMode) override
    {
        LOGINFO("SelectHDMIZoomMode: on hold pending PlaneControl (AIDL)");
        return WPEFramework::Core::ERROR_UNAVAILABLE;
    }

    uint32_t GetSupportedGameFeaturesList(IHDMIInGameFeatureListIterator*& gameFeatureList) override
    {
        std::vector<sp<IHDMIInput>> inputs;
        {
            std::lock_guard<std::mutex> lk(m_aidlMutex);
            for (const auto& kv : m_aidlPorts)
                if (kv.second.hdmiInput) inputs.push_back(kv.second.hdmiInput);
        }
        if (inputs.empty()) { gameFeatureList = nullptr; return WPEFramework::Core::ERROR_GENERAL; }

        bool supAllm = false, supVrr = false, supFreeSync = false;
        for (const auto& hi : inputs) {
            Capabilities caps;
            if (!hi->getCapabilities(&caps).isOk()) { gameFeatureList = nullptr; return WPEFramework::Core::ERROR_GENERAL; }
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
        LOGINFO("GetSupportedGameFeaturesList count=%zu (AIDL)", features.size());

        std::vector<DeviceSettingsHDMIIn::HDMIInGameFeatureList> featuresList;
        for (const auto& f : features) {
            DeviceSettingsHDMIIn::HDMIInGameFeatureList gf;
            gf.gameFeature = f;
            featuresList.push_back(gf);
        }
        // TODO: create iterator — left pending type resolution as in dev/1
        // gameFeatureList = WPEFramework::Core::Service<...>::Create<IHDMIInGameFeatureListIterator>(featuresList);
        gameFeatureList = nullptr;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetHDMIInAVLatency(uint32_t& videoLatency, uint32_t& audioLatency) override
    {
        audioLatency = 0;
        videoLatency = 0;
        LOGINFO("GetHDMIInAVLatency: returning 0/0 (AIDL, no PlaneControl)");
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetHDMIInAllmStatus(const HDMIInPort port, bool& allmStatus) override
    {
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        sp<IHDMIInput> hi;
        {
            std::lock_guard<std::mutex> lk(m_aidlMutex);
            auto it = m_aidlPorts.find((int)hdmiPort);
            if (it == m_aidlPorts.end() || !it->second.hdmiInput) return WPEFramework::Core::ERROR_GENERAL;
            hi = it->second.hdmiInput;
        }
        Capabilities caps;
        if (!hi->getCapabilities(&caps).isOk()) return WPEFramework::Core::ERROR_GENERAL;
        allmStatus = caps.supportsALLM;
        LOGINFO("GetHDMIInAllmStatus: port=%d, allmStatus=%s (AIDL)", (int)hdmiPort, allmStatus ? "true" : "false");
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetHDMIInEdid2AllmSupport(const HDMIInPort port, bool& allmSupport) override
    {
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        if (hdmiPort < dsHDMI_IN_PORT_MAX) {
            allmSupport = m_edidallmsupport[hdmiPort];
            LOGINFO("GetHDMIInEdid2AllmSupport: port=%d, allmSupport=%s", (int)hdmiPort, allmSupport ? "true" : "false");
            return WPEFramework::Core::ERROR_NONE;
        }
        return WPEFramework::Core::ERROR_GENERAL;
    }

    uint32_t SetHDMIInEdid2AllmSupport(const HDMIInPort port, bool allmSupport) override
    {
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        if (hdmiPort < dsHDMI_IN_PORT_MAX) {
            LOGINFO("SetHDMIInEdid2AllmSupport: checking edidversion of port %d: %d", (int)hdmiPort, m_edidversion[hdmiPort]);
            if (m_edidversion[hdmiPort] == HDMI_EDID_VER_20) {
                updateEdidAllmBitValuesInPersistence(hdmiPort, allmSupport);
                m_edidallmsupport[hdmiPort] = allmSupport;
                LOGINFO("SetHDMIInEdid2AllmSupport: port=%d, allmSupport=%s (AIDL)", (int)hdmiPort, allmSupport ? "true" : "false");
                return WPEFramework::Core::ERROR_NONE;
            }
            LOGINFO("EDID version is not 2.0, cannot set ALLM support for port %d", (int)hdmiPort);
            return WPEFramework::Core::ERROR_UNAVAILABLE;
        }
        return WPEFramework::Core::ERROR_GENERAL;
    }

    uint32_t GetEdidBytes(const HDMIInPort port, const uint16_t edidBytesLength, uint8_t edidBytes[]) override
    {
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        sp<IHDMIInput> hi;
        {
            std::lock_guard<std::mutex> lk(m_aidlMutex);
            auto it = m_aidlPorts.find((int)hdmiPort);
            if (it == m_aidlPorts.end() || !it->second.hdmiInput) return WPEFramework::Core::ERROR_GENERAL;
            hi = it->second.hdmiInput;
        }
        std::vector<uint8_t> edidVec;
        bool ok = false;
        if (!hi->getEDID(&edidVec, &ok).isOk() || !ok || edidVec.empty()) {
            LOGERR("GetEdidBytes: getEDID failed for port %d", (int)hdmiPort);
            return WPEFramework::Core::ERROR_GENERAL;
        }
        int copyLen = std::min((int)edidVec.size(), (int)edidBytesLength);
        memcpy(edidBytes, edidVec.data(), copyLen);
        LOGINFO("GetEdidBytes: port=%d, len=%d (AIDL)", (int)hdmiPort, copyLen);
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetHDMISPDInformation(const HDMIInPort port, const uint16_t spdBytesLength, uint8_t spdBytes[]) override
    {
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        sp<IHDMIInput> hi;
        {
            std::lock_guard<std::mutex> lk(m_aidlMutex);
            auto it = m_aidlPorts.find((int)hdmiPort);
            if (it == m_aidlPorts.end() || !it->second.hdmiInput) return WPEFramework::Core::ERROR_GENERAL;
            hi = it->second.hdmiInput;
        }
        std::vector<uint8_t> spdVec;
        if (!hi->getSPDInfoFrame(&spdVec).isOk() || spdVec.empty()) {
            LOGERR("GetHDMISPDInformation: getSPDInfoFrame failed for port %d", (int)hdmiPort);
            return WPEFramework::Core::ERROR_GENERAL;
        }
        memset(spdBytes, 0, spdBytesLength);
        size_t copyLen = std::min(spdVec.size(), (size_t)spdBytesLength);
        memcpy(spdBytes, spdVec.data(), copyLen);
        LOGINFO("GetHDMISPDInformation: port=%d, len=%zu (AIDL)", (int)hdmiPort, copyLen);
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetHDMIEdidVersion(const HDMIInPort port, HDMIInEdidVersion& edidVersion) override
    {
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        sp<IHDMIInput> hi;
        {
            std::lock_guard<std::mutex> lk(m_aidlMutex);
            auto it = m_aidlPorts.find((int)hdmiPort);
            if (it == m_aidlPorts.end() || !it->second.hdmiInput) return WPEFramework::Core::ERROR_GENERAL;
            hi = it->second.hdmiInput;
        }
        std::vector<uint8_t> edidVec;
        bool ok = false;
        if (!hi->getEDID(&edidVec, &ok).isOk() || !ok || edidVec.size() < 128)
            return WPEFramework::Core::ERROR_GENERAL;
        edidVersion = static_cast<HDMIInEdidVersion>(aidlGetEdidVersion(edidVec));
        LOGINFO("GetHDMIEdidVersion: port=%d, version=%d (AIDL)", (int)hdmiPort, (int)edidVersion);
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t SetHDMIEdidVersion(const HDMIInPort port, const HDMIInEdidVersion edidVersion) override
    {
        dsHdmiInPort_t hdmiPort    = static_cast<dsHdmiInPort_t>(port);
        tv_hdmi_edid_version_t ver = static_cast<tv_hdmi_edid_version_t>(edidVersion);

        HDMIVersion aidlVersion;
        if (!aidlMapEdidVersion(ver, &aidlVersion)) return WPEFramework::Core::ERROR_INVALID_PARAMETER;

        sp<IHDMIInput> hi;
        sp<IHDMIInputController> ctrl;
        {
            std::lock_guard<std::mutex> lk(m_aidlMutex);
            auto it = m_aidlPorts.find((int)hdmiPort);
            if (it == m_aidlPorts.end() || !it->second.hdmiInput || !it->second.controller)
                return WPEFramework::Core::ERROR_GENERAL;
            hi   = it->second.hdmiInput;
            ctrl = it->second.controller;
        }
        Capabilities caps;
        if (!hi->getCapabilities(&caps).isOk()) return WPEFramework::Core::ERROR_GENERAL;
        if (!aidlEdidVersionSupported(caps, aidlVersion)) return WPEFramework::Core::ERROR_UNAVAILABLE;

        std::vector<uint8_t> edidVec;
        bool ok = false;
        if (!hi->getDefaultEDID(aidlVersion, &edidVec, &ok).isOk() || !ok || edidVec.size() < 128)
            return WPEFramework::Core::ERROR_GENERAL;
        ok = false;
        if (!ctrl->setEDID(edidVec, &ok).isOk() || !ok) return WPEFramework::Core::ERROR_GENERAL;

        int port_no = (int)hdmiPort;
        if (port_no >= 0 && port_no < dsHDMI_IN_PORT_MAX) {
            std::string key = "HDMI" + std::to_string(port_no) + ".edidversion";
            device::HostPersistence::getInstance().persistHostProperty(key, std::to_string(static_cast<int>(ver)));
            m_edidversion[port_no] = ver;
        }
        LOGINFO("SetHDMIEdidVersion: port=%d, version=%d (AIDL)", (int)hdmiPort, ver);
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetHDMIVideoMode(HDMIVideoPortResolution& videoPortResolution) override
    {
        int vic = 0;
        {
            std::lock_guard<std::mutex> lk(m_aidlMutex);
            if (m_aidlActivePort >= 0) {
                auto it = m_aidlPorts.find(m_aidlActivePort);
                if (it != m_aidlPorts.end()) vic = it->second.lastVIC;
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

    uint32_t GetHDMIVersion(const HDMIInPort port, HDMIInCapabilityVersion& capabilityVersion) override
    {
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        sp<IHDMIInput> hi;
        {
            std::lock_guard<std::mutex> lk(m_aidlMutex);
            auto it = m_aidlPorts.find((int)hdmiPort);
            if (it == m_aidlPorts.end() || !it->second.hdmiInput) return WPEFramework::Core::ERROR_GENERAL;
            hi = it->second.hdmiInput;
        }
        Capabilities caps;
        if (!hi->getCapabilities(&caps).isOk()) return WPEFramework::Core::ERROR_GENERAL;
        dsHdmiMaxCapabilityVersion_t capversion = HDMI_COMPATIBILITY_VERSION_14;
        for (const auto& v : caps.supportedVersions) {
            if (v == HDMIVersion::HDMI_2_1 && capversion < HDMI_COMPATIBILITY_VERSION_21)
                capversion = HDMI_COMPATIBILITY_VERSION_21;
            else if (v == HDMIVersion::HDMI_2_0 && capversion < HDMI_COMPATIBILITY_VERSION_20)
                capversion = HDMI_COMPATIBILITY_VERSION_20;
        }
        capabilityVersion = static_cast<HDMIInCapabilityVersion>(capversion);
        LOGINFO("GetHDMIVersion: port=%d, version=%d (AIDL)", (int)hdmiPort, capversion);
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t SetVRRSupport(const HDMIInPort port, const bool vrrSupport) override
    {
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        if (hdmiPort < dsHDMI_IN_PORT_MAX) {
            LOGINFO("SetVRRSupport: checking edidversion of port %d: %d", (int)hdmiPort, m_edidversion[hdmiPort]);
            if (m_edidversion[hdmiPort] == HDMI_EDID_VER_20) {
                updateVRRBitValuesInPersistence(hdmiPort, vrrSupport);
                m_vrrsupport[hdmiPort] = vrrSupport;
                LOGINFO("SetVRRSupport: port=%d, vrrSupport=%d (AIDL)", (int)hdmiPort, vrrSupport);
                return WPEFramework::Core::ERROR_NONE;
            }
            LOGINFO("EDID version is not 2.0, cannot set VRR support for port %d", (int)hdmiPort);
            return WPEFramework::Core::ERROR_UNAVAILABLE;
        }
        return WPEFramework::Core::ERROR_GENERAL;
    }

    uint32_t GetVRRSupport(const HDMIInPort port, bool& vrrSupport) override
    {
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        if (hdmiPort < dsHDMI_IN_PORT_MAX) {
            vrrSupport = m_vrrsupport[hdmiPort];
            LOGINFO("GetVRRSupport: port=%d, vrrSupport=%d", (int)hdmiPort, vrrSupport);
            return WPEFramework::Core::ERROR_NONE;
        }
        return WPEFramework::Core::ERROR_GENERAL;
    }

    uint32_t GetVRRStatus(const HDMIInPort port, HDMIInVRRStatus& vrrStatus) override
    {
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        std::lock_guard<std::mutex> lk(m_aidlMutex);
        auto it = m_aidlPorts.find((int)hdmiPort);
        if (it == m_aidlPorts.end()) return WPEFramework::Core::ERROR_GENERAL;
        const AidlPortCtx& ctx = it->second;
        dsVRRType_t vrrType = ctx.vrrActive
            ? (ctx.vrrFrameRate > 0.0 ? dsVRR_AMD_FREESYNC : dsVRR_HDMI_VRR)
            : dsVRR_NONE;
        vrrStatus.vrrType = static_cast<HDMIInVRRType>(vrrType);
        vrrStatus.vrrFreeSyncFramerateHz = ctx.vrrActive ? ctx.vrrFrameRate : 0.0;
        LOGINFO("GetVRRStatus: port=%d, vrrType=%d (AIDL)", (int)hdmiPort, vrrType);
        return WPEFramework::Core::ERROR_NONE;
    }

    static bool IsAIDLAvailable()
    {
        LOGINFO("dHdmiInAIDLImpl: Checking AIDL HAL availability");
        try {
            ProcessState::self()->startThreadPool();
            sp<android::IServiceManager> sm = defaultServiceManager();
            if (!sm) return false;
            // checkService is non-blocking, unlike getService which waits with timeout.
            sp<android::IBinder> binder = sm->checkService(
                String16(IHDMIInputManager::serviceName().c_str()));
            bool available = (binder != nullptr);
            LOGINFO("dHdmiInAIDLImpl: AIDL service %s", available ? "available" : "not available");
            return available;
        } catch (...) {
            LOGERR("dHdmiInAIDLImpl: Exception checking AIDL availability");
            return false;
        }
    }
};
