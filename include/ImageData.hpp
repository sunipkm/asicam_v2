/**
 * @file ImageData.hpp
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Common Image Data Storage Format
 * @version 0.1
 * @date 2022-01-03
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef __IMAGEDATA_HPP__
#define __IMAGEDATA_HPP__
#include <stdlib.h>
#include <stdint.h>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <windows.h>
#define OS_Windows
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif
#include <string>
#include <map>
#include <mutex>

#ifndef _Nullable
/**
 * @brief Indicate the pointer can be set to null safely.
 *
 */
#define _Nullable
#endif

#ifndef CIMAGEDATA_DBG_LVL
/**
 * @brief Debug level for CImageData. 0 = no debug, 1 = errors only, 2 = errors and warnings, 3 = errors, warnings and info
 *
 */
#define CIMAGEDATA_DBG_LVL 3
#endif

#ifndef _DOXYGEN_
#undef FORMAT_STRING
#if _MSC_VER >= 1400
#include <sal.h>
#if _MSC_VER > 1400
#define FORMAT_STRING(p) _Printf_format_string_ p
#else
#define FORMAT_STRING(p) __format_string p
#endif /* FORMAT_STRING */
#else
#define FORMAT_STRING(p) p
#endif /* _MSC_VER */
#endif /* _DOXYGEN_ */

/**
 * @brief Image Data Statistics Storage Class
 *
 */
class ImageStats
{
    int min_;
    int max_;
    double mean_;
    double stddev_;

public:
    /**
     * @brief Construct a new Image Stats object
     *
     * @param min Minimum pixel count
     * @param max Maximum pixel count
     * @param mean Mean pixel count
     * @param stddev Standard deviation of pixel count
     */
    ImageStats(int min, int max, double mean, double stddev)
        : min_(min), max_(max), mean_(mean), stddev_(stddev) {}
    /**
     * @brief Get the minimum pixel count
     *
     * @return int
     */
    int GetMinValue() const { return min_; }
    /**
     * @brief Get the maximum pixel count
     *
     * @return int
     */
    int GetMaxValue() const { return max_; }
    /**
     * @brief Get the mean pixel count
     *
     * @return int
     */
    double GetMeanValue() const { return mean_; }
    /**
     * @brief Get the pixel count standard deviation
     *
     * @return int
     */
    double GetStandardDeviationValue() const { return stddev_; }
};

/**
 * @brief Image metadata storage class.
 *
 */
class CImageMetadata
{
public:
    double exposureTime;                                 /*!< Exposure time in seconds */
    int binX;                                            /*!< X axis bin */
    int binY;                                            /*!< Y axis bin */
    int imgTop;                                          /*!< Top offset of image (binned coordinates) */
    int imgLeft;                                         /*!< Left offset of image (binned coordinates) */
    double temperature;                                  /*!< CCD temperature in degree C */
    uint64_t timestamp;                                  /*!< Timestamp since epoch in ms */
    std::string cameraName;                              /*!< Camera name */
    int64_t gain;                                        /*!< Gain */
    int64_t offset;                                      /*!< Offset */
    int minGain;                                         /*!< Minimum gain */
    int maxGain;                                         /*!< Maximum gain */
    std::map<std::string, std::string> extendedMetadata; /*!< Extended metadata */

    /**
     * @brief Print metadata to a stream.
     *
     * @param stream Stream to print to. Defaults to stdout.
     */
    void print(FILE *stream = stdout) const;

    /**
     * @brief Add an extended attribute to metadata.
     *
     * @param key Metadata key
     * @param value Metadata value
     */
    inline void AddExtendedAttribute(std::string key, std::string value)
    {
        extendedMetadata[key] = value;
    }
};

/**
 * @brief Class to contain 16-bit raw image data
 *
 */
class CImageData
{
    int m_imageHeight;
    int m_imageWidth;

    CImageMetadata m_metadata;

    unsigned short *m_imageData;

    unsigned char *m_jpegData;
    int sz_jpegData;

    bool convert_jpeg;

    int JpegQuality;
    int pixelMin;
    int pixelMax;
    bool autoscale;

