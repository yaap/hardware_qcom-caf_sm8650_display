/*
 * SPDX-FileCopyrightText: 2025 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace android {
namespace hardware {
namespace graphics {
namespace common {
namespace V1_1 {
enum class RenderIntent : int32_t;
}
namespace V1_2 {
enum class ColorMode : int32_t;
}
}
}
}
}

namespace pxlw {

class PxlwIrisWrapper {
public:
    static PxlwIrisWrapper* GetInstance();
    static int GetIrisDisplayType(int param_1, int param_2);

private:
    PxlwIrisWrapper() = default;
};

// HW Iris 7 (AIDL-based) wrapper entry points exported by vendor libpwirishalwrapper.so.
// Only used when SUPPORTS_PXLW_IRIS7 is defined.
class PxlwIris7AidlWrapper {
public:
    int InitPrimaryDisplay(int param_1, unsigned int param_2, unsigned int param_3);
    void SetColorModeWithRenderIntent(int param_1, int param_2, int32_t param_3, int32_t param_4);

private:
    PxlwIris7AidlWrapper() = default;
    friend class PxlwIrisWrapper;
};

class PxlwSoftirisWrapper {
public:
    int InitPrimaryDisplay(int param_1, unsigned int param_2, unsigned int param_3);
    void SetColorModeWithRenderIntent(int param_1, int param_2, 
                                    android::hardware::graphics::common::V1_2::ColorMode param_3,
                                    android::hardware::graphics::common::V1_1::RenderIntent param_4);

private:
    PxlwSoftirisWrapper() = default;
    
    friend class PxlwIrisWrapper;
};

} // namespace pxlw
