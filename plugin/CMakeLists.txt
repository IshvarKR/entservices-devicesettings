# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2024 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

set(PLUGIN_NAME DeviceSettings)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
set(PLUGIN_IMPLEMENTATION ${MODULE_NAME}Imp)

set(PLUGIN_DEVICESETTINGS_AUTOSTART "true" CACHE STRING "Automatically start DeviceSettings plugin")
set(PLUGIN_DEVICESETTINGS_STARTUPORDER "15" CACHE STRING "To configure startup order of DeviceSettings plugin")
set(PLUGIN_DEVICESETTINGS_MODE "Local" CACHE STRING "Controls if the plugin should run in its own process, in process or remote")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(WPEFrameworkHelpers REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
find_library(PROCPS_LIBRARIES NAMES procps)
find_library(OEMHAL_LIBRARIES NAMES ds-hal)

add_library(${MODULE_NAME} SHARED
    Module.cpp
    DeviceSettings.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)
target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

add_library(${PLUGIN_IMPLEMENTATION} SHARED
    Module.cpp
    DeviceSettingsImplementation.cpp
    DeviceSettingsHALConfig.cpp
    DeviceSettingsFPDImplementation.cpp
    DeviceSettingsVideoPortImplementation.cpp
    DeviceSettingsVideoDeviceImplementation.cpp
    DeviceSettingsHdmiInImplementation.cpp
    DeviceSettingsAudioImplementation.cpp
    DeviceSettingsHostImplementation.cpp
    DeviceSettingsDisplayImplementation.cpp
    DeviceSettingsCompositeInImplementation.cpp
    fpd.cpp
    VideoPort.cpp
    VideoDevice.cpp
    HdmiIn.cpp
    Audio.cpp
    Host.cpp
    Display.cpp
    CompositeIn.cpp
    DSController.cpp
    DSPwrEventListener.cpp
    DSProductTraitsHandler.cpp
    )

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}
)

# DS HAL headers (dsUtl.h, dsError.h, dsTypes.h etc.) — previously pulled in by
# find_package(DS) via DS_INCLUDE_DIRS. Now resolved directly without the full DS package.
find_path(DSHAL_INCLUDE_DIRS NAMES dsTypes.h PATH_SUFFIXES rdk/halif/ds-hal)
if(NOT DSHAL_INCLUDE_DIRS)
    message(FATAL_ERROR "DS HAL headers not found (dsTypes.h). Check sysroot.")
endif()

# Add current directory to target include directories for proper header resolution
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${DSHAL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/../rdk-halif-device_settings/include
    ${CMAKE_SOURCE_DIR}/../devicesettings/ds/include
    ${GLIB2_INCLUDE_DIRS}
)

target_compile_definitions(${PLUGIN_IMPLEMENTATION} PRIVATE GLIB_AVAILABLE DS_AUDIO_SETTINGS_PERSISTENCE)

set_target_properties(${PLUGIN_IMPLEMENTATION} PROPERTIES
    CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES)

#if(RDK_SERVICES_L1_TEST OR RDK_SERVICE_L2_TEST)
#
#    target_compile_definitions(${PLUGIN_IMPLEMENTATION}
#                               PUBLIC
#                                 PLATCO_BOOTTO_STANDBY
#                                 ENABLE_THERMAL_PROTECTION
#                                 OFFLINE_MAINT_REBOOT)
#
#   find_library(TESTMOCKLIB_LIBRARIES NAMES TestMocklib)
#   if (TESTMOCKLIB_LIBRARIES)
#       message ("linking mock libraries ${TESTMOCKLIB_LIBRARIES} library")
#       target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${TESTMOCKLIB_LIBRARIES})
#   else (TESTMOCKLIB_LIBRARIES)
#       message ("Require ${TESTMOCKLIB_LIBRARIES} library")
#   endif ()
#endif ()

if(PROCPS_LIBRARIES)
    target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${PROCPS_LIBRARIES})
endif()

if (MFR_FOUND)
  target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${MFR_LIBRARIES})
  target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${MFR_INCLUDE_DIRS})
endif()

find_package(IARMBus REQUIRED)
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${IARMBUS_INCLUDE_DIRS})
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${IARMBUS_LIBRARIES})

find_path(AIDL_INCLUDE_DIR NAMES com/rdk/hal/hdmiinput/IHDMIInputManager.h
    PATH_SUFFIXES rdk-halif-aidl)
find_path(BINDER_INCLUDE_DIR NAMES binder/IServiceManager.h
    PATH_SUFFIXES binder_sdk android)
if(AIDL_INCLUDE_DIR AND BINDER_INCLUDE_DIR)
    target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE
        ${AIDL_INCLUDE_DIR}
        ${BINDER_INCLUDE_DIR})
else()
    message(FATAL_ERROR "AIDL or Binder headers not found for dHdmiInAIDLImpl")
endif()

find_library(BINDER_LIBRARY NAMES binder)
find_library(UTILS_LIBRARY NAMES utils)
find_library(HAL_AIDL_LIBRARY NAMES hal_aidl)
if(NOT BINDER_LIBRARY OR NOT UTILS_LIBRARY OR NOT HAL_AIDL_LIBRARY)
    message(FATAL_ERROR "binder/utils/hal_aidl libraries not found; hdmiinput AIDL symbols would stay unresolved")
endif()

# hal_aidl only supplies generated AIDL symbols, so keep it out of --as-needed pruning.
target_link_libraries(${PLUGIN_IMPLEMENTATION}
    PRIVATE
        -Wl,--no-as-needed
        ${HAL_AIDL_LIBRARY}
        -Wl,--as-needed
        ${BINDER_LIBRARY}
        ${UTILS_LIBRARY})

target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${DSHALSRV_LIBRARIES})
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${OEMHAL_LIBRARIES})

target_link_libraries(${PLUGIN_IMPLEMENTATION}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        WPEFrameworkHelpers::WPEFrameworkHelpers
        ${GLIB2_LIBRARIES})

install(TARGETS ${PLUGIN_IMPLEMENTATION}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