    mutable std::mutex m_mutex;

public:
    /**
     * @brief Construct a new CImageData object
     *
     */
    CImageData();
    /**
     * @brief Construct a new CImageData object from image data
     *
     * @param imageWidth Width of image
     * @param imageHeight Height of image
     * @param imageData [optional] Pointer to image data
     * @param metadata [optional] Image metadata (exposure, timestamp, binning, gain etc.)
     * @param is8bit [optional] Image data is 8-bit (default is 16-bit)
     * @param enableJpeg [optional] Enable JPEG conversion
     * @param JpegQuality [optional] Quality of JPEG conversion
     * @param pixelMin [optional] JPEG image scaling pixel count minimum, -1 for default (0x0000), overriden by autoscale flag
     * @param pixelMax [optional] JPEG image scaling pixel count maximum, -1 for default (0xffff), overriden by autoscale flag
     * @param autoscale [optional] Auto-scale JPEG image brightness based on data
     */
    CImageData(int imageWidth, int imageHeight, unsigned short *imageData = NULL, CImageMetadata metadata = CImageMetadata(), bool is8bit = false, bool enableJpeg = false, int JpegQuality = 100, int pixelMin = -1, int pixelMax = -1, bool autoscale = true);

    /**
     * @brief Construct a new CImageData object from another CImageData object
     *
     * @param rhs CImageData object
     */
    CImageData(const CImageData &rhs);
    /**
     * @brief Check if two CImageData objects are equal
     *
     * @param rhs
     * @return CImageData&
     */
    CImageData &operator=(const CImageData &rhs);

    ~CImageData();

    /**
     * @brief Clear existing data
     *
     */
    void ClearImage();

    /**
     * @brief Get the metadata associated with the current image.
     *
     * @return CImageMetadata Image metadata.
     */
    inline CImageMetadata GetImageMetadata() const { return m_metadata; }

