#include "CameraUnit_ASI.hpp"
#include "meb_print.h"
#include "ini.h"
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

const char *bootcount_fname = "./bootcount.dat";

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

typedef struct
{
    const char *progname;
    const char *savedir;
    float cadence,
        maxexposure,
        percentile,
        temperature;
    int maxbin,
        value,
        uncertainty,
        gain;
} asicam_config;

static int inihandler(void *user, const char *section, const char *name, const char *value)
{
    asicam_config *pconfig = (asicam_config *)user;
    if (user == NULL)
    {
        dbprintlf(FATAL "user is NULL");
        exit(0);
    }

#define MATCH(s, n) (strcmp(section, s) == 0 && strcmp(name, #n) == 0)
#define ASSIGNF(n) pconfig->##n = atof(value)
#define ASSIGNI(n) pconfig->##n = atol(value)
#define ASSIGNS(n) pconfig->##n = strdup(value)

#define CHECKS(s, n)      \
    else if (MATCH(s, n)) \
    {                     \
        ASSIGNS(n);       \
    }

#define CHECKF(s, n)      \
    else if (MATCH(s, n)) \
    {                     \
        ASSIGNF(n);       \
    }

#define CHECKI(s, n)      \
    else if (MATCH(s, n)) \
    {                     \
        ASSIGNI(n);       \
    }

    if (MATCH("PROGRAM", name))
    {
        pconfig->progname = strdup(value);
    }
    else if ((strcmp(section, "CONFIG") == 0 && strcmp(name, "savedir") == 0))
    {
        pconfig->savedir = strdup(value);
    }
    else if ((strcmp(section, "CONFIG") == 0 && strcmp(name, "cadence") == 0))
    {
        pconfig->cadence = atof(value);
    }
    else if ((strcmp(section, "CONFIG") == 0 && strcmp(name, "maxexposure") == 0))
    {
        pconfig->maxexposure = atof(value);
    }
    else if ((strcmp(section, "CONFIG") == 0 && strcmp(name, "percentile") == 0))
    {
        pconfig->percentile = atof(value);
    }
    else if ((strcmp(section, "CONFIG") == 0 && strcmp(name, "temperature") == 0))
    {
        pconfig->temperature = atof(value);
    }
    else if ((strcmp(section, "CONFIG") == 0 && strcmp(name, "maxbin") == 0))
    {
        pconfig->maxbin = atol(value);
    }
    else if ((strcmp(section, "CONFIG") == 0 && strcmp(name, "value") == 0))
    {
        pconfig->value = atol(value);
    }
    else if ((strcmp(section, "CONFIG") == 0 && strcmp(name, "uncertainty") == 0))
    {
        pconfig->uncertainty = atol(value);
    }
    else if ((strcmp(section, "CONFIG") == 0 && strcmp(name, "gain") == 0))
    {
        pconfig->gain = atol(value);
    }
    else
    {
        dbprintlf(RED_FG "%s -> %s: %s not accounted for.", section, name, value);
        return 0;
    }
    return 1;
}

