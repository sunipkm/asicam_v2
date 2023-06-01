/**
 * @file ImageData.cpp
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Image Data Storage Methods Implementation
 * @version 0.1
 * @date 2022-01-03
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "ImageData.hpp"

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#if !defined(OS_Windows)
#include <unistd.h>
#include <inttypes.h>
#else
static inline void sync()
{
    _flushall();
}
#endif
#include "jpge.hpp"
#include <fitsio.h>

#include <vector>
#include <algorithm>
#include <chrono>
#include <mutex>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#ifndef CIMAGE_PREFIX
#warning "Define CIMAGE_PREFIX"
#define CIMAGE_PREFIX ccameraunit
#endif

#define CIMAGE_PREFIX_STRING TOSTRING(CIMAGE_PREFIX)

#ifndef CIMAGE_PROGNAME
#warning "Define CIMAGE_PROGNAME"
#define CIMAGE_PROGNAME cameraunit_generic
#endif
#define CIMAGE_PROGNAME_STRING TOSTRING(CIMAGE_PROGNAME)

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

#if (CIMAGEDATA_DBG_LVL >= 3)
#define CIMAGEDATA_DBG_INFO(fmt, ...)                                                                   \
    {                                                                                                        \
        fprintf(stderr, "%s:%d:%s(): " CYAN_FG fmt RESET "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr);                                                                                      \
    }
#define CIMAGEDATA_DBG_INFO_NONL(fmt, ...)                                                         \
    {                                                                                                   \
        fprintf(stderr, "%s:%d:%s(): " CYAN_FG fmt RESET, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr);                                                                                 \
    }
#define CIMAGEDATA_DBG_INFO_NONE(fmt, ...)            \
    {                                                      \
        fprintf(stderr, CYAN_FG fmt RESET, ##__VA_ARGS__); \
        fflush(stderr);                                    \
    }
#else
#define CIMAGEDATA_DBG_INFO(fmt, ...)
#define CIMAGEDATA_DBG_INFO_NONL(fmt, ...)
#define CIMAGEDATA_DBG_INFO_NONE(fmt, ...)
#endif

#if (CIMAGEDATA_DBG_LVL >= 2)
#define CIMAGEDATA_DBG_WARN(fmt, ...)                                                                     \
    {                                                                                                          \
        fprintf(stderr, "%s:%d:%s(): " YELLOW_FG fmt "\n" RESET, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr);                                                                                        \
    }
#define CIMAGEDATA_DBG_WARN_NONL(fmt, ...)                                           \
    {                                                                                     \
        fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr);                                                                   \
    }
#define CIMAGEDATA_DBG_WARN_NONE(fmt, ...) \
    {                                           \
        fprintf(stderr, fmt, ##__VA_ARGS__);    \
        fflush(stderr);                         \
    }
#else
#define CIMAGEDATA_DBG_WARN(fmt, ...)
#define CIMAGEDATA_DBG_WARN_NONL(fmt, ...)
#define CIMAGEDATA_DBG_WARN_NONE(fmt, ...)
#endif

#if (CIMAGEDATA_DBG_LVL >= 1)
#define CIMAGEDATA_DBG_ERR(fmt, ...)                                                                   \
    {                                                                                                       \
        fprintf(stderr, "%s:%d:%s(): " RED_FG fmt "\n" RESET, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr);                                                                                     \
    }
#else
#define CIMAGEDATA_DBG_ERR(fmt, ...)
#endif

static inline uint64_t getTime()
{
    return ((std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now())).time_since_epoch())).count());
}

bool string_format(std::string &out, int &size, const char *fmt, va_list ap)
{
    std::vector<char> str(size, '\0');
    auto n = vsnprintf(str.data(), str.size(), fmt, ap);
    if ((n > -1) && (size_t(n) < str.size()))
    {
        out = str.data();
        return true;
    }
    if (n > -1)
    {
        size = n + 1;
        return false;
    }
    else
    {
        size = str.size() * 2;
        return false;
    }
    return false;
}

std::string string_format(const char *fmt, ...)
{
    std::vector<char> str(256, '\0');
    va_list ap;
    while (1)
    {
        va_start(ap, fmt);
        auto n = vsnprintf(str.data(), str.size(), fmt, ap);
        va_end(ap);
        if ((n > -1) && (size_t(n) < str.size()))
        {
            return str.data();
        }
        if (n > -1)
        {
            str.resize(n + 1);
        }
        else
        {
            str.resize(str.size() * 2);
        }
    }
    return str.data();
}

void CImageMetadata::print(FILE *stream) const
{
    fprintf(stream, "Image Metadata [%" PRIu64 "]:\n", timestamp);
    fprintf(stream, "Camera name: %s\n", cameraName.c_str());
    fprintf(stream, "Image Bin: %d x %d\n", binX, binY);
    fprintf(stream, "Image origin: %d x %d\n", imgLeft, imgTop);
    fprintf(stream, "Exposure: %.6lf s\n", exposureTime);
    fprintf(stream, "Gain: %" PRId64 ", Offset: %" PRId64 "\n", gain, offset);
    fprintf(stream, "Temperature: %.2lf C\n", temperature);
    for (auto iter = extendedMetadata.begin(); iter != extendedMetadata.end(); iter++)
    {
        fprintf(stream, "%s: %s\n", iter->first.c_str(), iter->second.c_str());
    }
}

void CImageData::ClearImage()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_imageData != 0)
        delete[] m_imageData;
    m_imageData = 0;

    m_imageWidth = 0;
    m_imageHeight = 0;
    m_metadata.imgLeft = 0;
    m_metadata.imgTop = 0;

    if (m_jpegData != nullptr)
    {
        delete[] m_jpegData;
        m_jpegData = nullptr;
    }
}

CImageData::CImageData()
    : m_imageHeight(0), m_imageWidth(0), m_imageData(NULL), m_jpegData(nullptr), sz_jpegData(-1), convert_jpeg(false), JpegQuality(100), pixelMin(-1), pixelMax(-1)
{
    ClearImage();
}

CImageData::CImageData(int imageWidth, int imageHeight, unsigned short *imageData, CImageMetadata metadata, bool is8bit, bool enableJpeg, int JpegQuality, int pixelMin, int pixelMax, bool autoscale)
    : m_imageData(NULL), m_jpegData(nullptr), sz_jpegData(-1), convert_jpeg(false)
{
    ClearImage();
    std::lock_guard<std::mutex> lock(m_mutex);
    if ((imageWidth <= 0) || (imageHeight <= 0))
    {
        return;
    }

    m_imageData = new unsigned short[imageWidth * imageHeight];
    if ((m_imageData == NULL) || (m_imageData == nullptr))
    {
        return;
    }

    if (!((imageData == NULL) || (imageData == nullptr)))
    {
        if (!is8bit)
            memcpy(m_imageData, imageData, imageWidth * imageHeight * sizeof(unsigned short));
        else
        {
            for (int i = 0; i < imageWidth * imageHeight; i++)
            {
                m_imageData[i] = (unsigned short)imageData[i];
                m_imageData[i] = (m_imageData[i] << 8); // 16 bit
                if (m_imageData[i] >= 0xff00)
                    m_imageData[i] = 0xffff;
            }
        }
    }
    else
    {
        memset(m_imageData, 0, imageWidth * imageHeight * sizeof(unsigned short));
    }
    m_imageWidth = imageWidth;
    m_imageHeight = imageHeight;
    m_metadata = metadata;
    if (m_metadata.timestamp == 0)
    {
        m_metadata.timestamp = getTime();
    }
    this->JpegQuality = JpegQuality;
    this->pixelMin = pixelMin;
    this->pixelMax = pixelMax;
    this->autoscale = autoscale;

    if (enableJpeg)
    {
        convert_jpeg = true;
        ConvertJPEG();
    }
}

void CImageData::SetImageMetadata(float exposureTime, int imageLeft, int imageTop, int binX, int binY, float temperature, uint64_t timestamp, std::string cameraName)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metadata.exposureTime = exposureTime;
    m_metadata.imgLeft = imageLeft;
    m_metadata.imgTop = imageTop;
    m_metadata.binX = binX;
    m_metadata.binY = binY;
    m_metadata.temperature = temperature;
    m_metadata.cameraName = cameraName;
    m_metadata.timestamp = timestamp;
    if (m_metadata.timestamp == 0)
    {
        m_metadata.timestamp = getTime();
    }
}

void CImageData::SetImageMetadata(CImageMetadata metadata)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metadata = metadata;
}

CImageData::CImageData(const CImageData &rhs)
    : m_imageData(NULL), m_jpegData(nullptr), sz_jpegData(-1), convert_jpeg(false)
{
    ClearImage();
    std::unique_lock<std::mutex> lock1(m_mutex, std::defer_lock);
    std::unique_lock<std::mutex> lock2(rhs.m_mutex, std::defer_lock);
    std::lock(lock1, lock2);
    if ((rhs.m_imageWidth == 0) || (rhs.m_imageHeight == 0) || (rhs.m_imageData == 0))
    {
        return;
    }

    m_imageData = new unsigned short[rhs.m_imageWidth * rhs.m_imageHeight];

    if (m_imageData == 0)
    {
        return;
    }

    memcpy(m_imageData, rhs.m_imageData, rhs.m_imageWidth * rhs.m_imageHeight * sizeof(unsigned short));
    m_imageWidth = rhs.m_imageWidth;
    m_imageHeight = rhs.m_imageHeight;
    m_metadata = rhs.m_metadata;

    m_jpegData = nullptr;
    sz_jpegData = -1;
    convert_jpeg = false;
    JpegQuality = rhs.JpegQuality;
    pixelMin = rhs.pixelMin;
    pixelMax = rhs.pixelMax;
    autoscale = rhs.autoscale;

    m_metadata = rhs.m_metadata;
}

CImageData &CImageData::operator=(const CImageData &rhs)
{
    if (&rhs == this)
    { // self asignment
        return *this;
    }

    ClearImage();

    std::unique_lock<std::mutex> lock1(m_mutex, std::defer_lock);
    std::unique_lock<std::mutex> lock2(rhs.m_mutex, std::defer_lock);
    std::lock(lock1, lock2);

    if ((rhs.m_imageWidth == 0) || (rhs.m_imageHeight == 0) || (rhs.m_imageData == 0))
    {
        return *this;
    }

    m_imageData = new unsigned short[rhs.m_imageWidth * rhs.m_imageHeight];

    if (m_imageData == 0)
    {
        return *this;
    }

    memcpy(m_imageData, rhs.m_imageData, rhs.m_imageWidth * rhs.m_imageHeight * sizeof(unsigned short));
    m_imageWidth = rhs.m_imageWidth;
    m_imageHeight = rhs.m_imageHeight;
    m_metadata = rhs.m_metadata;

    m_jpegData = nullptr;
    sz_jpegData = -1;
    convert_jpeg = false;
    JpegQuality = rhs.JpegQuality;
    pixelMin = rhs.pixelMin;
    pixelMax = rhs.pixelMax;
    autoscale = rhs.autoscale;
    return *this;
}

CImageData::~CImageData()
{
    ClearImage();
}

ImageStats CImageData::GetStats() const
{
    if (!m_imageData)
    {
        return ImageStats(0, 0, 0, 0);
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    int min = 0xFFFF;
    int max = 0;
    double mean = 0.0;

    // Calculate min max and mean

    unsigned short *imageDataPtr = m_imageData;
    double rowDivisor = m_imageHeight * m_imageWidth;
    unsigned long rowSum;
    double rowAverage;
    unsigned short currentPixelValue;

    int rowIndex;
    int columnIndex;

    for (rowIndex = 0; rowIndex < m_imageHeight; rowIndex++)
    {
        rowSum = 0L;

        for (columnIndex = 0; columnIndex < m_imageWidth; columnIndex++)
        {
            currentPixelValue = *imageDataPtr;

            if (currentPixelValue < min)
            {
                min = currentPixelValue;
            }

            if (currentPixelValue > max)
            {
                max = currentPixelValue;
            }

            rowSum += currentPixelValue;

            imageDataPtr++;
        }

        rowAverage = static_cast<double>(rowSum) / rowDivisor;

        mean += rowAverage;
    }

    // Calculate standard deviation

    double varianceSum = 0.0;
    imageDataPtr = m_imageData;

    for (rowIndex = 0; rowIndex < m_imageHeight; rowIndex++)
    {
        for (columnIndex = 0; columnIndex < m_imageWidth; columnIndex++)
        {
            double tempValue = (*imageDataPtr) - mean;
            varianceSum += tempValue * tempValue;
            imageDataPtr++;
        }
    }

    double stddev = sqrt(varianceSum / static_cast<double>((m_imageWidth * m_imageHeight) - 1));

    return ImageStats(min, max, mean, stddev);
}

void CImageData::Add(const CImageData &rhs)
{
    unsigned short *sourcePixelPtr = rhs.m_imageData;
    unsigned short *targetPixelPtr = m_imageData;
    unsigned long newPixelValue;

    if (!rhs.HasData())
        return;

    // if we don't have data yet we simply copy the rhs data
    if (!this->HasData())
    {
        *this = rhs;
        return;
    }

    // we do have data, make sure our size matches the new size
    if ((rhs.m_imageWidth != m_imageWidth) || (rhs.m_imageHeight != m_imageHeight))
        return;

    for (int pixelIndex = 0;
         pixelIndex < (m_imageWidth * m_imageHeight);
         pixelIndex++)
    {
        newPixelValue = *targetPixelPtr + *sourcePixelPtr;

        if (newPixelValue > 0xFFFF)
        {
            *targetPixelPtr = 0xFFFF;
        }
        else
        {
            *targetPixelPtr = static_cast<unsigned short>(newPixelValue);
        }

        sourcePixelPtr++;
        targetPixelPtr++;
    }

    m_metadata.exposureTime += rhs.m_metadata.exposureTime;

    if (convert_jpeg)
        ConvertJPEG();
}

void CImageData::ApplyBinning(int binX, int binY)
{
    if (!HasData())
        return;
    if ((binX == 1) && (binY == 1))
    { // No binning to apply
        return;
    }

    short newImageWidth = GetImageWidth() / binX;
    short newImageHeight = GetImageHeight() / binY;

    short binSourceImageWidth = newImageWidth * binX;
    short binSourceImageHeight = newImageHeight * binY;

    unsigned short *newImageData = new unsigned short[newImageHeight * newImageWidth];

    memset(newImageData, 0, newImageHeight * newImageWidth * sizeof(unsigned short));

    // Bin the data into the new image space allocated
    for (int rowIndex = 0; rowIndex < binSourceImageHeight; rowIndex++)
    {
        const unsigned short *sourceImageDataPtr = GetImageData() + (rowIndex * GetImageWidth());

        for (int columnIndex = 0; columnIndex < binSourceImageWidth; columnIndex++)
        {
            unsigned short *targetImageDataPtr = newImageData + (((rowIndex / binY) * newImageWidth) +
                                                                 (columnIndex / binX));

            unsigned long newPixelValue = *targetImageDataPtr + *sourceImageDataPtr;

            if (newPixelValue > 0xFFFF)
            {
                *targetImageDataPtr = 0xFFFF;
            }
            else
            {
                *targetImageDataPtr = static_cast<unsigned short>(newPixelValue);
            }

            sourceImageDataPtr++;
        }
    }

    delete[] m_imageData;
    m_imageData = newImageData;
    m_imageWidth = newImageWidth;
    m_imageHeight = newImageHeight;

    if (convert_jpeg)
        ConvertJPEG();
}

void CImageData::FlipHorizontal()
{
    for (int row = 0; row < m_imageHeight; ++row)
    {
        std::reverse(m_imageData + row * m_imageWidth, m_imageData + (row + 1) * m_imageWidth);
    }

    if (convert_jpeg)
        ConvertJPEG();
}

#include <stdint.h>

uint16_t CImageData::DataMin()
{
    uint16_t res = 0xffff;
    if (!HasData())
    {
        return 0xffff;
    }
    int idx = m_imageWidth * m_imageHeight;
    while (idx--)
    {
        if (res > m_imageData[idx])
        {
            res = m_imageData[idx];
        }
    }
    return res;
}

uint16_t CImageData::DataMax()
{
    uint16_t res = 0;
    if (!HasData())
    {
        return 0xffff;
    }
    int idx = m_imageWidth * m_imageHeight;
    while (idx--)
    {
        if (res < m_imageData[idx])
        {
            res = m_imageData[idx];
        }
    }
    return res;
}

#include <stdio.h>

void CImageData::ConvertJPEG()
{
    // Check if data exists
    if (!HasData())
        return;
    // source raw image
    uint16_t *imgptr = m_imageData;
    // temporary bitmap buffer
    uint8_t *data = new uint8_t[m_imageWidth * m_imageHeight * 3]; // 3 channels for RGB
    // autoscale
    uint16_t min, max;
    if (autoscale)
    {
        min = DataMin();
        max = DataMax();
    }
    else
    {
        min = pixelMin < 0 ? 0 : (pixelMin > 0xffff ? 0xffff : pixelMin);
        max = (uint16_t)(pixelMax < 0 ? 0xffff : (pixelMax > 0xffff ? 0xffff : pixelMax));
    }
    // scaling
    float scale = 0xffff / ((float)(max - min));
    // Data conversion
    for (int i = 0; i < m_imageWidth * m_imageHeight; i++) // for each pixel in raw image
    {
        int idx = 3 * i;         // RGB pixel in JPEG source bitmap
        if (imgptr[i] == 0xffff) // saturation
        {
            data[idx + 0] = 0xff;
            data[idx + 1] = 0x0;
            data[idx + 2] = 0x0;
        }
        else if (imgptr[i] > max) // limit
        {
            data[idx + 0] = 0xff;
            data[idx + 1] = 0xa5;
            data[idx + 2] = 0x0;
        }
        else // scaling
        {
            uint8_t tmp = ((imgptr[i] - min) / 0x100) * scale;
            data[idx + 0] = tmp;
            data[idx + 1] = tmp;
            data[idx + 2] = tmp;
        }
    }
    // JPEG output buffer, has to be larger than expected JPEG size
    if (m_jpegData != nullptr)
    {
        delete[] m_jpegData;
        m_jpegData = nullptr;
    }
    m_jpegData = new uint8_t[m_imageWidth * m_imageHeight * 4 + 1024]; // extra room for JPEG conversion
    // JPEG parameters
    jpge::params params;
    params.m_quality = JpegQuality;
    params.m_subsampling = static_cast<jpge::subsampling_t>(2); // 0 == grey, 2 == RGB
    // JPEG compression and image update
    if (!jpge::compress_image_to_jpeg_file_in_memory(m_jpegData, sz_jpegData, m_imageWidth, m_imageHeight, 3, data, params))
    {
        CIMAGEDATA_DBG_ERR("Failed to compress image to jpeg in memory");
    }
    delete[] data;
}

void CImageData::GetJPEGData(unsigned char *&ptr, int &sz)
{
    if (!convert_jpeg)
    {
        convert_jpeg = true;
        ConvertJPEG();
    }
    ptr = m_jpegData;
    sz = sz_jpegData;
}

/* Sorting */
int _compare_uint16(const void *a, const void *b)
{
    return (*((unsigned short *)a) - *((unsigned short *)b));
}
/* End Sorting */

