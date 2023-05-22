/**
 * @file CCameraUnit_ASI.cpp
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Implementation for the ZWO ASI CameraUnit.
 * @version 0.1
 * @date 2023-05-18
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "CameraUnit_ASI.hpp"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <mutex>
#include <chrono>

#include "meb_print.h"

#if !defined(OS_Windows)
#include <unistd.h>
static inline void Sleep(int dwMilliseconds)
{
    usleep(dwMilliseconds * 1000);
}
#endif

static inline uint64_t getTime()
{
    return ((std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now())).time_since_epoch())).count());
}

bool CCameraUnit_ASI::HasError(int error, unsigned int line)
{
    switch (error)
    {
    default:
        fprintf(stderr, "%s, %d: Unknown ASI Error %d\n", __FILE__, line, error);
        fflush(stderr);
        return true;
    case ASI_SUCCESS:
        return false;
#define ASI_ERROR_EXPAND(x)                                             \
    case x:                                                             \
        fprintf(stderr, "%s, %d: ASI error: " #x "\n", __FILE__, line); \
        fflush(stderr);                                                 \
        return true;

        ASI_ERROR_EXPAND(ASI_ERROR_INVALID_INDEX)
        ASI_ERROR_EXPAND(ASI_ERROR_INVALID_ID)
        ASI_ERROR_EXPAND(ASI_ERROR_INVALID_CONTROL_TYPE)
        ASI_ERROR_EXPAND(ASI_ERROR_CAMERA_CLOSED)
        ASI_ERROR_EXPAND(ASI_ERROR_CAMERA_REMOVED)
        ASI_ERROR_EXPAND(ASI_ERROR_INVALID_PATH)
        ASI_ERROR_EXPAND(ASI_ERROR_INVALID_FILEFORMAT)
        ASI_ERROR_EXPAND(ASI_ERROR_INVALID_SIZE)
        ASI_ERROR_EXPAND(ASI_ERROR_INVALID_IMGTYPE)
        ASI_ERROR_EXPAND(ASI_ERROR_OUTOF_BOUNDARY)
        ASI_ERROR_EXPAND(ASI_ERROR_TIMEOUT)
        ASI_ERROR_EXPAND(ASI_ERROR_INVALID_SEQUENCE)
        ASI_ERROR_EXPAND(ASI_ERROR_BUFFER_TOO_SMALL)
        ASI_ERROR_EXPAND(ASI_ERROR_VIDEO_MODE_ACTIVE)
        ASI_ERROR_EXPAND(ASI_ERROR_EXPOSURE_IN_PROGRESS)
        ASI_ERROR_EXPAND(ASI_ERROR_GENERAL_ERROR)
        ASI_ERROR_EXPAND(ASI_ERROR_INVALID_MODE)
    }
#undef ASI_ERROR_EXPAND
}

#define HasError(x) HasError(x, __LINE__)

int CCameraUnit_ASI::ListCameras(int &num_cameras, int *&cameraIDs, std::string *&cameraNames)
{
    cameraIDs = nullptr;
    num_cameras = ASIGetNumOfConnectedCameras();
    if (num_cameras <= 0)
    {
        return -1;
    }
    cameraIDs = new int[num_cameras];
    cameraNames = new std::string[num_cameras];
    if (cameraIDs == nullptr)
    {
        num_cameras = 0;
        return -1;
    }
    for (int i = 0; i < num_cameras; i++)
    {
        ASI_CAMERA_INFO ASICameraInfo;
        int res = ASIGetCameraProperty(&ASICameraInfo, i);
        if (res != ASI_SUCCESS)
        {
            HasError(res);
            delete[] cameraIDs;
            num_cameras = 0;
            return -res;
        }
        cameraIDs[i] = ASICameraInfo.CameraID;
        cameraNames[i] = ASICameraInfo.Name;
    }
    return 0;
}

CCameraUnit_ASI::CCameraUnit_ASI(int cameraID)
{
    this->cameraID = cameraID;
    init_ok = false;
    exposure_ = 0.0;
    capturing = false;
    roi_updated_ = false;
    binningX_ = 1;
    binningY_ = 1;
    imageLeft_ = 0;
    imageRight_ = 0;
    imageTop_ = 0;
    imageBottom_ = 0;
    roiLeft = 0;
    roiRight = 0;
    roiTop = 0;
    roiBottom = 0;
    CCDWidth_ = 0;
    CCDHeight_ = 0;
    status_ = "Camera not initialized";
    cam_name[0] = '\0';


    if (HasError(ASIOpenCamera(cameraID)))
    {
        throw std::runtime_error("Could not open camera with ID " + std::to_string(cameraID));
    }

    ASI_CAMERA_INFO ASICameraInfo;
    if (HasError(ASIGetCameraProperty(&ASICameraInfo, cameraID)))
    {
        throw std::runtime_error("Could not get camera property for camera with ID " + std::to_string(cameraID));
    }

    memset(cam_name, 0, sizeof(cam_name));
    strncpy(cam_name, ASICameraInfo.Name, sizeof(cam_name) - 1);

    CCDHeight_ = ASICameraInfo.MaxHeight;
    CCDWidth_ = ASICameraInfo.MaxWidth;

    if (ASICameraInfo.IsColorCam)
    {
        throw std::runtime_error("Color cameras not supported");
    }

    bool supportedImage = false;
    bool support8bit_only = true;
    for (int i = 0; i < 8 && ASICameraInfo.SupportedVideoFormat[i] != ASI_IMG_END; i++)
    {
        if (ASICameraInfo.SupportedVideoFormat[i] == ASI_IMG_RAW16)
        {
            supportedImage = true;
            support8bit_only = false;
            break;
        }
        if (ASICameraInfo.SupportedVideoFormat[i] == ASI_IMG_RAW8)
        {
            supportedImage = true;
        }
    }

    if (!supportedImage)
    {
        throw std::runtime_error("Camera does not support RAW8 or RAW16");
    }

    if (support8bit_only)
    {
        image_type = ASI_IMG_RAW8;
        is8bitonly = true;
    }
    else
    {
        image_type = ASI_IMG_RAW16;
    }

    memcpy(supportedBins, ASICameraInfo.SupportedBins, sizeof(supportedBins));

    pixelSz = ASICameraInfo.PixelSize;
    hasShutter = ASICameraInfo.MechanicalShutter;
    hasCooler = ASICameraInfo.IsCoolerCam;
    isUSB3 = ASICameraInfo.IsUSB3Camera;
    elecPerADU = ASICameraInfo.ElecPerADU;
    bitDepth = ASICameraInfo.BitDepth; 

    int numControls = 0;
    if (HasError(ASIGetNumOfControls(cameraID, &numControls)))
    {
        throw std::runtime_error("Could not get number of controls for camera with ID " + std::to_string(cameraID));
    }

    memset(controlCaps, 0, sizeof(controlCaps));

    for (int i = 0; i < numControls; i++)
    {
        ASI_CONTROL_CAPS *controlCap = new ASI_CONTROL_CAPS;
        if (HasError(ASIGetControlCaps(cameraID, i, controlCap)))
        {
            throw std::runtime_error("Could not get control caps " + std::to_string(i) + " for camera with ID " + std::to_string(cameraID));
        }
        controlCaps[controlCap->ControlType] = controlCap;
    }

    if (HasError(ASIInitCamera(cameraID)))
    {
        throw std::runtime_error("Could not initialize camera with ID " + std::to_string(cameraID));
    }
    // set exposure to 1 ms
    if (HasError(ASISetControlValue(cameraID, ASI_EXPOSURE, 1000, ASI_FALSE)))
    {
        throw std::runtime_error("Could not set exposure for camera with ID " + std::to_string(cameraID));
    }
    exposure_ = 0.001;

    init_ok = true;
    status_ = "Camera initialized";
}

void CCameraUnit_ASI::CaptureThread(CCameraUnit_ASI *cam, CImageData *data)
{
    std::lock_guard<std::mutex> lock(cam->camLock);
    long exposure = (long) (cam->exposure_ * 1e6);
    ASI_EXPOSURE_STATUS status;
    if (HasError(ASIGetExpStatus(cam->cameraID, &status)))
    {
        cam->status_ = "Failed to get exposure status";
        cam->capturing = false;
        return;
    }
    if (status == ASI_EXP_WORKING)
    {
        cam->status_ = "Exposure already in progress";
        cam->capturing = false;
        return;
    }
    if (status == ASI_EXP_FAILED)
    {
        cam->status_ = "Last exposure attempt failed, restarting exposure";
    }
start_exposure:
    if (HasError(ASIStartExposure(cam->cameraID, ASI_FALSE)))
    {
        cam->status_ = "Failed to start exposure";
        cam->capturing = false;
        return;
    }
    cam->capturing = true;
    cam->status_ = "Exposure started, waiting for " + std::to_string(cam->exposure_) + " s";
    if (exposure < 1000) // < 1 ms
    {
        // busy wait
        while (!HasError(ASIGetExpStatus(cam->cameraID, &status)) && status == ASI_EXP_WORKING)
        {
            
        }
    }
    else if (exposure < 16000) // < 16 ms
    {
        while (!HasError(ASIGetExpStatus(cam->cameraID, &status)) && status == ASI_EXP_WORKING)
        {
            Sleep(1);
        }
    }
    else if (exposure < 1000000) // < 1 s
    {
        while (!HasError(ASIGetExpStatus(cam->cameraID, &status)) && status == ASI_EXP_WORKING)
        {
            Sleep(100);
        }
    }
    else
    {
        while (!HasError(ASIGetExpStatus(cam->cameraID, &status)) && status == ASI_EXP_WORKING)
        {
            Sleep(1000);
        }
    }
    if (status == ASI_EXP_FAILED)
    {
        cam->status_ = "Exposure failed";
        cam->capturing = false;
        return;
    }
    else if (status == ASI_EXP_IDLE)
    {
        cam->status_ = "Exposure was successful but no data is available.";
        cam->capturing = false;
        return;
    }
    else if (status == ASI_EXP_SUCCESS)
    {
        cam->status_ = "Exposure successful, downloading image";
        uint16_t *dataptr = new uint16_t[cam->CCDWidth_ * cam->CCDHeight_];
        if (HasError(ASIGetDataAfterExp(cam->cameraID, (unsigned char *) dataptr, cam->CCDWidth_ * cam->CCDHeight_ * sizeof(uint16_t))))
        {
            cam->status_ = "Failed to download image";
            cam->capturing = false;
            return;
        }
        *data = CImageData(cam->CCDWidth_, cam->CCDHeight_, dataptr, cam->exposure_);
        return;
    }
    else
    {
        cam->status_ = "Unknown exposure status";
        cam->capturing = false;
        return;
    }
}

CImageData CCameraUnit_ASI::CaptureImage(bool blocking)
{
    CImageData data;
    if (!init_ok)
    {
        throw std::runtime_error("Camera not initialized");
    }
    if (capturing)
    {
        return data;
    }
    

}