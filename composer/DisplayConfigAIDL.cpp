/*
* Copyright (c) 2021 The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "DisplayConfigAIDL.h"

using ::aidl::android::hardware::common::NativeHandle;
using sdm::Locker;

namespace aidl {
namespace vendor {
namespace qti {
namespace hardware {
namespace display {
namespace config {

DisplayConfigAIDL::DisplayConfigAIDL() : hwc_session_(HWCSession::GetInstance()) {}

DisplayConfigAIDL::DisplayConfigAIDL(HWCSession *hwc_session) {
  if (!hwc_session) {
    ALOGE("%s: hwc_session:%p is invalid", __FUNCTION__, hwc_session);
  } else {
    hwc_session_ = hwc_session;
  }
}

int MapDisplayType(DisplayType dpy) {
  switch (dpy) {
    case DisplayType::PRIMARY:
      return qdutils::DISPLAY_PRIMARY;

    case DisplayType::EXTERNAL:
      return qdutils::DISPLAY_EXTERNAL;

    case DisplayType::VIRTUAL:
      return qdutils::DISPLAY_VIRTUAL;

    case DisplayType::BUILTIN2:
      return qdutils::DISPLAY_BUILTIN_2;

    default:
      break;
  }

  return -EINVAL;
}

sdm::HWCDisplay::DisplayStatus MapExternalStatus(ExternalStatus status) {
  switch (status) {
    case ExternalStatus::OFFLINE:
      return sdm::HWCDisplay::kDisplayStatusOffline;

    case ExternalStatus::ONLINE:
      return sdm::HWCDisplay::kDisplayStatusOnline;

    case ExternalStatus::PAUSE:
      return sdm::HWCDisplay::kDisplayStatusPause;

    case ExternalStatus::RESUME:
      return sdm::HWCDisplay::kDisplayStatusResume;

    default:
      break;
  }

  return sdm::HWCDisplay::kDisplayStatusInvalid;
}

bool WaitForResourceNeeded(sdm::PowerMode prev_mode, sdm::PowerMode new_mode) {
  return ((prev_mode == sdm::PowerMode::OFF) &&
          (new_mode == sdm::PowerMode::ON || new_mode == sdm::PowerMode::DOZE));
}

ScopedAStatus DisplayConfigAIDL::isDisplayConnected(DisplayType dpy, bool *connected) {
  int disp_id = MapDisplayType(dpy);
  int disp_idx = hwc_session_->GetDisplayIndex(disp_id);

  if (disp_idx == -1) {
    ALOGE("%s: Invalid display = %d", __FUNCTION__, disp_id);
    return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  } else {
    SEQUENCE_WAIT_SCOPE_LOCK(hwc_session_->locker_[disp_idx]);
    *connected = hwc_session_->hwc_display_[disp_idx];
  }

  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::setDisplayStatus(DisplayType dpy, ExternalStatus status) {
  int disp_id = MapDisplayType(dpy);
  sdm::HWCDisplay::DisplayStatus external_status = MapExternalStatus(status);

  if (hwc_session_->SetDisplayStatus(disp_id, external_status) != 0) {
    ALOGW("%s: Setting status:%d to display:%d failed", __FUNCTION__, status, disp_id);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::configureDynRefreshRate(DynRefreshRateOp op, int refresh_rate) {
  SEQUENCE_WAIT_SCOPE_LOCK(hwc_session_->locker_[HWC_DISPLAY_PRIMARY]);
  sdm::HWCDisplay *hwc_display = hwc_session_->hwc_display_[HWC_DISPLAY_PRIMARY];

  if (!hwc_display) {
    ALOGW("%s: Display = %d is not connected.", __FUNCTION__, HWC_DISPLAY_PRIMARY);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  switch (op) {
    case DynRefreshRateOp::DISABLE_METADATA:
      hwc_display->Perform(sdm::HWCDisplayBuiltIn::SET_METADATA_DYN_REFRESH_RATE, false);
      break;

    case DynRefreshRateOp::ENABLE_METADATA:
      hwc_display->Perform(sdm::HWCDisplayBuiltIn::SET_METADATA_DYN_REFRESH_RATE, true);
      break;

    case DynRefreshRateOp::SET_BINDER:
      hwc_display->Perform(sdm::HWCDisplayBuiltIn::SET_BINDER_DYN_REFRESH_RATE, refresh_rate);
      break;

    default:
      ALOGW("%s: Invalid operation %d", __FUNCTION__, op);
      return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::getConfigCount(DisplayType dpy, int *count) {
  int error = hwc_session_->GetConfigCount(MapDisplayType(dpy), (uint32_t *)count);
  if (error == -EINVAL) {
    ALOGW("%s: Failed to retrieve config count for display:%d", __FUNCTION__, dpy);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::getActiveConfig(DisplayType dpy, int *config) {
  int disp_id = MapDisplayType(dpy);

  int error = hwc_session_->GetActiveConfigIndex(disp_id, (uint32_t *)config);
  if (error == -EINVAL) {
    ALOGW("%s: Failed to retrieve the active config index for display:%d", __FUNCTION__, dpy);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::setActiveConfig(DisplayType dpy, int config) {
  int disp_id = MapDisplayType(dpy);

  if (hwc_session_->SetActiveConfigIndex(disp_id, (uint32_t)config) == -EINVAL) {
    ALOGW("%s: Failed to set active config index to display:%d", __FUNCTION__, dpy);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::getDisplayAttributes(int config_index, DisplayType dpy,
                                                      Attributes *attributes) {
  int error = -EINVAL;
  int disp_id = MapDisplayType(dpy);
  int disp_idx = hwc_session_->GetDisplayIndex(disp_id);

  if (disp_idx == -1) {
    ALOGW("%s: Invalid display = %d", __FUNCTION__, disp_id);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  } else {
    SEQUENCE_WAIT_SCOPE_LOCK(hwc_session_->locker_[disp_idx]);
    if (hwc_session_->hwc_display_[disp_idx]) {
      sdm::DisplayConfigVariableInfo var_info;
      error = hwc_session_->hwc_display_[disp_idx]->GetDisplayAttributesForConfig(INT(config_index),
                                                                                  &var_info);
      if (!error) {
        attributes->vsyncPeriod = var_info.vsync_period_ns;
        attributes->xRes = var_info.x_pixels;
        attributes->yRes = var_info.y_pixels;
        attributes->xDpi = var_info.x_dpi;
        attributes->yDpi = var_info.y_dpi;
        attributes->panelType = DisplayPortType::DEFAULT;
        attributes->isYuv = var_info.is_yuv;
      }
    }
  }

  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::setPanelBrightness(int level) {
  if (!(0 <= level && level <= 255)) {
    ALOGW("%s: Invalid panel brightness level :%d", __FUNCTION__, level);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  if (level == 0) {
    hwc_session_->SetDisplayBrightness(HWC_DISPLAY_PRIMARY, -1.0f);
  } else {
    hwc_session_->SetDisplayBrightness(HWC_DISPLAY_PRIMARY, (level - 1) / 254.0f);
  }
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::getPanelBrightness(int *level) {
  float brightness = -1.0f;

  hwc_session_->getDisplayBrightness(HWC_DISPLAY_PRIMARY, &brightness);
  if (brightness == -1.0f) {
    *level = 0;
  } else {
    *level = static_cast<uint32_t>(254.0f * brightness + 1);
  }

  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::minHdcpEncryptionLevelChanged(DisplayType dpy, int min_enc_level) {
  hwc_session_->MinHdcpEncryptionLevelChanged(MapDisplayType(dpy), min_enc_level);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::refreshScreen() {
  SEQUENCE_WAIT_SCOPE_LOCK(hwc_session_->locker_[HWC_DISPLAY_PRIMARY]);
  hwc_session_->callbacks_.Refresh(HWC_DISPLAY_PRIMARY);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::controlPartialUpdate(DisplayType dpy, bool enable) {
  hwc_session_->ControlPartialUpdate(MapDisplayType(dpy), enable);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::toggleScreenUpdate(bool on) {
  hwc_session_->ToggleScreenUpdate(on);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::setIdleTimeout(int value) {
  hwc_session_->SetIdleTimeout(value);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::getHDRCapabilities(DisplayType dpy, HDRCapsParams *caps) {
  int error = -EINVAL;

  do {
    int disp_id = MapDisplayType(dpy);
    int disp_idx = hwc_session_->GetDisplayIndex(disp_id);
    if (disp_idx == -1) {
      ALOGE("Invalid display = %d", disp_id);
      break;
    }

    SCOPE_LOCK(hwc_session_->locker_[disp_id]);
    sdm::HWCDisplay *hwc_display = hwc_session_->hwc_display_[disp_idx];
    if (!hwc_display) {
      ALOGW("Display = %d is not connected.", disp_idx);
      error = -ENODEV;
      break;
    }

    // query number of hdr types
    uint32_t out_num_types = 0;
    float out_max_luminance = 0.0f;
    float out_max_average_luminance = 0.0f;
    float out_min_luminance = 0.0f;
    if (hwc_display->GetHdrCapabilities(&out_num_types, nullptr, &out_max_luminance,
                                        &out_max_average_luminance,
                                        &out_min_luminance) != Error::None) {
      break;
    }
    if (!out_num_types) {
      error = 0;
      break;
    }

    // query hdr caps
    caps->supportedHdrTypes.resize(out_num_types);

    if (hwc_display->GetHdrCapabilities(&out_num_types, caps->supportedHdrTypes.data(),
                                        &out_max_luminance, &out_max_average_luminance,
                                        &out_min_luminance) == Error::None) {
      error = 0;
    }
  } while (false);

  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::setCameraLaunchStatus(int on) {
  hwc_session_->SetCameraLaunchStatus(on);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::displayBWTransactionPending(bool *status) {
  hwc_session_->DisplayBWTransactionPending(status);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::setDisplayAnimating(int64_t display_id, bool animating) {
  hwc_session_->CallDisplayFunction(display_id, &sdm::HWCDisplay::SetDisplayAnimating, animating);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::controlIdlePowerCollapse(bool enable, bool synchronous) {
  hwc_session_->ControlIdlePowerCollapse(enable, synchronous);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::getWriteBackCapabilities(bool *is_wb_ubwc_supported) {
  hwc_session_->IsWbUbwcSupported(is_wb_ubwc_supported);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::setDisplayDppsAdROI(int display_id, int h_start, int h_end,
                                                     int v_start, int v_end, int factor_in,
                                                     int factor_out) {
  hwc_session_->SetDisplayDppsAdROI(display_id, h_start, h_end, v_start, v_end, factor_in,
                                    factor_out);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::updateVSyncSourceOnPowerModeOff() {
  hwc_session_->update_vsync_on_power_off_ = true;
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::updateVSyncSourceOnPowerModeDoze() {
  hwc_session_->update_vsync_on_doze_ = true;
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::setPowerMode(int disp_id, PowerMode power_mode) {
  // This API is deprecated
  return ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ScopedAStatus DisplayConfigAIDL::isPowerModeOverrideSupported(int disp_id, bool *supported) {
  *supported = false;
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::isHDRSupported(int disp_id, bool *supported) {
  if (disp_id < 0 || disp_id >= sdm::HWCCallbacks::kNumDisplays) {
    ALOGW("%s: Not valid display", __FUNCTION__);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }
  SCOPE_LOCK(hwc_session_->hdr_locker_[disp_id]);

  if (hwc_session_->is_hdr_display_.size() <= disp_id) {
    ALOGW(
        "%s: is_hdr_display_ is not initialized for display %d!! Reporting it as HDR not "
        "supported",
        __FUNCTION__, disp_id);

    *supported = false;
    return ScopedAStatus::ok();
  }

  *supported = static_cast<bool>(hwc_session_->is_hdr_display_[disp_id]);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::isWCGSupported(int disp_id, bool *supported) {
  isHDRSupported(disp_id, supported);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::setLayerAsMask(int32_t disp_id, int64_t layer_id) {
  SCOPE_LOCK(hwc_session_->locker_[disp_id]);
  sdm::HWCDisplay *hwc_display = hwc_session_->hwc_display_[disp_id];
  if (!hwc_display) {
    ALOGW("%s: Display = %d is not connected.", __FUNCTION__, disp_id);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  if (hwc_session_->disable_mask_layer_hint_) {
    ALOGW("%s: Mask layer hint is disabled!", __FUNCTION__);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
  }

  auto hwc_layer = hwc_display->GetHWCLayer(layer_id);
  if (hwc_layer == nullptr) {
    ALOGW("%s: Failed to retrieve the hwc layer fpr display:%d", __FUNCTION__, disp_id);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  hwc_layer->SetLayerAsMask();
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::getDebugProperty(const std::string &prop_name,
                                                  std::string *value) {
  std::string vendor_prop_name = DISP_PROP_PREFIX;
  int error = -EINVAL;
  char val[64] = {};

  vendor_prop_name += prop_name.c_str();
  if (sdm::HWCDebugHandler::Get()->GetProperty(vendor_prop_name.c_str(), val) == sdm::kErrorNone) {
    *value = val;
    error = 0;
  }

  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::setClientUp() {
  hwc_session_->is_client_up_ = true;
  sdm::HWCDisplay *hwc_display = hwc_session_->hwc_display_[HWC_DISPLAY_PRIMARY];
  hwc_display->MarkClientActive(true);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::getActiveBuiltinDisplayAttributes(Attributes *attr) {
  int error = -EINVAL;
  Display disp_id = hwc_session_->GetActiveBuiltinDisplay();

  if (disp_id >= sdm::HWCCallbacks::kNumDisplays) {
    ALOGE("%s: Invalid display = %lu", __FUNCTION__, disp_id);
  } else {
    if (hwc_session_->hwc_display_[disp_id]) {
      uint32_t config_index = 0;
      Error ret = hwc_session_->hwc_display_[disp_id]->GetActiveConfig(&config_index);
      if (ret != Error::None) {
        return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
      }
      sdm::DisplayConfigVariableInfo var_info;
      error = hwc_session_->hwc_display_[disp_id]->GetDisplayAttributesForConfig(INT(config_index),
                                                                                 &var_info);
      if (!error) {
        attr->vsyncPeriod = var_info.vsync_period_ns;
        attr->xRes = var_info.x_pixels;
        attr->yRes = var_info.y_pixels;
        attr->xDpi = var_info.x_dpi;
        attr->yDpi = var_info.y_dpi;
        attr->panelType = DisplayPortType::DEFAULT;
        attr->isYuv = var_info.is_yuv;
      }
    }
  }

  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::setPanelLuminanceAttributes(int disp_id, float min_lum,
                                                             float max_lum) {
  // currently doing only for virtual display
  if (disp_id != static_cast<int>(DisplayType::VIRTUAL)) {
    ALOGW("%s: Setting panel luminance on non virtual display is not supported", __FUNCTION__);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  // check for out of range luminance values
  if (min_lum <= 0.0f || min_lum >= 1.0f || max_lum <= 100.0f || max_lum >= 1000.0f) {
    ALOGW("%s: Luminance values are out of range : minimum_luminance:%f maximum_luminance:%f",
          __FUNCTION__, min_lum, max_lum);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  std::lock_guard<std::mutex> obj(hwc_session_->mutex_lum_);
  hwc_session_->set_min_lum_ = min_lum;
  hwc_session_->set_max_lum_ = max_lum;
  ALOGI("%s: set max_lum %f, min_lum %f", __FUNCTION__, max_lum, min_lum);

  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::isBuiltInDisplay(int disp_id, bool *is_built_in) {
  if ((hwc_session_->map_info_primary_.client_id == disp_id) &&
      (hwc_session_->map_info_primary_.disp_type == sdm::kBuiltIn)) {
    *is_built_in = true;
    return ScopedAStatus::ok();
  }

  for (auto &info : hwc_session_->map_info_builtin_) {
    if (disp_id == info.client_id) {
      *is_built_in = true;
      return ScopedAStatus::ok();
    }
  }

  *is_built_in = false;
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::isAsyncVDSCreationSupported(bool *supported) {
  if (!hwc_session_->async_vds_creation_) {
    *supported = false;
    return ScopedAStatus::ok();
  }

  *supported = true;
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::createVirtualDisplay(int width, int height, int format) {
  if (!hwc_session_->async_vds_creation_) {
    ALOGW("%s: Asynchronous virtual display creation is not supported.", __FUNCTION__);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  if (!width || !height) {
    ALOGW("%s: Width and height provided are invalid.", __FUNCTION__);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  Display virtual_id = 0;
  Display active_builtin_disp_id = hwc_session_->GetActiveBuiltinDisplay();
  auto status = hwc_session_->CreateVirtualDisplayObj(width, height, &format, &virtual_id);
  if (status == Error::None) {
    ALOGI("%s, Created virtual display id:%" PRIu64 ", res: %dx%d", __FUNCTION__, virtual_id, width,
          height);

    if (active_builtin_disp_id < sdm::HWCCallbacks::kNumRealDisplays) {
      hwc_session_->WaitForResources(true, active_builtin_disp_id, virtual_id);
    }
  } else {
    ALOGE("%s: Failed to create virtual display: %s", __FUNCTION__, sdm::to_string(status).c_str());
  }

  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::getSupportedDSIBitClks(int32_t disp_id,
                                                        std::vector<int64_t> *bit_clks) {
  SCOPE_LOCK(hwc_session_->locker_[disp_id]);
  if (!hwc_session_->hwc_display_[disp_id]) {
    ALOGW("%s: Display:%d is not connected", __FUNCTION__, disp_id);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  hwc_session_->hwc_display_[disp_id]->GetSupportedDSIClock((std::vector<uint64_t> *)bit_clks);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::getDSIClk(int32_t disp_id, int64_t *bit_clk) {
  SCOPE_LOCK(hwc_session_->locker_[disp_id]);
  if (!hwc_session_->hwc_display_[disp_id]) {
    ALOGW("%s: Invalid display:%d", __FUNCTION__, disp_id);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  hwc_session_->hwc_display_[disp_id]->GetDynamicDSIClock((uint64_t *)bit_clk);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::setDSIClk(int32_t disp_id, int64_t bit_clk) {
  SCOPE_LOCK(hwc_session_->locker_[disp_id]);
  if (!hwc_session_->hwc_display_[disp_id]) {
    ALOGW("%s: Invalid display:%d", __FUNCTION__, disp_id);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  hwc_session_->hwc_display_[disp_id]->SetDynamicDSIClock(bit_clk);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::setQsyncMode(int disp_id, QsyncMode mode) {
  SEQUENCE_WAIT_SCOPE_LOCK(hwc_session_->locker_[disp_id]);
  if (!hwc_session_->hwc_display_[disp_id]) {
    ALOGW("%s: Invalid display:%d", __FUNCTION__, disp_id);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  sdm::QSyncMode qsync_mode = sdm::kQSyncModeNone;
  switch (mode) {
    case QsyncMode::NONE:
      qsync_mode = sdm::kQSyncModeNone;
      break;

    case QsyncMode::WAIT_FOR_FENCES_ONE_FRAME:
      qsync_mode = sdm::kQsyncModeOneShot;
      break;

    case QsyncMode::WAIT_FOR_FENCES_EACH_FRAME:
      qsync_mode = sdm::kQsyncModeOneShotContinuous;
      break;

    case QsyncMode::WAIT_FOR_COMMIT_EACH_FRAME:
      qsync_mode = sdm::kQSyncModeContinuous;
      break;
  }

  hwc_session_->hwc_display_[disp_id]->SetQSyncMode(qsync_mode);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::isSmartPanelConfig(int disp_id, int config_id, bool *is_smart) {
  SCOPE_LOCK(hwc_session_->locker_[disp_id]);
  if (!hwc_session_->hwc_display_[disp_id]) {
    ALOGE("%s: Display %d is not created yet.", __FUNCTION__, disp_id);
    *is_smart = false;
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  if (hwc_session_->hwc_display_[disp_id]->GetDisplayClass() != sdm::DISPLAY_CLASS_BUILTIN) {
    ALOGW("%s: Smart panel config is only supported on built in displays.", __FUNCTION__);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  *is_smart = hwc_session_->hwc_display_[disp_id]->IsSmartPanelConfig(config_id);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::isRotatorSupportedFormat(int hal_format, bool ubwc,
                                                          bool *supported) {
  if (!hwc_session_->core_intf_) {
    ALOGW("%s: core_intf_ not initialized.", __FUNCTION__);
    *supported = false;
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  int flag = ubwc ? qtigralloc::PRIV_FLAGS_UBWC_ALIGNED : 0;

  sdm::LayerBufferFormat sdm_format = sdm::HWCLayer::GetSDMFormat(hal_format, flag);

  *supported = hwc_session_->core_intf_->IsRotatorSupportedFormat(sdm_format);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::controlQsyncCallback(bool enable) {
  if (enable) {
    hwc_session_->qsync_callback_ = callback_;
  } else {
    hwc_session_->qsync_callback_.reset();
  }

  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::sendTUIEvent(DisplayType dpy, TUIEventType event_type) {
  int disp_id = MapDisplayType(dpy);
  int ret = hwc_session_->TUIEventHandler(disp_id, event_type);
  if (ret != 0) {
    ALOGW("TUIEventHandler failed with %d", ret);
    return ScopedAStatus(AStatus_fromServiceSpecificError(ret));
  }
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::getDisplayHwId(int disp_id, int *display_hw_id) {
  int disp_idx = hwc_session_->GetDisplayIndex(disp_id);
  if (disp_idx == -1) {
    ALOGE("%s: Invalid display = %d", __FUNCTION__, disp_id);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  SCOPE_LOCK(hwc_session_->locker_[disp_id]);
  if (!hwc_session_->hwc_display_[disp_idx]) {
    ALOGW("%s: Display %d is not connected.", __FUNCTION__, disp_id);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  // Supported for Built-In displays only.
  if ((hwc_session_->map_info_primary_.client_id == disp_id) &&
      (hwc_session_->map_info_primary_.disp_type == sdm::kBuiltIn)) {
    if (hwc_session_->map_info_primary_.sdm_id >= 0) {
      *display_hw_id = static_cast<uint32_t>(hwc_session_->map_info_primary_.sdm_id);
      return ScopedAStatus::ok();
    }
  }

  for (auto &info : hwc_session_->map_info_builtin_) {
    if (disp_id == info.client_id) {
      if (info.sdm_id >= 0) {
        *display_hw_id = static_cast<uint32_t>(info.sdm_id);
        return ScopedAStatus::ok();
      }
    }
  }

  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::getSupportedDisplayRefreshRates(
    DisplayType dpy, std::vector<int> *supported_refresh_rates) {
  hwc_session_->GetSupportedDisplayRefreshRates(MapDisplayType(dpy),
                                                (std::vector<uint32_t> *)supported_refresh_rates);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::isRCSupported(int disp_id, bool *supported) {
  // Mask layers can potentially be shown on any display so report RC supported on all displays if
  // the property enables the feature for use.
  int val = false;  // Default value.
  sdm::Debug::GetProperty(ENABLE_ROUNDED_CORNER, &val);
  *supported = val ? true : false;

  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::controlIdleStatusCallback(bool enable) {
  if (enable) {
    hwc_session_->enable_aidl_idle_notification_ = true;
  } else {
    hwc_session_->enable_aidl_idle_notification_ = false;
  }

  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::isSupportedConfigSwitch(int disp_id, int config, bool *supported) {
  if (!hwc_session_) {
    ALOGW("%s: Invalid hwc session:%p found.", __FUNCTION__, hwc_session_);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  int disp_idx = hwc_session_->GetDisplayIndex(disp_id);
  if (disp_idx == -1) {
    ALOGW("%s: Invalid display = %d", __FUNCTION__, disp_id);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  SCOPE_LOCK(hwc_session_->locker_[disp_idx]);
  if (!hwc_session_->hwc_display_[disp_idx]) {
    ALOGW("%s: Display %d is not connected.", __FUNCTION__, disp_id);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  *supported = hwc_session_->hwc_display_[disp_idx]->IsModeSwitchAllowed(config);
  return ScopedAStatus::ok();
}

DisplayType GetDisplayConfigDisplayType(int qdutils_disp_type) {
  switch (qdutils_disp_type) {
    case qdutils::DISPLAY_PRIMARY:
      return DisplayType::PRIMARY;

    case qdutils::DISPLAY_EXTERNAL:
      return DisplayType::EXTERNAL;

    case qdutils::DISPLAY_VIRTUAL:
      return DisplayType::VIRTUAL;

    case qdutils::DISPLAY_BUILTIN_2:
      return DisplayType::BUILTIN2;

    default:
      return DisplayType::INVALID;
  }
}

int DisplayConfigAIDL::GetDispTypeFromPhysicalId(uint64_t physical_disp_id,
                                                 DisplayType *disp_type) {
  // TODO(user): Least significant 8 bit is port id based on the SF current implementaion. Need to
  // revisit this if there is a change in logic to create physical display id in SF.
  int port_id = (physical_disp_id & 0xFF);
  int out_port = 0;
  for (int dpy = qdutils::DISPLAY_PRIMARY; dpy <= qdutils::DISPLAY_EXTERNAL_2; dpy++) {
    int ret = hwc_session_->GetDisplayPortId(dpy, &out_port);
    if (ret != 0) {
      return ret;
    }
    if (port_id == out_port) {
      *disp_type = GetDisplayConfigDisplayType(dpy);
      return 0;
    }
  }

  return -ENODEV;
}

ScopedAStatus DisplayConfigAIDL::getDisplayType(int64_t physical_disp_id,
                                                DisplayType *display_type) {
  if (!display_type) {
    ALOGW("%s: Display type provided is invalid.", __FUNCTION__);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  GetDispTypeFromPhysicalId(physical_disp_id, display_type);
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::setCWBOutputBuffer(
    const std::shared_ptr<IDisplayConfigCallback> &callback, int32_t disp_id, const Rect &rect,
    bool post_processed, const NativeHandle &buffer) {
  if (!callback) {
    ALOGE("%s: Callback provided is invalid.", __FUNCTION__);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  int dpy_index = -1;
  Display disp_type = HWC_DISPLAY_PRIMARY;
  if (disp_id == UINT32(DisplayType::PRIMARY)) {
    dpy_index = hwc_session_->GetDisplayIndex(qdutils::DISPLAY_PRIMARY);
  } else if (disp_id == UINT32(DisplayType::EXTERNAL)) {
    dpy_index = hwc_session_->GetDisplayIndex(qdutils::DISPLAY_EXTERNAL);
    disp_type = HWC_DISPLAY_EXTERNAL;
  } else if (disp_id == UINT32(DisplayType::BUILTIN2)) {
    dpy_index = hwc_session_->GetDisplayIndex(qdutils::DISPLAY_BUILTIN_2);
    disp_type = sdm::HWC_DISPLAY_BUILTIN_2;
  } else {
    ALOGE("%s: CWB is supported on primary or external display only at present.", __FUNCTION__);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  if (dpy_index == -1) {
    ALOGW("Unable to retrieve display index for display:%d", disp_id);
    return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
  }

  // Mutex scope
  {
    SCOPE_LOCK(hwc_session_->locker_[dpy_index]);
    if (!hwc_session_->hwc_display_[dpy_index]) {
      ALOGE("%s: Display is not created yet.", __FUNCTION__);
      return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
    }
  }

  sdm::CwbConfig cwb_config = {};
  cwb_config.tap_point = static_cast<sdm::CwbTapPoint>(post_processed);
  sdm::LayerRect &roi = cwb_config.cwb_roi;
  roi.left = FLOAT(rect.left);
  roi.top = FLOAT(rect.top);
  roi.right = FLOAT(rect.right);
  roi.bottom = FLOAT(rect.bottom);

  ALOGI("CWB config passed by cwb_client : tappoint %d  CWB_ROI : (%f %f %f %f)",
        cwb_config.tap_point, roi.left, roi.top, roi.right, roi.bottom);

  // TODO(user): Convert NativeHandle to native_handle_t, call PostBuffer

  int ret = hwc_session_->cwb_.PostBuffer(callback, cwb_config, ::android::dupFromAidl(buffer),
                                          disp_type, dpy_index);
  return ret == 0 ? ScopedAStatus::ok() : ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED);
}

ScopedAStatus DisplayConfigAIDL::setCameraSmoothInfo(CameraSmoothOp op, int32_t fps) {
  int ret = -1;

  if (fps < 0) {
    return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }

  ret = hwc_session_->SetCameraSmoothInfo(op, fps);

  return ret == 0 ? ScopedAStatus::ok() : ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED);
}

ScopedAStatus DisplayConfigAIDL::registerCallback(
    const std::shared_ptr<IDisplayConfigCallback> &callback, int64_t *client_handle) {
  int ret = -1;

  if (callback == nullptr) {
    return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }

  ret = hwc_session_->RegisterCallbackClient(callback, client_handle);

  return ret == 0 ? ScopedAStatus::ok() : ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED);
}

ScopedAStatus DisplayConfigAIDL::unRegisterCallback(int64_t client_handle) {
  int ret = -1;

  if (client_handle < 0) {
    return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }

  ret = hwc_session_->UnregisterCallbackClient(client_handle);

  return ret == 0 ? ScopedAStatus::ok() : ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED);
}

ScopedAStatus DisplayConfigAIDL::getDisplayPortId(int32_t disp_id, int32_t *port_id) {
  int ret = -1;

  if (!port_id) {
    return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }

  ret = hwc_session_->GetDisplayPortId(disp_id, port_id);

  return ret == 0 ? ScopedAStatus::ok() : ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED);
}

ScopedAStatus DisplayConfigAIDL::isCacV2Supported(int dispId, bool *supported) {
  return ScopedAStatus::ok();
}
ScopedAStatus DisplayConfigAIDL::configureCacV2(int32_t dispId, const CacV2Config &config,
                                                bool enable) {
  return ScopedAStatus::ok();
}
ScopedAStatus DisplayConfigAIDL::configureCacV2PerEye(int32_t dispId, const CacV2Config &leftConfig,
                                                      const CacV2Config &rightConfig, bool enable) {
  return ScopedAStatus::ok();
}
ScopedAStatus DisplayConfigAIDL::configureCacV2ExtPerEye(int32_t dispId,
                                                         const CacV2ConfigExt &leftConfig,
                                                         const CacV2ConfigExt &rightConfig,
                                                         bool enable) {
  return ScopedAStatus::ok();
}

ScopedAStatus DisplayConfigAIDL::allowIdleFallback() {
  SEQUENCE_WAIT_SCOPE_LOCK(hwc_session_->locker_[HWC_DISPLAY_PRIMARY]);
  uint32_t active_ms = 0;
  uint32_t inactive_ms = 0;
  sdm::Debug::GetIdleTimeoutMs(&active_ms, &inactive_ms);
  if (hwc_session_->hwc_display_[HWC_DISPLAY_PRIMARY]) {
    ALOGI("enable idle time active_ms:%d inactive_ms:%d", active_ms, inactive_ms);
    hwc_session_->hwc_display_[HWC_DISPLAY_PRIMARY]->SetIdleTimeoutMs(active_ms, inactive_ms);
    hwc_session_->is_client_up_ = true;
    hwc_session_->hwc_display_[HWC_DISPLAY_PRIMARY]->MarkClientActive(true);
    hwc_session_->idle_time_inactive_ms_ = inactive_ms;
    hwc_session_->idle_time_active_ms_ = active_ms;
    return ScopedAStatus::ok();
  }

  ALOGW("Display = %d is not connected.", HWC_DISPLAY_PRIMARY);
  return ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_STATE));
}

}  // namespace config
}  // namespace display
}  // namespace hardware
}  // namespace qti
}  // namespace vendor
}  // namespace aidl