bool CImageData::FindOptimumExposure(float &targetExposure, int &bin, float percentilePixel, int pixelTarget, float maxAllowedExposure, int maxAllowedBin, int numPixelExclusion, int pixelTargetUncertainty)
{
    double exposure = m_metadata.exposureTime;
    targetExposure = exposure;
    bool changeBin = true;
    if (m_metadata.binX != m_metadata.binY)
    {
        changeBin = false;
    }
    if (maxAllowedBin < 0)
    {
        changeBin = false;
    }
    bin = m_metadata.binX;
#ifdef CIMAGE_OPTIMUM_EXP_DEBUG
    dbprintlf("Input: %lf s, bin %d x %d", exposure, m_metadata.binX, m_metadata.binY);
#endif
    double val;
    int m_imageSize = m_imageHeight * m_imageWidth;
    uint16_t *picdata = new uint16_t[m_imageSize];
    memcpy(picdata, m_imageData, m_imageSize * sizeof(uint16_t));
    qsort(picdata, m_imageSize, sizeof(unsigned short), _compare_uint16);

    bool direction;
    if (picdata[0] < picdata[m_imageSize - 1])
        direction = true;
    else
        direction = false;
    unsigned int coord;
    if (percentilePixel > 99.99)
        coord = m_imageSize - 1;
    else
        coord = floor((percentilePixel * (m_imageSize - 1) * 0.01));
    int validPixelCoord = m_imageSize - 1 - coord;
    if (validPixelCoord < numPixelExclusion)
        coord = m_imageSize - 1 - numPixelExclusion;
    if (direction)
        val = picdata[coord];
    else
    {
        if (coord == 0)
            coord = 1;
        val = picdata[m_imageSize - coord];
    }

    float targetExposure_;
    int bin_ = bin;

    /** If calculated median pixel is within pixelTarget +/- pixelTargetUncertainty, return current exposure **/
#ifdef CIMAGE_OPTIMUM_EXP_DEBUG
    dbprintlf("Uncertainty: %f, Reference: %d", fabs(pixelTarget - val), pixelTargetUncertainty);
#endif
    if (fabs(pixelTarget - val) < pixelTargetUncertainty)
    {
        goto ret;
    }

    targetExposure = ((double)pixelTarget) * exposure / ((double)val); // target optimum exposure
    targetExposure_ = targetExposure;
#ifdef CIMAGE_OPTIMUM_EXP_DEBUG
    dbprintlf("Required exposure: %f", targetExposure);
#endif
    if (changeBin)
    {
        // consider lowering binning here
        if (targetExposure_ < maxAllowedExposure)
        {
#ifdef CIMAGE_OPTIMUM_EXP_DEBUG
            dbprintlf("Considering lowering bin:");
#endif
            while (targetExposure_ < maxAllowedExposure && bin_ > 2)
            {
#ifdef CIMAGE_OPTIMUM_EXP_DEBUG
                dbprintlf("Target %f < Allowed %f, bin %d > 2", targetExposure_, maxAllowedExposure, bin_);
#endif
                targetExposure_ *= 4;
                bin_ /= 2;
            }
        }
        else
        {
            // consider bin increase here
            while (targetExposure_ > maxAllowedExposure && ((bin_ * 2) <= maxAllowedBin))
            {
                targetExposure_ /= 4;
                bin_ *= 2;
            }
        }
    }
    // update exposure and bin
    targetExposure = targetExposure_;
    bin = bin_;
ret:
    // boundary checking
    if (targetExposure > maxAllowedExposure)
        targetExposure = maxAllowedExposure;
    // round to 1 ms
    targetExposure = ((int)(targetExposure * 1000)) * 0.001;
    if (bin < 1)
        bin = 1;
    if (bin > maxAllowedBin)
        bin = maxAllowedBin;
#ifdef CIMAGE_OPTIMUM_EXP_DEBUG
    dbprintlf(YELLOW_FG "Final exposure and bin: %f s, %d", targetExposure, bin);
#endif
    delete[] picdata;
    return true;
}

