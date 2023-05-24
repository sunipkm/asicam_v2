#include "CameraUnit_ASI.hpp"
#include "meb_print.h"
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <limits.h>

#include <dirent.h>
#include <errno.h>

#include <thread>

#define _Catchable

static int GetBootCount();
void _Catchable checknmakedir(const char *path);

#include <time.h>
static inline uint64_t get_msec()
{
    struct timespec tm;
    clock_gettime(CLOCK_REALTIME, &tm);
    uint64_t msec = tm.tv_sec * 1000LLU;
    msec += (tm.tv_nsec / 1000000LLU);
    return msec;
}

volatile sig_atomic_t done = 0;
void sighandler(int sig)
{
    done = 1;
}

static int bootCount = 0;
static char dirname[256] = {
    0,
};

// void frame_grabber(CCameraUnit *cam, uint64_t cadence = 10) // cadence in seconds
// {
//     static float LandsatExposureCutoff = 10; // collect landsat exposures IF aeronomy exposures are shorter than THIS
//     static float maxExposure = 120;
//     static float pixelPercentile = 99.7;
//     static int pixelTarget = 40000;
//     static int pixelUncertainty = 5000;
//     static int maxBin = 2;
//     static int imgXMin = 100, imgYMin = 335, imgXMax = -1, imgYMax = -1;

//     static float exposure_1 = 0.2; // 200 ms
//     static int bin_1 = 1;          // start with bin 1

//     long retrycount = 10;

//     if (cam == nullptr)
//     {
//         dbprintlf(FATAL "Camera is NULL");
//         return;
//     }

//     while (!done)
//     {
//         uint64_t start = get_msec();
//         // aeronomy section
//         {
//             // set binning, ROI, exposure
//             cam->SetBinningAndROI(bin_1, bin_1, imgXMin, imgXMax, imgYMin, imgYMax);
//             cam->SetExposure(exposure_1);
//             CImageData img = cam->CaptureImage(retrycount); // capture frame
//             if (!img.SaveFits((char *) "aero", dirname))               // save frame
//             {
//                 bprintlf(FATAL "[%" PRIu64 "] AERO: Could not save FITS", start);
//             }
//             else
//             {
//                 bprintlf(GREEN_FG "[%" PRIu64 "] AERO: Saved Exposure %.3f s, Bin %d", start, exposure_1, bin_1);
//             }
//             sync();
//             // run auto exposure
//             img.FindOptimumExposure(exposure_1, bin_1, pixelPercentile, pixelTarget, maxExposure, maxBin, 100, pixelUncertainty);
//         }
//         // landsat section
//         if (exposure_1 < LandsatExposureCutoff) // exposure < cutoff, probably daytime + close to ground
//         {
//             // set binning, ROI, exposure
//             cam->SetBinningAndROI(1, 1, imgXMin, imgXMax, 0, imgYMin);
//             float lexposures[] = {0.001, 0.002, 0.005};
//             for (int lidx = 0; lidx < sizeof(lexposures) / sizeof(lexposures[0]); lidx++)
//             {
//                 cam->SetExposure(lexposures[lidx]);
//                 for (int midx = 0; midx < 4; midx++)
//                 {
//                     CImageData img = cam->CaptureImage(retrycount); // capture frame
//                     if (!img.SaveFits((char *) "lsat", dirname))               // save frame
//                     {
//                         bprintlf(FATAL "[%" PRIu64 "] LSAT: Could not save FITS", start);
//                     }
//                     else
//                     {
//                         bprintlf(GREEN_FG "[%" PRIu64 "] LSAT: Saved Exposure %.3f s", start, lexposures[lidx]);
//                     }
//                 }
//             }
//             sync();
//         }
//         start = get_msec() - start;
//         if (start < cadence * 1000)
//         {
//             uint64_t res = (cadence * 1000 - start) * 1000;
//             usleep(res % 1000000);
//             res -= res % 1000000;
//             while ((res > 0) && (!done))
//             {
//                 usleep(1000000);
//                 res -= 1000000;
//             }
//         }
//     }
// }

