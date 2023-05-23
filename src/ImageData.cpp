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

#include <math.h>
#if !defined(OS_Windows)
#include <string.h>
#include <unistd.h>
#else
#include <stdio.h>
static inline void sync()
{
    _flushall();
}
#endif
#include "jpge.hpp"
#include "meb_print.h"
#include <fitsio.h>

#include <algorithm>
#include <chrono>

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

static inline uint64_t getTime()
{
    return ((std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now())).time_since_epoch())).count());
}

void CImageData::ClearImage()
{
    if (m_imageData != 0)
        delete[] m_imageData;
    m_imageData = 0;

    m_imageWidth = 0;
    m_imageHeight = 0;

    if (m_jpegData != nullptr)
    {
        delete[] m_jpegData;
        m_jpegData = nullptr;
    }
}

CImageData::CImageData()
    : m_imageHeight(0), m_imageWidth(0), m_exposureTime(0), m_imageData(NULL), m_jpegData(nullptr), sz_jpegData(-1), convert_jpeg(false), JpegQuality(100), pixelMin(-1), pixelMax(-1)
{
    ClearImage();
}

CImageData::CImageData(int imageWidth, int imageHeight, unsigned short *imageData, float exposureTime, int binX, int binY, float temperature, uint64_t timestamp, std::string cameraName, bool enableJpeg, int JpegQuality, int pixelMin, int pixelMax, bool autoscale)
    : m_imageData(NULL), m_jpegData(nullptr), sz_jpegData(-1), convert_jpeg(false)
{
    ClearImage();

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
        memcpy(m_imageData, imageData, imageWidth * imageHeight * sizeof(unsigned short));
    }
    else
    {
        memset(m_imageData, 0, imageWidth * imageHeight * sizeof(unsigned short));
    }
    m_imageWidth = imageWidth;
    m_imageHeight = imageHeight;
    m_exposureTime = exposureTime;
    m_binX = binX;
    m_binY = binY;
    m_temperature = temperature;
    m_cameraName = cameraName;
    m_timestamp = timestamp;
    if (m_timestamp == 0)
    {
        m_timestamp = getTime();
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

void CImageData::SetImageMetadata(float exposureTime, int binX, int binY, float temperature, uint64_t timestamp, std::string cameraName)
{
    m_exposureTime = exposureTime;
    m_binX = binX;
    m_binY = binY;
    m_temperature = temperature;
    m_cameraName = cameraName;
    m_timestamp = timestamp;
    if (m_timestamp == 0)
    {
        m_timestamp = getTime();
    }
}

CImageData::CImageData(const CImageData &rhs)
    : m_imageData(NULL), m_jpegData(nullptr), sz_jpegData(-1), convert_jpeg(false)
{
    ClearImage();

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
    m_exposureTime = rhs.m_exposureTime;
    m_binX = rhs.m_binX;
    m_binY = rhs.m_binY;
    m_temperature = rhs.m_temperature;
    m_cameraName = rhs.m_cameraName;
    m_timestamp = rhs.m_timestamp;

    m_jpegData = nullptr;
    sz_jpegData = -1;
    convert_jpeg = false;
    JpegQuality = rhs.JpegQuality;
    pixelMin = rhs.pixelMin;
    pixelMax = rhs.pixelMax;
    autoscale = rhs.autoscale;
}

CImageData &CImageData::operator=(const CImageData &rhs)
{
    if (&rhs == this)
    { // self asignment
        return *this;
    }

    ClearImage();

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
    m_exposureTime = rhs.m_exposureTime;
    m_binX = rhs.m_binX;
    m_binY = rhs.m_binY;
    m_temperature = rhs.m_temperature;
    m_cameraName = rhs.m_cameraName;
    m_timestamp = rhs.m_timestamp;

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
    if (m_imageData != NULL)
        delete[] m_imageData;
    if (m_jpegData != nullptr)
        delete[] m_jpegData;
}

ImageStats CImageData::GetStats() const
{
    if (!m_imageData)
    {
        return ImageStats(0, 0, 0, 0);
    }

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

    m_exposureTime += rhs.m_exposureTime;

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
        dbprintlf(FATAL "Failed to compress image to jpeg in memory\n");
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
    double exposure = m_exposureTime;
    targetExposure = exposure;
    bool changeBin = true;
    if (m_binX != m_binY)
    {
        changeBin = false;
    }
    if (maxAllowedBin < 0)
    {
        changeBin = false;
    }
    bin = m_binX;
#ifdef CIMAGE_OPTIMUM_EXP_DEBUG
    dbprintlf("Input: %lf s, bin %d x %d", exposure, m_binX, m_binY);
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

bool CImageData::SaveFits(char * _Nullable filePrefix, char * _Nullable DirPrefix, bool filePrefixIsName, int i, int n, char *outString, ssize_t outStringSz, bool syncOnWrite)
{
    static char defaultFilePrefix[] = CIMAGE_PREFIX_STRING;
    static char defaultDirPrefix[] = "." DIR_DELIM "fits" DIR_DELIM;
    if ((filePrefix == NULL) || (strlen(filePrefix) == 0))
        filePrefix = defaultFilePrefix;
    if ((DirPrefix == NULL) || (strlen(DirPrefix) == 0))
        DirPrefix = defaultDirPrefix;
    char fileName[256];
    char *fileName_s;
    fitsfile *fptr;
    int status = 0, bitpix = USHORT_IMG, naxis = 2;
    int bzero = 32768, bscale = 1;
    long naxes[2] = {(long)(m_imageWidth), (long)(m_imageHeight)};
    unsigned int exposureTime = m_exposureTime * 1000U;
    if (!filePrefixIsName)
    {
        if (n > 0)
        {
            if (_snprintf(fileName, sizeof(fileName), "%s" DIR_DELIM "%s_%ums_%d_%d_%llu.fit", DirPrefix, filePrefix, exposureTime, i, n, (unsigned long long)m_timestamp) > (int)sizeof(fileName))
                goto print_err;
        }
        else
        {
            if (_snprintf(fileName, sizeof(fileName), "%s" DIR_DELIM "%s_%ums_%llu.fit", DirPrefix, filePrefix, exposureTime, (unsigned long long)m_timestamp) > (int)sizeof(fileName))
                goto print_err;
        }
    }
    else
    {
        if (n > 0)
        {
            dbprintlf(FATAL "Saving snapshots is not allowed with provided file name");
        }
        else
        {
            if (_snprintf(fileName, sizeof(fileName), "%s" DIR_DELIM "%s.fit", DirPrefix, filePrefix) > (int)sizeof(fileName))
                goto print_err;
        }
    }

    unlink(fileName);
    fileName_s = new char[strlen(fileName) + 16];
    _snprintf(fileName_s, strlen(fileName) + 16, "%s[compress]", fileName);
    if (!fits_create_file(&fptr, fileName_s, &status))
    {
        fits_create_img(fptr, bitpix, naxis, naxes, &status);
        fits_write_key(fptr, TSTRING, "PROGRAM", (void *)CIMAGE_PROGNAME_STRING, NULL, &status);
        fits_write_key(fptr, TSTRING, "CAMERA", (void *)(m_cameraName.c_str()), NULL, &status);
        fits_write_key(fptr, TLONGLONG, "TIMESTAMP", &(m_timestamp), NULL, &status);
        fits_write_key(fptr, TUSHORT, "BZERO", &bzero, NULL, &status);
        fits_write_key(fptr, TUSHORT, "BSCALE", &bscale, NULL, &status);
        fits_write_key(fptr, TFLOAT, "CCDTEMP", &(m_temperature), NULL, &status);
        fits_write_key(fptr, TUINT, "EXPOSURE_MS", &(exposureTime), NULL, &status);
        fits_write_key(fptr, TUSHORT, "BINX", &(m_binX), NULL, &status);
        fits_write_key(fptr, TUSHORT, "BINY", &(m_binY), NULL, &status);

        for (auto iter = m_metadata.begin(); iter != m_metadata.end(); iter++)
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
        if (outString != NULL && outStringSz > 0)
        {
            _snprintf(outString, outStringSz, "wrote %d of %d", i, n);
        }
        delete[] fileName_s;
        return true;
    }
    else
    {
        dbprintlf(FATAL "Could not create file %s", fileName_s);
    }
    delete[] fileName_s;
    return false;
print_err:
    if (outString != NULL && outStringSz > 0)
    {
        _snprintf(outString, outStringSz, "failed %d of %d", i, n);
    }
    return false;
}