bool CImageData::FindOptimumExposure(float &targetExposure, float percentilePixel, int pixelTarget, float maxAllowedExposure, int numPixelExclusion, int pixelTargetUncertainty)
{
    int bin = 1;
    return FindOptimumExposure(targetExposure, bin, percentilePixel, pixelTarget, maxAllowedExposure, -1, numPixelExclusion, pixelTargetUncertainty);
}

#if !defined(OS_Windows)
#define _snprintf snprintf
#define DIR_DELIM "/"
#else
#define DIR_DELIM "\\"
#endif

#include "utilities.h"

bool CImageData::SaveFits(bool syncOnWrite, const char *DirNamePrefix, const char *fileNameFormat, ...)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    static char defaultFilePrefix[] = CIMAGE_PREFIX_STRING;
    static char defaultDirPrefix[] = "." DIR_DELIM "fits" DIR_DELIM;
    char *DirPrefix = nullptr;
    if ((DirNamePrefix == nullptr) || (strlen(DirNamePrefix) == 0))
        DirNamePrefix = defaultDirPrefix;
    else
        DirPrefix = (char *)DirNamePrefix;
    // Default case: need to make directory if does not exist
    if (std::string(defaultDirPrefix) == std::string(DirPrefix))
    {
        // Check if directory exists
        if (file_exists(DirPrefix))
        {
            CIMAGEDATA_DBG_ERR("Directory %s is a FILE!", DirPrefix)
            return false;
        }
        if (!dir_exists(DirPrefix))
        {
            int err;
#if !defined(OS_Windows)
            err = mkdir(DirPrefix, 0764);
#else
            err = mkdir(DirPrefix);
#endif
            if (err != 0)
            {
                CIMAGEDATA_DBG_ERR("Could not create directory %s", DirPrefix)
                return false;
            }
        }
    }
    // Get the file name
    std::string fname = "";
    if (fileNameFormat == nullptr || strlen(fileNameFormat) == 0 || (std::string(fileNameFormat).find("%") == std::string::npos))
    {
        fname = string_format("%s_%llu", defaultFilePrefix, (unsigned long long)m_metadata.timestamp);
    }
    else
    {
        int sz = 256;
        bool cond = true;
        va_list ap;
        do
        {
            va_start(ap, fileNameFormat);
            if (string_format(fname, sz, fileNameFormat, ap))
            {
                cond = false;
            }
            va_end(ap);
        } while (cond);
    }
    // Get the full file name
    std::string full_name_base = string_format("%s" DIR_DELIM "%s", DirPrefix, fname.c_str());
    std::string full_name = full_name_base.c_str(); // make a copy
    int ctr = 0;
    // Check if the file exists
    do
    {
        FILE *fp = fopen(full_name.c_str(), "r");
        if (fp != NULL)
        {
            fclose(fp);
            full_name = string_format("%s_%d", full_name_base.c_str(), ++ctr);
        }
        else
        {
            break;
        }
    } while (true);

    full_name = full_name.append(".fits[compress]"); // append the format

    fitsfile *fptr;
    int status = 0, bitpix = USHORT_IMG, naxis = 2;
    long naxes[2] = {(long)(m_imageWidth), (long)(m_imageHeight)};
    unsigned int exposureTime = m_metadata.exposureTime * 1000000U;

    if (!fits_create_file(&fptr, full_name.c_str(), &status))
    {
        fits_create_img(fptr, bitpix, naxis, naxes, &status);
        fits_write_key(fptr, TSTRING, "PROGRAM", (void *)CIMAGE_PROGNAME_STRING, NULL, &status);
        fits_write_key(fptr, TSTRING, "CAMERA", (void *)(m_metadata.cameraName.c_str()), NULL, &status);
        fits_write_key(fptr, TLONGLONG, "TIMESTAMP", &(m_metadata.timestamp), NULL, &status);
        fits_write_key(fptr, TFLOAT, "CCDTEMP", &(m_metadata.temperature), NULL, &status);
        fits_write_key(fptr, TUINT, "EXPOSURE_US", &(exposureTime), NULL, &status);
        fits_write_key(fptr, TUINT, "ORIGIN_X", &(m_metadata.imgLeft), NULL, &status);
        fits_write_key(fptr, TUINT, "ORIGIN_Y", &(m_metadata.imgTop), NULL, &status);
        fits_write_key(fptr, TUSHORT, "BINX", &(m_metadata.binX), NULL, &status);
        fits_write_key(fptr, TUSHORT, "BINY", &(m_metadata.binY), NULL, &status);
        fits_write_key(fptr, TLONGLONG, "GAIN", &(m_metadata.gain), NULL, &status);
        fits_write_key(fptr, TLONGLONG, "OFFSET", &(m_metadata.offset), NULL, &status);
        fits_write_key(fptr, TINT, "GAIN_MIN", &(m_metadata.minGain), NULL, &status);
        fits_write_key(fptr, TINT, "GAIN_MAX", &(m_metadata.maxGain), NULL, &status);

        for (auto iter = m_metadata.extendedMetadata.begin(); iter != m_metadata.extendedMetadata.end(); iter++)
        {
            fits_write_key(fptr, TSTRING, iter->first.c_str(), (void *)(iter->second.c_str()), NULL, &status);
        }

        long fpixel[] = {1, 1};
        fits_write_pix(fptr, TUSHORT, fpixel, (m_imageWidth) * (m_imageHeight), m_imageData, &status);
        fits_close_file(fptr, &status);
        if (syncOnWrite)
        {
            sync();
        }
        return true;
    }
    else
    {
        CIMAGEDATA_DBG_ERR("Could not create file %s", full_name.c_str());
    }
    return false;
}