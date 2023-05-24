/**
 * @file CameraUnit_ASI.hpp
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Interface for the ZWO ASI CameraUnit
 * @version 0.1
 * @date 2023-05-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __CAMERAUNIT_ASI_HPP__
#define __CAMERAUNIT_ASI_HPP__

#include "CameraUnit.hpp"
#include "ASICamera2.h"

#define LIBVENDOR "ZWO ASI"

#include <atomic>
#include <thread>
#include <memory>
#include <mutex>

#if (__cplusplus >= 202002L)
#error "C++20 is not supported."
#endif

#ifndef CCAMERAUNIT_ASI_DBG_LVL
#define CCAMERAUNIT_ASI_DBG_LVL CCAMERAUNIT_DBG_LVL
#endif

class CCameraUnit_ASI : public CCameraUnit
{
private:
    const char* vendor = LIBVENDOR;

    int cameraID;
    std::mutex camLock;
    std::atomic<bool> init_ok;
    std::atomic<double> exposure_;
    std::atomic<bool> capturing;
    std::shared_ptr<CImageData> image_data;

    char cam_name[100];
    std::string status_;

    int binningX_;
    int binningY_;

    int roiLeft;
    int roiRight;
    int roiTop;
    int roiBottom;

    mutable std::mutex roiLock;
    mutable ROI roi;

    int CCDWidth_;
    int CCDHeight_;

    double pixelSz = 0;
    bool hasShutter = false;
    bool hasCooler = false;
    bool isUSB3 = true;
    bool is8bitonly = false;

    double minExposure = 0;
    double maxExposure = 0;
    long minGain = 0;
    long maxGain = 0;

    float elecPerADU;
    int bitDepth;

    ASI_IMG_TYPE image_type;
    int supportedBins[16];

    ASI_CONTROL_CAPS *controlCaps[ASI_CONTROL_TYPE_END];

    std::thread captureThread;

public:
    static int ListCameras(int &num_cameras, int *&cameraIDs, std::string *&cameraNames);
    const void *GetHandle() const { return (void *)(uintptr_t) cameraID; }
    _Catchable CCameraUnit_ASI(int cameraID);
    ~CCameraUnit_ASI();

    inline const char *GetVendor() const { return vendor; }

    CImageData CaptureImage(bool blocking = true, CCameraUnitCallback callback_fn = nullptr, void *user_data = nullptr);
    const CImageData *GetLastImage() const;
    void CancelCapture();
    bool IsCapturing() const { return capturing; };

    inline bool CameraReady() const { return init_ok; }
    inline const char *CameraName() const { return cam_name; }
    void SetExposure(double exposureInSeconds);
    inline double GetExposure() const { return exposure_; }
    float GetGain() const;
    float SetGain(float gain);
    const double GetMinExposure() const { return minExposure; };
    const double GetMaxExposure() const { return maxExposure; };
    const float GetMinGain() const { return 0; };
    const float GetMaxGain() const { return 100; };
    inline void _NotImplemented SetShutterOpen(bool open) { return; };
    void SetTemperature(double temperatureInCelcius);
    double GetTemperature() const;
    void _Catchable SetBinningAndROI(int x, int y, int x_min = 0, int x_max = 0, int y_min = 0, int y_max = 0);
    inline int GetBinningX() const { return binningX_; }
    inline int GetBinningY() const { return binningY_; }
    const ROI *GetROI() const;
    inline std::string GetStatus() const { return status_; }
    inline int GetCCDWidth() const { return CCDWidth_; }
    inline int GetCCDHeight() const { return CCDHeight_; }
    
    inline double GetPixelSize() const { return pixelSz; }

private:
    static void CaptureThread(CCameraUnit_ASI *cam, CImageData *data = nullptr, CCameraUnitCallback callback_fn = nullptr, void *user_data = nullptr);
    static bool HasError(int error, unsigned int line);
};

#endif // __CAMERAUNIT_ASI_HPP__