    /**
     * @brief Returns if the container contains image data
     *
     * @return true
     * @return false
     */
    bool HasData() const { return m_imageData != NULL; }
    /**
     * @brief Set metadata for the image
     *
     * @param exposureTime Exposure for the image
     * @param imageLeft [optional] Left offset of image
     * @param imageTop [optional] Top offset of image
     * @param binX [optional] X axia bin
     * @param binY [optional] Y axis bin
     * @param temperature [optional] CCD Temperature
     * @param timestamp [optional] Image timestamp
     * @param cameraName [optional] Camera name
     */
    void SetImageMetadata(float exposureTime, int imageLeft = 0, int imageTop = 0, int binX = 1, int binY = 1, float temperature = 0, uint64_t timestamp = 0, std::string cameraName = "");
    /**
     * @brief Set metadata for the image
     *
     * @param metadata Image metadata
     */
    void SetImageMetadata(CImageMetadata metadata);
    /**
     * @brief Set the extended metadata info
     *
     * @param key String key
     * @param value String value
     */
    void SetExtendedMetadata(std::string const key, std::string const value)
    {
        m_metadata.AddExtendedAttribute(key, value);
    }
    /**
     * @brief Retrieve JPEG image corresponding to raw data
     *
     * @param ptr Pointer to JPEG image data
     * @param sz Size of JPEG image data
     */
    void GetJPEGData(unsigned char *&ptr, int &sz);
    /**
     * @brief Set quality of JPEG image
     *
     * @param quality Quality in % (10 - 100)
     */
    void SetJPEGQuality(int quality = 100)
    {
        JpegQuality = quality;
        JpegQuality = JpegQuality < 0 ? 10 : JpegQuality;
        JpegQuality = JpegQuality > 100 ? 100 : JpegQuality;
    }
    /**
     * @brief Set pixel scaling values for JPEG image conversion
     *
     * @param min Minimum pixel count, this is the minimum brightness [dark level]
     * @param max Maximum pixel count, this is the maximum brightness [bright level]
     */
    void SetJPEGScaling(int min = -1, int max = -1)
    {
        pixelMin = min;
        pixelMax = max;
    }
    /**
     * @brief Enable/disable automatic scaling of image brightness based on pixel data
     *
     * @param autoscale
     */
    void SetJPEGScaling(bool autoscale);
    /**
     * @brief Get statistics on image data
     *
     * @return ImageStats Statistics data container
     */
    ImageStats GetStats() const;
    /**
     * @brief Get the pointer to image data
     *
     * @return const unsigned short* const
     */
    const unsigned short *const GetImageData() const { return m_imageData; }
    /**
     * @brief Get the pointer to image data
     *
     * @return unsigned short* const
     */
    unsigned short *const GetImageData() { return m_imageData; }
    /**
     * @brief Stack data from another image
     *
     * @param rsh Image container
     */
    void Add(const CImageData &rsh);
    /**
     * @brief Software-binning of image data
     *
     * @param x X axis binning
     * @param y Y axis binning
     */
    void ApplyBinning(int x, int y);
    /**
     * @brief Flip image horizontally
     *
     */
    void FlipHorizontal();
    /**
     * @brief Find optimum exposure from this exposure
     *
     * @param targetExposure Target exposure time (output)
     * @param bin Target bin size (output)
     * @param percentilePixel Pixel percentile target (input, default: 80 percentile)
     * @param pixelTarget Value terget for pixel percentile (input, default: 40000)
     * @param maxAllowedExposure Maximum allowed exposure time (input, default: 10 s)
     * @param maxAllowedBin Maximum allowed binning (input, default: 4)
     * @param numPixelExclusion Number of pixels to be excluded from calculation (input, default: 100)
     * @param pixelTargetUncertainty Value target uncertainty (inpit, default: 5000)
     * @return bool Returns true.
     */
    bool FindOptimumExposure(float &targetExposure, int &bin, float percentilePixel = 80, int pixelTarget = 40000, float maxAllowedExposure = 10.0, int maxAllowedBin = 4, int numPixelExclusion = 100, int pixelTargetUncertainty = 5000);
    /**
     * @brief Find optimum exposure from this exposure without binning adjustment
     *
     * @param targetExposure Target exposure time (output)
     * @param percentilePixel Pixel percentile target (input, default: 80 percentile)
     * @param pixelTarget Value terget for pixel percentile (input, default: 40000)
     * @param maxAllowedExposure aximum allowed exposure time (input, default: 10 s)
     * @param numPixelExclusion Number of pixels to be excluded from calculation (input, default: 100)
     * @param pixelTargetUncertainty Value target uncertainty (inpit, default: 5000)
     * @return bool Returns true.
     */
    bool FindOptimumExposure(float &targetExposure, float percentilePixel = 80, int pixelTarget = 40000, float maxAllowedExposure = 10.0, int numPixelExclusion = 100, int pixelTargetUncertainty = 5000);
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((__format__(__printf__, 4, 5)))
#endif
    /**
     * @brief Save image contained in CImageData
     *
     * @param syncOnWrite Sync on write (write to disk immediately).
     * @param DirNamePrefix Directory name prefix (if NULL, uses ./fits)
     * @param fileNameFormat File name format. If NULL, uses <{@link CIMAGE_PREFIX_STRING}>_<timestamp>.fit. If not null and does not contain any '%' character, uses <fileNameFormat>_<timestamp>.fit. If the file exists, numbers are appended to the file name. Presence of a '%' character indicates that the file name format is a printf-style format string.
     *
     * @return bool Returns true if successful, false otherwise.
     */
    bool SaveFITS(bool syncOnWrite, const char *_Nullable DirNamePrefix, FORMAT_STRING(const char *fileNameFormat), ...);
    /**
     * @brief Get the image height
     *
     * @return int Image height in pixels
     */
    inline int GetImageHeight() const { return m_imageHeight; }
    /**
     * @brief Get the image width
     *
     * @return int Image width in pixels
     */
    inline int GetImageWidth() const { return m_imageWidth; }
    /**
     * @brief Get the image exposure
     *
     * @return float Exposure in seconds
     */
    inline float GetExposure() const { return m_metadata.exposureTime; }
    /**
     * @brief Get the X axis (width) binning
     *
     * @return int
     */
    inline int GetBinX() const { return m_metadata.binX; }
    /**
     * @brief Get the Y axis (height) binning
     *
     * @return int
     */
    inline int GetBinY() const { return m_metadata.binY; }
    /**
     * @brief Get the CCD Temperature
     *
     * @return float Temperature in degree C
     */
    inline float GetTemperature() const { return m_metadata.temperature; }
    /**
     * @brief Get the timestamp of image
     *
     * @return uint64_t Timestamp since epoch in ms
     */
    inline uint64_t GetTimestamp() const { return m_metadata.timestamp; }
    /**
     * @brief Get the camera name string
     *
     * @return std::string
     */
    inline std::string GetCameraName() const { return m_metadata.cameraName; }

private:
    /**
     * @brief Convert raw image to JPEG image with preset settings
     *
     */
    void ConvertJPEG();
    /**
     * @brief Return minimum pixel count
     *
     * @return uint16_t
     */
    uint16_t DataMin();
    /**
     * @brief Return maximum pixel count
     *
     * @return uint16_t
     */
    uint16_t DataMax();
};

#endif // __IMAGEDATA_HPP__
