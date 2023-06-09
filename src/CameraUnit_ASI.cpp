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

#if !defined(OS_Windows)
#include <unistd.h>
static inline void Sleep(int dwMilliseconds)
{
    usleep(dwMilliseconds * 1000);
}
#endif

#if !defined(OS_Windows)
#define YELLOW_FG "\033[33m"
#define RED_FG "\033[31m"
#define CYAN_FG "\033[36m"
#define RESET "\033[0m"
#else
#define YELLOW_FG
#define RED_FG
#define CYAN_FG
#define RESET
#endif

#if (CCAMERAUNIT_ASI_DBG_LVL >= 3)
#define CCAMERAUNIT_ASI_DBG_INFO(fmt, ...)                                                                   \
    {                                                                                                        \
        fprintf(stderr, "%s:%d:%s(): " CYAN_FG fmt RESET "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr);                                                                                      \
    }
#define CCAMERAUNIT_ASI_DBG_INFO_NONL(fmt, ...)                                                         \
    {                                                                                                   \
        fprintf(stderr, "%s:%d:%s(): " CYAN_FG fmt RESET, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr);                                                                                 \
    }
#define CCAMERAUNIT_ASI_DBG_INFO_NONE(fmt, ...)            \
    {                                                      \
        fprintf(stderr, CYAN_FG fmt RESET, ##__VA_ARGS__); \
        fflush(stderr);                                    \
    }
#else
#define CCAMERAUNIT_ASI_DBG_INFO(fmt, ...)
#define CCAMERAUNIT_ASI_DBG_INFO_NONL(fmt, ...)
#define CCAMERAUNIT_ASI_DBG_INFO_NONE(fmt, ...)
#endif

#if (CCAMERAUNIT_ASI_DBG_LVL >= 2)
#define CCAMERAUNIT_ASI_DBG_WARN(fmt, ...)                                                                     \
    {                                                                                                          \
        fprintf(stderr, "%s:%d:%s(): " YELLOW_FG fmt "\n" RESET, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr);                                                                                        \
    }
#define CCAMERAUNIT_ASI_DBG_WARN_NONL(fmt, ...)                                           \
    {                                                                                     \
        fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr);                                                                   \
    }
#define CCAMERAUNIT_ASI_DBG_WARN_NONE(fmt, ...) \
    {                                           \
        fprintf(stderr, fmt, ##__VA_ARGS__);    \
        fflush(stderr);                         \
    }
#else
#define CCAMERAUNIT_ASI_DBG_WARN(fmt, ...)
#define CCAMERAUNIT_ASI_DBG_WARN_NONL(fmt, ...)
#define CCAMERAUNIT_ASI_DBG_WARN_NONE(fmt, ...)
#endif

#if (CCAMERAUNIT_ASI_DBG_LVL >= 1)
#define CCAMERAUNIT_ASI_DBG_ERR(fmt, ...)                                                                   \
    {                                                                                                       \
        fprintf(stderr, "%s:%d:%s(): " RED_FG fmt "\n" RESET, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr);                                                                                     \
    }
#else
#define CCAMERAUNIT_ASI_DBG_ERR(fmt, ...)
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
        fprintf(stderr, "%s, %d: " RED_FG "Unknown ASI Error %d" RESET "\n", __FILE__, line, error);
        fflush(stderr);
        return true;
    case ASI_SUCCESS:
        return false;
#define ASI_ERROR_EXPAND(x)                                                          \
    case x:                                                                          \
        fprintf(stderr, "%s, %d: ASI error: " RED_FG #x RESET "\n", __FILE__, line); \
        fflush(stderr);                                                              \
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
        CCAMERAUNIT_ASI_DBG_ERR("No cameras found");
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
            delete[] cameraNames;
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
    binningX_ = 1;
    binningY_ = 1;
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

    if (controlCaps[ASI_GAIN])
    {
        minGain = controlCaps[ASI_GAIN]->MinValue;
        maxGain = controlCaps[ASI_GAIN]->MaxValue;
    }
    else
    {
        minGain = 0;
        maxGain = 0;
    }

    if (controlCaps[ASI_EXPOSURE])
    {
        minExposure = controlCaps[ASI_EXPOSURE]->MinValue * 1e-6;
        maxExposure = controlCaps[ASI_EXPOSURE]->MaxValue * 1e-6;
    }
    else
    {
        minExposure = 0.001;
        maxExposure = 200;
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

    if (HasError(ASISetROIFormat(cameraID, CCDWidth_, CCDHeight_, 1, image_type)))
    {
        throw std::runtime_error("Could not set ROI format for camera with ID " + std::to_string(cameraID));
    }

    roiLeft = 0;
    roiRight = CCDWidth_;
    roiTop = 0;
    roiBottom = CCDHeight_;
    binningX_ = 1;
    binningY_ = 1;

    exposure_ = 0.001;

    init_ok = true;
    status_ = "Camera initialized";
}

CCameraUnit_ASI::~CCameraUnit_ASI()
{
    if (init_ok)
    {
        CCAMERAUNIT_ASI_DBG_INFO("Closing camera");
        if (capturing)
        {
            CancelCapture();
        }
        if (captureThread.joinable())
        {
            captureThread.join();
        }
        ASICloseCamera(cameraID);
    }
}

void CCameraUnit_ASI::PrintCtrlCapInfo(ASI_CONTROL_TYPE ctrlType) const
{
    if (controlCaps[ctrlType] != nullptr)
    {
        fprintf(stderr, "Control type: %d [%s]", ctrlType, controlCaps[ctrlType]->Name);
        fprintf(stderr, "Description: %s\n", controlCaps[ctrlType]->Description);
        fprintf(stderr, "Min: %ld, Max: %ld, Default: %ld, IsAutoSupported: %d, IsWritable: %d\n",
                controlCaps[ctrlType]->MinValue, controlCaps[ctrlType]->MaxValue,
                controlCaps[ctrlType]->DefaultValue, controlCaps[ctrlType]->IsAutoSupported,
                controlCaps[ctrlType]->IsWritable);
    }
    else
    {
        fprintf(stderr, "Control type: %d [Not supported]\n", ctrlType);
    }
}

void CCameraUnit_ASI::CaptureThread(CCameraUnit_ASI *cam, CImageData *data, CCameraUnitCallback callback_fn, void *user_data)
{
    std::lock_guard<std::mutex> lock(cam->camLock);
    long exposure = (long)(cam->exposure_ * 1e6);
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
    uint64_t start_time = getTime();
    if (HasError(ASIStartExposure(cam->cameraID, cam->isDarkFrame ? ASI_TRUE : ASI_FALSE)))
    {
        cam->status_ = "Failed to start exposure";
        cam->capturing = false;
        return;
    }
    cam->capturing = true;
    cam->status_ = "Exposure started, waiting for " + std::to_string(cam->exposure_) + " s";
    if (exposure < 16000) // < 1 ms
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
    else // >= 1 s
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
        if (HasError(ASIGetDataAfterExp(cam->cameraID, (unsigned char *)dataptr, cam->CCDWidth_ * cam->CCDHeight_ * sizeof(uint16_t))))
        {
            cam->status_ = "Failed to download image";
            cam->capturing = false;
            return;
        }
        int iwid = (cam->roiRight - cam->roiLeft) / cam->binningX_;
        int ihei = (cam->roiBottom - cam->roiTop) / cam->binningY_;
        int imgleft = cam->roiLeft / cam->binningX_;
        int imgtop = cam->roiTop / cam->binningY_;
        CImageMetadata metadata;
        metadata.binX = cam->binningX_;
        metadata.binY = cam->binningY_;
        metadata.exposureTime = cam->exposure_;
        metadata.timestamp = start_time;
        metadata.temperature = cam->GetTemperature();
        metadata.cameraName = cam->cam_name;
        metadata.imgLeft = imgleft;
        metadata.imgTop = imgtop;
        metadata.gain = cam->GetGainRaw();
        metadata.offset = cam->GetOffset();
        metadata.minGain = cam->GetMinGain();
        metadata.maxGain = cam->GetMaxGain();
        CImageData *new_img = new CImageData(iwid, ihei, dataptr, metadata); // create new image data object
        cam->image_data = std::shared_ptr<CImageData>(new_img);              // store in shared pointer
        if (data != nullptr)
            *data = *new_img; // copy to output
        delete[] dataptr;
        cam->status_ = "Image downloaded";
        if (callback_fn != nullptr)
        {
            const ROI *roi = cam->GetROI();
            ROI roi_;
            memcpy(&roi_, roi, sizeof(ROI));
            callback_fn(new_img, roi_, user_data);
        }
        cam->capturing = false;
        return;
    }
    else
    {
        cam->status_ = "Unknown exposure status";
        cam->capturing = false;
        return;
    }
    cam->capturing = false;
    return;
}

const std::pair<bool, std::string> CCameraUnit_ASI::GetUUID() const
{
    if (!init_ok)
    {
        return std::pair<bool, std::string>(false, "");
    }
    if (!isUSB3)
    {
        CCAMERAUNIT_ASI_DBG_WARN("Only USB3 ZWO ASI Cameras have UUIDs");
        return std::pair<bool, std::string>(false, "");
    }
    ASI_ID id;
    if (HasError(ASIGetID(cameraID, &id)))
    {
        CCAMERAUNIT_ASI_DBG_ERR("Failed to get camera ID");
        return std::pair<bool, std::string>(false, "");
    }
    char uuid[9] = {
        0,
    };
    memcpy(uuid, id.id, 8);
    return std::pair<bool, std::string>(true, uuid);
}

const CImageData *CCameraUnit_ASI::GetLastImage() const
{
    if (!init_ok)
    {
        return nullptr;
    }
    return image_data.get();
}

float CCameraUnit_ASI::GetGain() const
{
    if (!init_ok)
    {
        CCAMERAUNIT_ASI_DBG_ERR("Camera not initialized");
        return 0;
    }

    long gain;
    ASI_BOOL bAuto;
    if (HasError(ASIGetControlValue(cameraID, ASI_GAIN, &gain, &bAuto)))
    {
        CCAMERAUNIT_ASI_DBG_ERR("Failed to get gain");
        return 0;
    }
    CCAMERAUNIT_ASI_DBG_INFO("Gain is %ld", gain);
    return ((float)(gain - minGain)) / ((maxGain - minGain) * 100.0);
}

float CCameraUnit_ASI::SetGain(float gain)
{
    if (!init_ok)
    {
        CCAMERAUNIT_ASI_DBG_ERR("Camera not initialized");
        return 0;
    }
    if (gain < 0 || gain > 100)
    {
        CCAMERAUNIT_ASI_DBG_ERR("Gain must be between 0 and 1");
        return 0;
    }
    long newGain = (long)(((gain * (maxGain - minGain)) / 100) + minGain);
    CCAMERAUNIT_ASI_DBG_INFO("Setting gain to %ld", newGain);
    std::lock_guard<std::mutex> lock(camLock);
    if (HasError(ASISetControlValue(cameraID, ASI_GAIN, newGain, ASI_FALSE)))
    {
        CCAMERAUNIT_ASI_DBG_ERR("Failed to set gain");
        return 0;
    }
    return GetGain();
}

long CCameraUnit_ASI::GetGainRaw() const
{
    if (!init_ok)
    {
        CCAMERAUNIT_ASI_DBG_ERR("Camera not initialized");
        return 0;
    }

    long gain;
    ASI_BOOL bAuto;
    if (HasError(ASIGetControlValue(cameraID, ASI_GAIN, &gain, &bAuto)))
    {
        CCAMERAUNIT_ASI_DBG_ERR("Failed to get gain");
        return 0;
    }
    CCAMERAUNIT_ASI_DBG_INFO("Gain is %ld", gain);
    return gain;
}

long CCameraUnit_ASI::SetGainRaw(long gain)
{
    if (!init_ok)
    {
        CCAMERAUNIT_ASI_DBG_ERR("Camera not initialized");
        return 0;
    }
    if (gain < minGain || gain > maxGain)
    {
        CCAMERAUNIT_ASI_DBG_ERR("Gain must be between %ld and %ld", minGain, maxGain);
        return 0;
    }
    CCAMERAUNIT_ASI_DBG_INFO("Setting gain to %ld", gain);
    std::lock_guard<std::mutex> lock(camLock);
    if (HasError(ASISetControlValue(cameraID, ASI_GAIN, gain, ASI_FALSE)))
    {
        CCAMERAUNIT_ASI_DBG_ERR("Failed to set gain");
        return 0;
    }
    return GetGainRaw();
}

CImageData CCameraUnit_ASI::CaptureImage(bool blocking, CCameraUnitCallback callback_fn, void *user_data)
{
    CImageData data;
    if (!init_ok)
    {
        CCAMERAUNIT_ASI_DBG_WARN("Camera not initialized");
        return data;
    }
    if (capturing)
    {
        CCAMERAUNIT_ASI_DBG_WARN("Already capturing");
        return data;
    }
    if (blocking)
    {
        captureThread = std::thread(CaptureThread, this, &data, nullptr, nullptr);
        captureThread.join();
        return data;
    }
    else
    {
        captureThread = std::thread(CaptureThread, this, nullptr, callback_fn, user_data);
        captureThread.detach();
        return data;
    }
}

void CCameraUnit_ASI::CancelCapture()
{
    if (capturing)
    {
        if (HasError(ASIStopExposure(cameraID)))
        {
            CCAMERAUNIT_ASI_DBG_INFO("Cancelled ongoing exposure");
        }
    }
}

void CCameraUnit_ASI::SetExposure(double exposureInSeconds)
{
    if (!init_ok)
    {
        return;
    }
    if (exposureInSeconds < minExposure)
    {
        CCAMERAUNIT_ASI_DBG_ERR("Exposure too short");
        return;
    }
    if (exposureInSeconds > maxExposure)
    {
        CCAMERAUNIT_ASI_DBG_ERR("Exposure too long");
        return;
    }
    std::lock_guard<std::mutex> lock(camLock);
    if (!HasError(ASISetControlValue(cameraID, ASI_EXPOSURE, (long)(exposureInSeconds * 1e6), ASI_FALSE)))
    {
        exposure_ = exposureInSeconds;
        status_ = "Set exposure to " + std::to_string(exposureInSeconds) + " s";
        return;
    }
    else
    {
        status_ = "Failed to set exposure";
        CCAMERAUNIT_ASI_DBG_WARN("Failed to set exposure");
    }
}

bool CCameraUnit_ASI::SetShutterOpen(bool open)
{
    if (!init_ok)
    {
        return true;
    }
    if (!hasShutter)
    {
        CCAMERAUNIT_ASI_DBG_WARN("Camera %s (%s): Does not have shutter", cam_name, GetUUID().second.c_str());
        return true;
    }
    isDarkFrame = !open;
    return GetShutterOpen();
}

bool CCameraUnit_ASI::GetShutterOpen() const
{
    if (!init_ok)
    {
        return true;
    }
    if (!hasShutter)
    {
        CCAMERAUNIT_ASI_DBG_WARN("Camera %s (%s): Does not have shutter", cam_name, GetUUID().second.c_str());
        return true;
    }
    return !isDarkFrame;
}

void CCameraUnit_ASI::SetTemperature(double temperatureInCelcius)
{
    if (!init_ok)
    {
        return;
    }
    if (!hasCooler)
    {
        CCAMERAUNIT_ASI_DBG_WARN("Camera %s (%s): Does not have cooler", cam_name, GetUUID().second.c_str());
        return;
    }
    if (temperatureInCelcius < -80)
    {
        return;
    }
    long tempval = (long)temperatureInCelcius;
    CCAMERAUNIT_ASI_DBG_INFO("Setting temperature to %lf -> %ld C", temperatureInCelcius, tempval);
    if (!HasError(ASISetControlValue(cameraID, ASI_TARGET_TEMP, tempval, ASI_FALSE)))
    {
        status_ = "Set cooler temperature to " + std::to_string(temperatureInCelcius);
    }
    else
    {
        status_ = "Failed to set temperature";
        CCAMERAUNIT_ASI_DBG_WARN("Failed to set temperature");
        return;
    }
#if (CCAMERAUNIT_ASI_DBG_LVL >= 3)
    {
        long val;
        ASI_BOOL bAuto;
        ASIGetControlValue(cameraID, ASI_TARGET_TEMP, &val, &bAuto);
        CCAMERAUNIT_ASI_DBG_INFO("Target temperature is %ld C", val);
    }
#endif
    if (!HasError(ASISetControlValue(cameraID, ASI_COOLER_ON, 1, ASI_FALSE)))
    {
        status_ = "Cooler on";
    }
    else
    {
        status_ = "Failed to turn on cooler";
        CCAMERAUNIT_ASI_DBG_WARN("Failed to turn on cooler");
        return;
    }
    if (controlCaps[ASI_FAN_ON] != nullptr && controlCaps[ASI_FAN_ON]->IsWritable)
    {
        if (!HasError(ASISetControlValue(cameraID, ASI_FAN_ON, 1, ASI_FALSE)))
        {
            status_ = "Fan on";
        }
        else
        {
            status_ = "Failed to turn on fan";
            CCAMERAUNIT_ASI_DBG_WARN("Failed to turn on fan");
            if (HasError(ASISetControlValue(cameraID, ASI_COOLER_ON, 0, ASI_FALSE)))
            {
                status_ = "Failed to turn off cooler";
                CCAMERAUNIT_ASI_DBG_ERR("Failed to turn off cooler after failing to turn on fan");
                throw std::runtime_error("Failed to turn off cooler after failing to turn on fan");
            }
        }
    }
    if (controlCaps[ASI_COOLER_POWER_PERC] != nullptr && controlCaps[ASI_COOLER_POWER_PERC]->IsWritable)
    {
        if (!HasError(ASISetControlValue(cameraID, ASI_COOLER_POWER_PERC, 100, ASI_TRUE)))
        {
            status_ = "Cooler power 100%";
        }
        else
        {
            status_ = "Failed to set cooler power";
            CCAMERAUNIT_ASI_DBG_WARN("Failed to set cooler power");
        }
    }
}

double CCameraUnit_ASI::GetTemperature() const
{
    if (!init_ok)
    {
        return INVALID_TEMPERATURE;
    }
    long temp;
    ASI_BOOL is_auto;
    if (HasError(ASIGetControlValue(cameraID, ASI_TEMPERATURE, &temp, &is_auto)))
    {
        return INVALID_TEMPERATURE;
    }
    return temp / 10.0;
}

double CCameraUnit_ASI::GetCoolerPower() const
{
    if (!init_ok)
    {
        return -1;
    }
    long power;
    ASI_BOOL is_auto;
    if (HasError(ASIGetControlValue(cameraID, ASI_COOLER_POWER_PERC, &power, &is_auto)))
    {
        return -1;
    }
    return power;
}

void CCameraUnit_ASI::SetBinningAndROI(int binX, int binY, int x_min, int x_max, int y_min, int y_max)
{
    if (!init_ok)
    {
        return;
    }

    if (binX != binY)
    {
        throw std::invalid_argument("BinX and BinY must be equal");
    }

    bool valid_bin = false;
    for (int i = 0; i < 16; i++)
    {
        if (binX == supportedBins[i])
        {
            valid_bin = true;
            break;
        }
    }

    if (!valid_bin)
    {
        throw std::invalid_argument("Binning value is invalid.");
    }

    binY = binX; // just to make sure

    // default values
    if (x_max <= 0)
    {
        x_max = CCDWidth_;
    }
    if (y_max <= 0)
    {
        y_max = CCDHeight_;
    }

    x_min /= binX;
    x_max /= binX;
    y_min /= binY;
    y_max /= binY;

    int img_wid = (x_max - x_min);
    int img_height = (y_max - y_min);

    int old_img_w, old_img_h, old_bin;
    ASI_IMG_TYPE old_bitdepth;

    std::lock_guard<std::mutex> lock(roiLock);

    if (!isUSB3 && std::string(cam_name).find("ASI120") != std::string::npos)
    {
        if (img_wid * img_height % 1024 != 0)
        {
            throw std::invalid_argument("ASI120 only supports image sizes that are multiples of 1024");
        }
    }
    if (HasError(ASIGetROIFormat(cameraID, &old_img_w, &old_img_h, &old_bin, &old_bitdepth)))
    {
        throw std::runtime_error("Failed to get current ROI format");
        return;
    }
    CCAMERAUNIT_ASI_DBG_INFO("Current ROI: %d x %d bin %d bitdepth %d", old_img_w, old_img_h, old_bin, old_bitdepth);
    CCAMERAUNIT_ASI_DBG_INFO("New ROI: %d x %d bin %d bitdepth %d", img_wid, img_height, binX, image_type);
    if (HasError(ASISetROIFormat(cameraID, img_wid, img_height, binX, image_type)))
    {
        CCAMERAUNIT_ASI_DBG_ERR("Failed to set ROI format");
        throw std::runtime_error("Failed to set ROI format");
    }

    if (HasError(ASISetStartPos(cameraID, x_min, y_min)))
    {
        if (HasError(ASISetROIFormat(cameraID, old_img_w, old_img_h, old_bin, old_bitdepth)))
        {
            throw std::runtime_error("Failed to reset ROI format after failed offset change");
        }
        CCAMERAUNIT_ASI_DBG_ERR("Failed to set ROI offset");
        return;
    }
    // Here, everything has changed successfully

    int imageLeft_ = x_min * binX;
    int imageRight_ = (x_min + img_wid) * binX;
    int imageTop_ = y_min * binY;
    int imageBottom_ = (y_min + img_height) * binY;
    roiLeft = imageLeft_;
    roiRight = imageRight_;
    roiBottom = imageBottom_;
    roiTop = imageTop_;
    binningX_ = binX;
    binningY_ = binY;
    CCAMERAUNIT_ASI_DBG_INFO("ROI set to (%d, %d) to (%d, %d) bin %d bitdepth %d", roiLeft, roiTop, roiRight, roiBottom, binningX_, image_type);
}

const ROI *CCameraUnit_ASI::GetROI() const
{
    std::lock_guard<std::mutex> lock(roiLock);
    roi.x_min = roiLeft;
    roi.x_max = roiRight;
    roi.y_min = roiTop;
    roi.y_max = roiBottom;
    roi.bin_x = binningX_;
    roi.bin_y = binningY_;
    return &roi;
}