void CaptureCallback(const CImageData *img, const ROI roi, void *data)
{
    CImageData imgdata = *img;
    PrintROI(roi);
    unsigned char *jpegdata = nullptr;
    int size = 0;
    imgdata.GetJPEGData(jpegdata, size);
    bprintf("JPEG size: %d\t", size);
    for (int i = 0; i < 20; i++)
    {
        printf("%02x ", jpegdata[i]);
    }
    bprintlf();

    FILE *fp = fopen("test.jpg", "wb");
    fwrite(data, size, 1, fp);
    fclose(fp);
    delete[] data;
}

int main(int argc, char *argv[])
{
    uint64_t cadence = 30;

    sleep(1);

    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    // CCameraUnit *cam = nullptr;
    long retryCount = 10;
    int num_cameras = 0;
    int *camera_ids = nullptr;
    std::string *camera_names = nullptr;
    CCameraUnit_ASI::ListCameras(num_cameras, camera_ids, camera_names);
    for (int i = 0; i < num_cameras; i++)
    {
        bprintlf(GREEN_FG "Camera %d> %d: %s", i, camera_ids[i], camera_names[i].c_str());
    }
    if (num_cameras == 0)
    {
        bprintlf(RED_FG "No cameras found");
    }

    CCameraUnit *cameras[num_cameras];
    for (int i = 0; i < num_cameras; i++)
    {
        cameras[i] = new CCameraUnit_ASI(camera_ids[i]);
        if (cameras[i]->CameraReady())
        {
            bprintlf(GREEN_FG "Camera %d ready", i);
        }
        else
        {
            bprintlf(RED_FG "Camera %d not ready", i);
        }
    }
    cameras[0]->SetExposure(0.5);
    cameras[0]->SetBinningAndROI(1, 1, 200, 2800, 200, 2800);
    cameras[0]->SetGain(50);
    CImageData img = cameras[0]->CaptureImage(false, CaptureCallback);
    // unsigned char *data = nullptr;
    // int size = 0;
    // img.GetJPEGData(data, size);
    // bprintf("JPEG size: %d\t", size);
    // for (int i = 0; i < 20; i++)
    // {
    //     printf("%02x ", data[i]);
    // }
    // bprintlf();

    // FILE *fp = fopen("test.jpg", "wb");
    // fwrite(data, size, 1, fp);
    // fclose(fp);
    // delete[] data;

    CCameraUnit_ASI *cam = (CCameraUnit_ASI *) cameras[0];

    cameras[0]->SetTemperature(-10);
    int ctr = 0;
    while (!done)
    {
        sleep(1);
        double temp = cameras[0]->GetTemperature();
        bprintlf("%04d s> Temperature: %.2f", ++ctr, temp);
    }

    if (num_cameras)
    {
        delete[] camera_ids;
        delete[] camera_names;
    }
    exit(0);
    //     do
    //     {
    //         cam = new CCameraUnit_ASI();
    //         if (cam->CameraReady())
    //         {
    //             bprintlf(GREEN_FG "Camera ready");
    //             break;
    //         }
    //         else
    //         {
    //             delete cam;
    //             cam = nullptr;
    //         }
    //     } while (retryCount--);

    //     if (cam == nullptr)
    //     {
    //         bprintlf(RED_FG "Error opening camera");
    //         exit(0);
    //     }

    //     bootCount = GetBootCount();

    //     snprintf(dirname, sizeof(dirname), "data/%d", bootCount);

    //     try
    //     {
    //         checknmakedir(dirname);
    //     }
    //     catch (const std::exception &e)
    //     {
    //         dbprintlf(FATAL "Error creating directory: %s", e.what());
    //         delete cam;
    //         gpioWrite(11, GPIO_LOW);
    //         exit(0);
    //     }

    //     std::thread camera_thread(frame_grabber, cam, cadence);

    //     mcp9808 board_tsensor;
    //     bool board_ts_active = board_tsensor.begin(0x18);
    //     if (!board_ts_active)
    //     {
    //         dbprintlf(RED_FG "Board temperature sensor not found.");
    //     }
    //     std::string tfile_name = dirname;
    //     tfile_name += "/templog.txt";
    //     FILE *fp = fopen(tfile_name.c_str(), "a");
    //     if (fp == NULL)
    //     {
    //         dbprintlf(FATAL "Could not open temperature log file");
    //     }
    //     else
    //     {
    //         fprintf(fp, "# Time (ms), CCD Temperature (C x 100), Board Temperature (C x 100)\n");
    //     }
    //     while (!done)
    //     {
    //         uint64_t tnow = get_msec();
    //         short board_temp = -9999;
    //         if (board_ts_active)
    //             board_temp = board_tsensor.readTemp();
    //         short ccd_temp = cam->GetTemperature() * 100;
    //         std::string tlog_str = std::to_string(tnow) + "," + std::to_string(ccd_temp) + "," + std::to_string(board_temp) + "\n";
    //         bprintf("%s", tlog_str.c_str());
    //         if (fp != NULL)
    //         {
    //             fprintf(fp, "%s", tlog_str.c_str());
    //             fflush(fp);
    //         }
    //         usleep(1000000); // every second
    //     }
    //     if (fp != NULL)
    //     {
    //         fclose(fp);
    //     }
    //     camera_thread.join();
    //     if (board_ts_active)
    //         board_tsensor.shutdown();
    //     sync();

    //     delete cam;
    //     gpioWrite(11, GPIO_LOW);
    //     bprintlf("Exiting.");
    //     return 0;
    // }

    // static int create_or_open(const char *path, int create_flags, int open_flags,
    //                           int *how)
    // {
    //     int fd;
    //     create_flags |= (O_CREAT | O_EXCL);
    //     open_flags &= ~(O_CREAT | O_EXCL);
    //     for (;;)
    //     {
    //         *how = 1;
    //         fd = open(path, create_flags, 0666);
    //         if (fd >= 0)
    //             break;
    //         if (errno != EEXIST)
    //             break;
    //         *how = 0;
    //         fd = open(path, open_flags);
    //         if (fd >= 0)
    //             break;
    //         if (errno != ENOENT)
    //             break;
    //     }
    //     return fd;
    // }

    // static int GetBootCount()
    // {
    //     // 1. Check if file exists, if not ask for step counter and clear all calibration
    //     char cwd[PATH_MAX];
    //     off_t sz;
    //     memset(cwd, 0x0, sizeof(cwd));
    //     if (getcwd(cwd, sizeof(cwd)) == NULL)
    //     {
    //         dbprintlf("Error getting current directory.");
    //     }
    //     int newfile = 0;
    //     int fd = create_or_open(bootcount_fname, O_WRONLY | O_SYNC, O_RDWR | O_SYNC, &newfile);
    //     int current_pos = 0;
    //     if (fd < 0)
    //     {
    //         dbprintlf(FATAL "Error creating/opening file. Error: %s, Errno: %d.", strerror(errno), errno);
    //     }
    //     if (!newfile)
    //     {
    //         // 2. File exists
    //         sz = lseek(fd, 0, SEEK_END);
    //         if (sz < (int)(sizeof(int)))
    //         {
    //             dbprintlf("Size of position file %u, invalid.", (unsigned int)sz);
    //             goto rewrite;
    //         }
    //         lseek(fd, 0, SEEK_SET);
    //         if (read(fd, &current_pos, sizeof(current_pos)) < 0)
    //         {
    //             dbprintlf(RED_FG "Unexpected EOF on %s/%s.", cwd, bootcount_fname);
    //             current_pos = 0;
    //         }
    //     }
    // rewrite:
    //     lseek(fd, 0, SEEK_SET);
    //     int _current_pos = current_pos + 1;
    //     if (write(fd, &_current_pos, sizeof(_current_pos)) != sizeof(_current_pos))
    //     {
    //         dbprintlf(RED_FG "Could not update current position.");
    //     }
    //     close(fd);
    //     return current_pos;
    // }

    // void _Catchable checknmakedir(const char *path)
    // {
    //     DIR *dir = opendir(path);
    //     if (dir)
    //     {
    //         /* Directory exists. */
    //         closedir(dir);
    //     }
    //     else if (ENOENT == errno)
    //     {
    //         char buf[512];
    //         snprintf(buf, sizeof(buf), "mkdir -p %s", path);
    //         int retval = system(buf);
    //         if (retval)
    //         {
    //             dbprintlf(FATAL "could not create directory %s, retval %d", path, retval);
    //             throw std::runtime_error("Could not create savedir, retval " + std::to_string(retval));
    //         }
    //     }
    //     else
    //     {
    //         dbprintlf(FATAL "could not create directory %s", path);
    //         throw std::runtime_error("Could not create savedir");
    //     }
}