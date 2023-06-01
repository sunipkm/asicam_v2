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

#include <string>
#include "ImageData.hpp"

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

#ifndef CCAMERAUNIT_DBG_LVL
/**
 * @brief Debug level for the camera unit. 0 = no debug, 1 = errors only, 2 = errors and warnings, 3 = errors, warnings and info
 *
 */
#define CCAMERAUNIT_DBG_LVL 3
#endif // !CCAMERAUNIT_DBG_LVL

/**
 * @brief ROI struct specifying a region of interest
 *
 */
class ROI
{
public:
    int x_min; /*!< Minimum X (in unbinned coordinates) */
    int x_max; /*!< Maximum X (in unbinned coordinates) */
    int y_min; /*!< Minimum Y (in unbinned coordinates) */
    int y_max; /*!< Maximum Y (in unbinned coordinates) */
    int bin_x; /*!< Binning in X */
    int bin_y; /*!< Binning in Y */

    /**
     * @brief Print the ROI to a stream.
     * 
     * @param stream Stream to print to. Defaults to stdout.
     */
    void PrintROI(FILE *stream = stdout)
    {
        fprintf(stream, "ROI: x_min = %d, x_max = %d, y_min = %d, y_max = %d, bin_x = %d, bin_y = %d\n", x_min, x_max, y_min, y_max, bin_x, bin_y);
    }
};

/**
 * @brief Callback function type for non-blocking image capture.
 *
 * @param image Pointer to the captured image ({@link CImageData}).
 * @param roi ROI of the captured image.
 * @param data User data passed to the callback function.
 */
typedef void (*CCameraUnitCallback)(const CImageData *image, const ROI roi, void *data);

const double INVALID_TEMPERATURE = -273.0;
class CCameraUnit
{
public:
    virtual ~CCameraUnit() = default;

    /**
     * @brief Get the camera vendor (used to identify backend library)
     *
     * @return const char* Camera vendor. Must be a string literal,
     * preferably defined in the implementation file for the relevant backend.
     */
    virtual const char *GetVendor() const = 0;

    /**
     * @brief Get the internal camera handle. This is backend specific.
     * e.g. for ZWO ASI cameras, this is the camera ID.(int).
     *
     * @return const void* Internal camera handle (backend specific).
     */
    virtual const void *GetHandle() const = 0;

    /**
     * @brief Get the camera UUID
     * 
     * @return const std::string 
     */
    virtual const std::string GetUUID() const = 0;
    
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
     * @brief Get the raw gain value (backend specific)
     *
     * @return long Raw gain value
     */
    virtual long GetGainRaw() const = 0;

    /**
     * @brief Set the raw gain value (backend specific)
     *
     * @param gain Raw gain value
     * @return long Gain that was set
     */
    virtual long SetGainRaw(long gain) = 0;

    /**
     * @brief Set the gain of the camera
     *
     * @return float Gain that was set
     */
    virtual float SetGain(float gain) = 0;

    /**
     * @brief Get the pixel voltage offset
     *
     * @return int Offset
     */
    virtual int GetOffset() const = 0;

    /**
     * @brief Set the pixel voltage offset
     *
     * @param offset Offset to be set
     * @return int
     */
    virtual int SetOffset(int offset) = 0;

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
     * @return bool Status of the shutter. True = open, False = closed
     */
    virtual bool SetShutterOpen(bool open) = 0;

    /**
     * @brief Check if the shutter is open.
     *
     * @return bool Status of the shutter. True = open, False = closed
     */
    virtual bool GetShutterOpen() const = 0;

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
     * @brief Get the current cooler power
     * 
     * @return double Cooler power %
     */
    virtual double GetCoolerPower() const = 0;

    /**
     * @brief Set the current cooler power
     * 
     * @param power Cooler power %
     * @return double 
     */
    virtual double SetCoolerPower(double power) = 0;

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