#define SEC_TO_USEC(x) ((x)*1000000LLU)
#define SEC_TO_MSEC(x) ((x)*1000LLU)
#define MSEC_TO_USEC(x) ((x)*1000LLU)
#define MIN_SLEEP_USEC SEC_TO_USEC(1)
#define FRAME_TIME_SEC 20
void frame_grabber(CCameraUnit *cam, uint64_t cadence, volatile bool *start_capture) // cadence in seconds
{
    static char *progname = "asicam";
    static char *savedir = "./data/";
    static float maxExposure = 200;
    static float pixelPercentile = 99.7;
    static int pixelTarget = 40000;
    static int pixelUncertainty = 5000;
    static int maxBin = 1;
#define GAIN 200
    static int gain = GAIN;
    static int imgXMin = 300, imgYMin = 800, imgXMax = 2700, imgYMax = 2100;

    static float exposure_1 = 0.2; // 200 ms
    static int bin_1 = 1;          // start with bin 1

    asicam_config pconfig = {
        .progname = progname,
        .savedir = "./data/",
        .cadence = 20,
        .maxexposure = 200,
        .percentile = 99.7,
        .temperature = -20,
        .maxbin = 1,
        .value = 40000,
        .uncertainty = 5000,
        .gain = 200,
    };

    if (ini_parse("./asicam.ini", inihandler, &pconfig) < 0)
    {
        dbprintlf(FATAL "Could not load asicam.ini");
        dbprintlf("Using default values");
    }

    maxExposure = pconfig.maxexposure;
    pixelPercentile = pconfig.percentile;
    pixelTarget = pconfig.value;
    pixelUncertainty = pconfig.uncertainty;
    maxBin = pconfig.maxbin;
    gain = pconfig.gain;
    if (pconfig.progname == NULL || strlen(pconfig.progname) == 0)
    {
        progname = "asicam";
    }
    else
    {
        progname = strdup(pconfig.progname);
    }
    if (pconfig.savedir == NULL || strlen(pconfig.savedir) == 0)
    {
        savedir = "./data/";
    }
    else
    {
        savedir = strdup(pconfig.savedir);
    }

    static bool change_roi = true;
    static bool change_exposure = true;

    int gain_ = cam->SetGainRaw(gain);
    if (gain_ != gain)
    {
        bprintlf(RED_FG "Could not set gain to %d", GAIN);
    }

    if (cam == nullptr)
    {
        dbprintlf(FATAL "Camera is NULL");
        return;
    }

    while (!done)
    {
        uint64_t start = get_msec();
        // aeronomy section
        if (start_capture == nullptr || *start_capture == true)
        {
            // set binning, ROI, exposure
            if (change_roi)
            {
                change_roi = false;
                cam->SetBinningAndROI(bin_1, bin_1, imgXMin, imgXMax, imgYMin, imgYMax);
            }
            if (change_exposure)
            {
                change_exposure = false;
                cam->SetExposure(exposure_1);
            }
            CImageData img = cam->CaptureImage(); // capture frame

            time_t t = time(NULL);
            struct tm tm = *localtime(&t);

            snprintf(dirname, sizeof(dirname), "data/%04d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

            try
            {
                checknmakedir(dirname);
            }
            catch (const std::exception &e)
            {
                dbprintlf(FATAL "Error creating directory: %s", e.what());
                exit(0);
            }

            if (!img.SaveFITS(true, (char const *)dirname, (char const *)"comics_%" PRIu64, start)) // save frame
            {
                bprintlf(FATAL "[%" PRIu64 "] AERO: Could not save FITS", start);
            }
            else
            {
                bprintlf(GREEN_FG "[%" PRIu64 "] AERO: Saved Exposure %.3f s, Bin %d", start, exposure_1, bin_1);
            }
            sync();
            // run auto exposure
            float last_exposure = exposure_1;
            int last_bin = bin_1;
            img.FindOptimumExposure(exposure_1, bin_1, pixelPercentile, pixelTarget, maxExposure, maxBin, 100, pixelUncertainty);
            if (exposure_1 != last_exposure)
            {
                bprintlf(YELLOW_FG "[%" PRIu64 "] AERO: Exposure changed from %.3f s to %.3f s", start, last_exposure, exposure_1);
                change_exposure = true;
            }
            if (bin_1 != last_bin)
            {
                bprintlf(YELLOW_FG "[%" PRIu64 "] AERO: Bin changed from %d to %d", start, last_bin, bin_1);
                change_roi = true;
            }
        }
        start = get_msec() - start;
        if (start < SEC_TO_MSEC(cadence))
        {
            uint64_t res = MSEC_TO_USEC((SEC_TO_MSEC(cadence) - start));
            usleep(res % MIN_SLEEP_USEC);
            res -= res % MIN_SLEEP_USEC;
            while ((res > 0) && (!done))
            {
                usleep(MIN_SLEEP_USEC);
                res -= MIN_SLEEP_USEC;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    uint64_t cadence = 30;

    sleep(1);

    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    CCameraUnit *camera = nullptr;
    int num_cameras = 0;
    int *camera_ids = nullptr;
    std::string *camera_names = nullptr;
    std::thread camera_thread;
    volatile bool start_capture = false;
    CCameraUnit_ASI::ListCameras(num_cameras, camera_ids, camera_names);
    for (int i = 0; i < num_cameras; i++)
    {
        bprintlf(GREEN_FG "Camera %d> %d: %s", i, camera_ids[i], camera_names[i].c_str());
    }
    if (num_cameras == 0)
    {
        bprintlf(RED_FG "No cameras found");
        goto cleanup_init;
    }

    try
    {
        camera = new CCameraUnit_ASI(camera_ids[0]);
    }
    catch (const std::exception &e)
    {
        dbprintlf(FATAL "Could not open camera: %s", e.what());
    }

    if (camera->CameraReady())
    {
        bprintlf(GREEN_FG "Camera %s ready", camera_names[0].c_str());
    }
    else
    {
        bprintlf(RED_FG "Camera %s not ready", camera_names[0].c_str());
        goto close_camera;
    }

    if (!camera->GetUUID().first)
    {
        int cameraID = (int)(uint64_t)camera->GetHandle();
        ASI_ID id;
        static char *idstr = (char *)"LCSTZA01";
        memcpy(id.id, idstr, sizeof(id.id));
        ASI_ERROR_CODE res;
        if ((res = ASISetID(cameraID, id)) != ASI_SUCCESS)
        {
            dbprintlf(RED_FG "Error setting ID: %d", (int)res);
        }
        goto close_camera;
    }
    else
    {
        bprintlf(CYAN_FG "Camera %s: UUID %s", camera_names[0].c_str(), camera->GetUUID().second.c_str());
    }

#define CAMERA_TEMP_SET_POINT -25
    camera->SetTemperature(CAMERA_TEMP_SET_POINT);

    camera_thread = std::thread(frame_grabber, camera, cadence, &start_capture);
    while (!done)
    {
        sleep(1);
        double temperature = camera->GetTemperature();
        bprintf("%s" GREEN_FG "Current CCD Temperature: %+5.1lf C, Cooler %3.0lf%%\r", get_time_now(), temperature, camera->GetCoolerPower());
        if (!start_capture && (fabs(temperature - CAMERA_TEMP_SET_POINT) < 0.5)) // not capturing and temperature delta is 0.5 deg, may heat up later
        {
            start_capture = true;
        }
    }
    camera->CancelCapture();
    camera_thread.join();
    sync();

close_camera:
    delete camera;
cleanup_init:
    if (num_cameras)
    {
        delete[] camera_ids;
        delete[] camera_names;
    }
}

static int create_or_open(const char *path, int create_flags, int open_flags,
                          int *how)
{
    int fd;
    create_flags |= (O_CREAT | O_EXCL);
    open_flags &= ~(O_CREAT | O_EXCL);
    for (;;)
    {
        *how = 1;
        fd = open(path, create_flags, 0666);
        if (fd >= 0)
            break;
        if (errno != EEXIST)
            break;
        *how = 0;
        fd = open(path, open_flags);
        if (fd >= 0)
            break;
        if (errno != ENOENT)
            break;
    }
    return fd;
}

static int GetBootCount()
{
    // 1. Check if file exists, if not ask for step counter and clear all calibration
    char cwd[PATH_MAX];
    off_t sz;
    memset(cwd, 0x0, sizeof(cwd));
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        dbprintlf("Error getting current directory.");
    }
    int newfile = 0;
    int fd = create_or_open(bootcount_fname, O_WRONLY | O_SYNC, O_RDWR | O_SYNC, &newfile);
    int current_pos = 0;
    if (fd < 0)
    {
        dbprintlf(FATAL "Error creating/opening file. Error: %s, Errno: %d.", strerror(errno), errno);
    }
    if (!newfile)
    {
        // 2. File exists
        sz = lseek(fd, 0, SEEK_END);
        if (sz < (int)(sizeof(int)))
        {
            dbprintlf("Size of position file %u, invalid.", (unsigned int)sz);
            goto rewrite;
        }
        lseek(fd, 0, SEEK_SET);
        if (read(fd, &current_pos, sizeof(current_pos)) < 0)
        {
            dbprintlf(RED_FG "Unexpected EOF on %s/%s.", cwd, bootcount_fname);
            current_pos = 0;
        }
    }
rewrite:
    lseek(fd, 0, SEEK_SET);
    int _current_pos = current_pos + 1;
    if (write(fd, &_current_pos, sizeof(_current_pos)) != sizeof(_current_pos))
    {
        dbprintlf(RED_FG "Could not update current position.");
    }
    close(fd);
    return current_pos;
}

void _Catchable checknmakedir(const char *path)
{
    DIR *dir = opendir(path);
    if (dir)
    {
        /* Directory exists. */
        closedir(dir);
    }
    else if (ENOENT == errno)
    {
        char buf[512];
        snprintf(buf, sizeof(buf), "mkdir -p %s", path);
        int retval = system(buf);
        if (retval)
        {
            dbprintlf(FATAL "could not create directory %s, retval %d", path, retval);
            throw std::runtime_error("Could not create savedir, retval " + std::to_string(retval));
        }
    }
    else
    {
        dbprintlf(FATAL "could not create directory %s", path);
        throw std::runtime_error("Could not create savedir");
    }
}