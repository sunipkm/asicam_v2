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

#include <atomic>
#include <thread>
#include <mutex>

#ifndef _Catchable
#define _Catchable
#endif

class CCameraUnit_ASI : public CCameraUnit
{
private:
    int cameraID;
    std::mutex camLock;
    std::atomic<bool> init_ok;
    char cam_name[100];
    std::atomic<float> exposure_;
    std::atomic<bool> cancel_capture;

    std::string status_;

    int binningX_;
    int binningY_;

    int imageLeft_;
    int imageRight_;
    int imageTop_;
    int imageBottom_;

    int roiLeft;
    int roiRight;
    int roiTop;
    int roiBottom;

    bool roi_updated_;

    int CCDWidth_;
    int CCDHeight_;

    double pixelSz;
    bool hasShutter;
    bool hasCooler;
    bool isUSB3;
    bool is8bitonly = false;

    float elecPerADU;
    int bitDepth;

    ASI_IMG_TYPE image_type;
    int supportedBins[16];

    ASI_CONTROL_CAPS *controlCaps[ASI_CONTROL_TYPE_END];

    std::thread captureThread;

public:
    static int ListCameras(int &num_cameras, int *&cameraIDs, std::string *&cameraNames);
    _Catchable CCameraUnit_ASI(int cameraID);
    ~CCameraUnit_ASI();

    CImageData CaptureImage(bool blocking);
    void CancelCapture();

    inline bool CameraReady() const { return init_ok; }
    inline const char *CameraName() const { return cam_name; }
    void SetExposure(float exposureInSeconds);
    inline float GetExposure() const { return exposure_; }
    void SetShutterIsOpen(bool open);
    void SetReadout(int ReadSpeed);
    void SetTemperature(double temperatureInCelcius);
    double GetTemperature() const;
    void SetBinningAndROI(int x, int y, int x_min = 0, int x_max = 0, int y_min = 0, int y_max = 0);
    inline int GetBinningX() const { return binningX_; }
    inline int GetBinningY() const { return binningY_; }
    const ROI *GetROI() const;
    inline std::string GetStatus() const { return status_; }
    inline int GetCCDWidth() const { return CCDWidth_; }
    inline int GetCCDHeight() const { return CCDHeight_; }
    
    inline double GetPixelSize() const { return pixelSz; }

private:
    static void CaptureThread(CCameraUnit_ASI *cam);
    static bool HasError(int error, unsigned int line);
};

#endif // __CAMERAUNIT_ASI_HPP__