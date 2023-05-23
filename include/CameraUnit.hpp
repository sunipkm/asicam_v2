/**
 * @file CameraUnit.hpp
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Virtual CameraUnit interface for different camera backends
 * @version 1.0a
 * @date 2023-05-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __CAMERAUNIT_HPP__
#define __CAMERAUNIT_HPP__

#include "ImageData.hpp"
#include <string>

#ifndef _Catchable
/**
 * @brief Functions marked with _Catchable are expected to throw exceptions.
 * 
 */
#define _Catchable
#endif

#ifndef _NotImplemented
/**
 * @brief Functions marked with _NotImplemented are not implemented.
 * 
 */
#define _NotImplemented
#endif

/**
 * @brief ROI struct specifying a region of interest
 * 
 */
typedef struct
{
    int x_min; // Minimum X (in unbinned coordinates)
    int x_max; // Maximum X (in unbinned coordinates)
    int y_min; // Minimum Y (in unbinned coordinates)
    int y_max; // Maximum Y (in unbinned coordinates)
    int bin_x; // Binning in X
    int bin_y; // Binning in Y
} ROI;

typedef void (*CCameraUnitCallback)(const CImageData *image, const ROI roi, void *data);

const double INVALID_TEMPERATURE = -273.0;
class CCameraUnit
{
public:
    virtual ~CCameraUnit(){};

    /**
     * @brief Get the camera vendor (used to identify backend library)
     * 
     * @return const char* Camera vendor.
     */
    virtual const char *GetVendor() const = 0;

    /**
     * @brief Get the internal camera handle
     * 
     * @return const void* 
     */
    virtual const void *GetHandle() const = 0;

    /**
     * @brief Capture image from the connected camera
     * 
     * @param blocking If true, block until capture is complete. Blocks by default.
     * @param callback_fn Callback function to call when capture is complete. If blocking is true, this is ignored.
     * @param user_data User data to pass to the callback function. If blocking is true, this is ignored.
     * @return CImageData Container with raw image data
     */
    virtual CImageData CaptureImage(bool blocking = true, CCameraUnitCallback callback_fn = nullptr, void *user_data = nullptr) = 0;

    /**
     * @brief Cancel an ongoing capture
     * 
     */
    virtual void CancelCapture() = 0;

    /**
     * @brief Check if a capture is ongoing
     * 
     * @return true 
     * @return false 
     */
    virtual bool IsCapturing() const = 0;

    /**
     * @brief Get the last captured image
     * 
     * @return const CImageData* Pointer to the last captured image
     */
    virtual const CImageData *GetLastImage() const = 0;

    /**
     * @brief Check if camera was initialized properly
     * 
     * @return true 
     * @return false 
     */
    virtual bool CameraReady() const = 0;

    /**
     * @brief Get the name of the connected camera
     * 
     * @return const char* 
     */
    virtual const char *CameraName() const = 0;

    /**
     * @brief Set the exposure time in seconds
     * 
     * @param exposureInSeconds 
     */
    virtual void SetExposure(double exposureInSeconds) = 0;

    /**
     * @brief Get the currently set exposure
     * 
     * @return double Exposure time in seconds
     */
    virtual double GetExposure() const = 0;

    /**
     * @brief Get the current gain
     * 
     * @return float 
     */
    virtual float GetGain() const = 0;

    /**
     * @brief Set the gain of the camera
     * 
     * @return float Gain that was set
     */
    virtual float SetGain(float gain) = 0;

    /**
     * @brief Get the minimum exposure time in seconds
     * 
     * @return float 
     */
    virtual const double GetMinExposure() const = 0;

    /**
     * @brief Get the maximum exposure time in seconds
     * 
     * @return float 
     */
    virtual const double GetMaxExposure() const = 0;

    /**
     * @brief Get the minimum usable gain
     * 
     * @return float 
     */
    virtual const float GetMinGain() const = 0;

    /**
     * @brief Get the maximum usable gain
     * 
     * @return float 
     */
    virtual const float GetMaxGain() const = 0;

    /**
     * @brief Open or close the shutter
     * 
     * @param open 
     */
    virtual void SetShutterOpen(bool open) = 0;

    /**
     * @brief Set the cooler target temperature
     * 
     * @param temperatureInCelcius 
     */
    virtual void SetTemperature(double temperatureInCelcius) = 0;

    /**
     * @brief Get the current detector temperature
     * 
     * @return double 
     */
    virtual double GetTemperature() const = 0;

    /**
     * @brief Set the Binning And ROI information
     * 
     * @param x X axis binning
     * @param y Y axis binning
     * @param x_min Leftmost pixel index (unbinned)
     * @param x_max Rightmost pixel index (unbinned)
     * @param y_min Topmost pixel index (unbinned)
     * @param y_max Bottommost pixel index (unbinned)
     */
    virtual void _Catchable SetBinningAndROI(int x, int y, int x_min = 0, int x_max = 0, int y_min = 0, int y_max = 0) = 0;

    /**
     * @brief Get the X binning set on the detector
     * 
     * @return int 
     */
    virtual int GetBinningX() const = 0;

    /**
     * @brief Get the Y binning set on the detector
     * 
     * @return int 
     */
    virtual int GetBinningY() const = 0;

    /**
     * @brief Get the currently set region of interest
     * 
     * @return const ROI* 
     */
    virtual const ROI *GetROI() const = 0;

    /**
     * @brief Get the current status string
     * 
     * @return std::string 
     */
    virtual std::string GetStatus() const = 0; // should return empty string when idle

    /**
     * @brief Get the detector width in pixels
     * 
     * @return int 
     */
    virtual int GetCCDWidth() const = 0;

    /**
     * @brief Get the detector height in pixels
     * 
     * @return int 
     */
    virtual int GetCCDHeight() const = 0;

    /**
     * @brief Get the pixel size in microns
     * 
     * @return double Pixel size in microns
     */
    virtual double GetPixelSize() const = 0;
};

#endif // __CAMERAUNIT_HPP__
