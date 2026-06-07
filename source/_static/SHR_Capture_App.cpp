/**
 * SHR 10GigE Capture & Image Transformation Application
 * Built with Vimba X SDK (2026-1)
 *
 * Covers:
 *  - GigE camera discovery and connection (by IP or first found)
 *  - Full feature configuration (resolution, pixel format, acquisition mode)
 *  - Asynchronous multi-buffer image acquisition
 *  - Image transformation: Bayer debayering and color correction
 *  - Saving frames to disk as raw binary + metadata
 *  - Clean shutdown sequence
 *
 * Build (Linux, CMake):
 *   cmake -S . -B build && cmake --build build
 *
 * Build (Windows, Visual Studio):
 *   Add VmbCPP.props from VimbaX/Examples/VmbCPP/Common/build_vs/
 *
 * System prerequisites (Linux, GigE):
 *   sudo ip link set eth0 mtu 9000
 *   sudo sysctl -w net.core.rmem_max=33554432
 *   sudo sysctl -w net.core.rmem_default=33554432
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <csignal>
#include <cstring>

#include "VmbCPP/VmbCPP.h"
#include "VmbImageTransform/VmbTransform.h"

using namespace VmbCPP;

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

struct AppConfig
{
    std::string cameraIP       = "";          // Empty = use first found camera
    int         frameCount     = 10;          // Number of frames to capture (0 = continuous)
    int         bufferCount    = 5;           // Number of queued frame buffers
    std::string pixelFormat    = "BayerGR12"; // SHR common output format
    bool        saveFrames     = true;        // Write frames to disk
    std::string outputDir      = "./frames";  // Output directory
    bool        colorCorrect   = false;       // Apply identity color matrix (extend as needed)

    // Debayer algorithm: 0=2x2(default), 1=3x3, 2=LCAA(8-bit only), 3=LCAAV(8-bit only)
    int         debayerMode    = 0;
};

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

static std::atomic<bool>  g_running{true};
static std::atomic<int>   g_framesCaptured{0};
static std::mutex         g_consoleMutex;
static AppConfig          g_config;

void signalHandler(int) { g_running = false; }

void log(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(g_consoleMutex);
    std::cout << "[SHR] " << msg << std::endl;
}

// ---------------------------------------------------------------------------
// Image Transform Helper
// ---------------------------------------------------------------------------

/**
 * Transforms a raw camera frame into an RGB8 output buffer.
 * Handles Bayer debayering + optional 3x3 color correction.
 *
 * @param pSrcData   Pointer to raw frame data from camera
 * @param srcFormat  VmbPixelFormat_t of the source frame (e.g. BayerGR12)
 * @param width      Frame width in pixels
 * @param height     Frame height in pixels
 * @param outBuffer  Pre-allocated output buffer (width * height * 3 bytes for RGB8)
 * @param applyCC    Whether to apply the color correction matrix
 * @return true on success
 */
bool transformFrame(
    void*              pSrcData,
    VmbPixelFormat_t   srcFormat,
    VmbUint32_t        width,
    VmbUint32_t        height,
    std::vector<uint8_t>& outBuffer,
    bool               applyCC = false)
{
    VmbImage sourceImage{};
    VmbImage destinationImage{};

    sourceImage.Size      = sizeof(sourceImage);
    destinationImage.Size = sizeof(destinationImage);

    sourceImage.Data      = pSrcData;
    destinationImage.Data = outBuffer.data();

    // Describe the source pixel format
    VmbError_t err = VmbSetImageInfoFromPixelFormat(srcFormat, width, height, &sourceImage);
    if (err != VmbErrorSuccess)
    {
        log("VmbSetImageInfoFromPixelFormat failed: " + std::to_string(err));
        return false;
    }

    // Describe the destination: RGB8 output
    err = VmbSetImageInfoFromInputImage(&sourceImage, VmbPixelLayoutRGB, 8, &destinationImage);
    if (err != VmbErrorSuccess)
    {
        log("VmbSetImageInfoFromInputImage failed: " + std::to_string(err));
        return false;
    }

    // Build transform parameter list
    std::vector<VmbTransformInfo> transformParams;

    // Debayer mode
    VmbTransformInfo debayerInfo{};
    VmbDebayerMode_t debayerMode = VmbDebayerMode2x2;
    switch (g_config.debayerMode)
    {
        case 1: debayerMode = VmbDebayerMode3x3;   break;
        case 2: debayerMode = VmbDebayerModeLCAA;  break;
        case 3: debayerMode = VmbDebayerModeLCAAV; break;
        default: debayerMode = VmbDebayerMode2x2;  break;
    }
    VmbSetDebayerMode(debayerMode, &debayerInfo);
    transformParams.push_back(debayerInfo);

    // Optional color correction (identity matrix by default — tune as needed)
    VmbTransformInfo ccInfo{};
    if (applyCC)
    {
        const VmbFloat_t mat[] = {
            1.0f, 0.0f, 0.0f,   // rr  rg  rb
            0.0f, 1.0f, 0.0f,   // gr  gg  gb
            0.0f, 0.0f, 1.0f    // br  bg  bb
        };
        VmbSetColorCorrectionMatrix3x3(mat, &ccInfo);
        transformParams.push_back(ccInfo);
    }

    err = VmbImageTransform(
        &sourceImage,
        &destinationImage,
        transformParams.data(),
        static_cast<VmbUint32_t>(transformParams.size()));

    if (err != VmbErrorSuccess)
    {
        log("VmbImageTransform failed: " + std::to_string(err));
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Save frame to disk
// ---------------------------------------------------------------------------

void saveFrame(const std::vector<uint8_t>& rgbData, VmbUint32_t width, VmbUint32_t height, int index)
{
    // Write a simple binary PPM (P6) for easy viewing with ImageMagick, GIMP, etc.
    std::ostringstream path;
    path << g_config.outputDir << "/frame_" << std::setw(5) << std::setfill('0') << index << ".ppm";

    std::ofstream file(path.str(), std::ios::binary);
    if (!file)
    {
        log("Could not write frame: " + path.str());
        return;
    }

    // PPM header
    file << "P6\n" << width << " " << height << "\n255\n";
    file.write(reinterpret_cast<const char*>(rgbData.data()), rgbData.size());
    log("Saved: " + path.str());
}

// ---------------------------------------------------------------------------
// Frame Observer — called by Vimba X on every completed frame
// ---------------------------------------------------------------------------

class FrameObserver : public IFrameObserver
{
public:
    explicit FrameObserver(CameraPtr pCamera)
        : IFrameObserver(pCamera)
    {}

    void FrameReceived(const FramePtr pFrame) override
    {
        VmbFrameStatusType status;
        if (VmbErrorSuccess != pFrame->GetReceiveStatus(status) ||
            status != VmbFrameStatusComplete)
        {
            log("Incomplete or failed frame — requeueing");
            m_pCamera->QueueFrame(pFrame);
            return;
        }

        VmbUint32_t width  = 0;
        VmbUint32_t height = 0;
        VmbPixelFormat_t pixelFormat{};
        void* pData = nullptr;

        pFrame->GetWidth(width);
        pFrame->GetHeight(height);
        pFrame->GetPixelFormat(pixelFormat);
        pFrame->GetImage(pData);

        int frameIndex = ++g_framesCaptured;
        log("Frame " + std::to_string(frameIndex) +
            " received: " + std::to_string(width) + "x" + std::to_string(height));

        // Allocate RGB8 output buffer: width * height * 3
        std::vector<uint8_t> rgbBuffer(width * height * 3);

        if (transformFrame(pData, pixelFormat, width, height, rgbBuffer, g_config.colorCorrect))
        {
            if (g_config.saveFrames)
                saveFrame(rgbBuffer, width, height, frameIndex);
        }

        // Stop if we've reached the requested frame count
        if (g_config.frameCount > 0 && frameIndex >= g_config.frameCount)
        {
            g_running = false;
            return; // Don't requeue — we're done
        }

        // Requeue for next frame
        m_pCamera->QueueFrame(pFrame);
    }
};

// ---------------------------------------------------------------------------
// Feature helpers
// ---------------------------------------------------------------------------

VmbError_t setFeatureEnum(CameraPtr& camera, const char* name, const char* value)
{
    FeaturePtr feature;
    VmbError_t err = camera->GetFeatureByName(name, feature);
    if (err != VmbErrorSuccess) return err;
    return feature->SetValue(value);
}

VmbError_t setFeatureInt(CameraPtr& camera, const char* name, VmbInt64_t value)
{
    FeaturePtr feature;
    VmbError_t err = camera->GetFeatureByName(name, feature);
    if (err != VmbErrorSuccess) return err;
    return feature->SetValue(value);
}

VmbError_t runCommand(CameraPtr& camera, const char* name)
{
    FeaturePtr feature;
    VmbError_t err = camera->GetFeatureByName(name, feature);
    if (err != VmbErrorSuccess) return err;
    return feature->RunCommand();
}

VmbError_t getFeatureInt(CameraPtr& camera, const char* name, VmbInt64_t& value)
{
    FeaturePtr feature;
    VmbError_t err = camera->GetFeatureByName(name, feature);
    if (err != VmbErrorSuccess) return err;
    return feature->GetValue(value);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // Parse optional CLI args: [cameraIP] [frameCount]
    if (argc > 1) g_config.cameraIP    = argv[1];
    if (argc > 2) g_config.frameCount  = std::stoi(argv[2]);

    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Create output directory
    std::string mkdirCmd = "mkdir -p " + g_config.outputDir;
    std::system(mkdirCmd.c_str());

    log("=== SHR 10GigE Capture & Transform App ===");
    log("Camera IP  : " + (g_config.cameraIP.empty() ? "(first found)" : g_config.cameraIP));
    log("Frames     : " + (g_config.frameCount == 0 ? "continuous" : std::to_string(g_config.frameCount)));
    log("Buffers    : " + std::to_string(g_config.bufferCount));
    log("Format     : " + g_config.pixelFormat);
    log("Output     : " + g_config.outputDir);

    // -----------------------------------------------------------------------
    // 1. Initialize Vimba X SDK
    // -----------------------------------------------------------------------
    VmbSystem& system = VmbSystem::GetInstance();

    VmbError_t err = system.Startup();
    if (err != VmbErrorSuccess)
    {
        log("VmbSystem::Startup failed: " + std::to_string(err));
        return 1;
    }
    log("Vimba X started");

    // -----------------------------------------------------------------------
    // 2. Discover and open the camera
    // -----------------------------------------------------------------------
    CameraPtr camera;

    if (!g_config.cameraIP.empty())
    {
        // Open by IP address (GigE)
        err = system.OpenCameraByID(
            g_config.cameraIP.c_str(), VmbAccessModeFull, camera);
        if (err != VmbErrorSuccess)
        {
            log("Failed to open camera at " + g_config.cameraIP + ": " + std::to_string(err));
            system.Shutdown();
            return 1;
        }
        log("Opened camera at " + g_config.cameraIP);
    }
    else
    {
        // Discover all cameras — for GigE, register observer to trigger discovery
        // then call GetCameras which waits for responses
        CameraPtrVector cameras;
        err = system.GetCameras(cameras);
        if (err != VmbErrorSuccess || cameras.empty())
        {
            log("No cameras found: " + std::to_string(err));
            system.Shutdown();
            return 1;
        }

        camera = cameras.at(0);
        err = camera->Open(VmbAccessModeFull);
        if (err != VmbErrorSuccess)
        {
            log("Failed to open camera: " + std::to_string(err));
            system.Shutdown();
            return 1;
        }

        std::string camName, camModel, camSerial;
        camera->GetName(camName);
        camera->GetModel(camModel);
        camera->GetSerialNumber(camSerial);
        log("Opened camera: " + camName + " | Model: " + camModel + " | S/N: " + camSerial);
    }

    // -----------------------------------------------------------------------
    // 3. Configure camera features
    // -----------------------------------------------------------------------

    // Pixel format — SHR 10GigE typically outputs Bayer 12-bit
    err = setFeatureEnum(camera, "PixelFormat", g_config.pixelFormat.c_str());
    if (err != VmbErrorSuccess)
        log("Warning: Could not set PixelFormat to " + g_config.pixelFormat +
            " (err=" + std::to_string(err) + ") — using camera default");

    // Use full sensor resolution (SHR goes up to 245 MP)
    // These may already be at max — setting explicitly is a best-practice
    VmbInt64_t maxWidth = 0, maxHeight = 0;
    getFeatureInt(camera, "WidthMax",  maxWidth);
    getFeatureInt(camera, "HeightMax", maxHeight);
    if (maxWidth > 0)  setFeatureInt(camera, "Width",  maxWidth);
    if (maxHeight > 0) setFeatureInt(camera, "Height", maxHeight);

    VmbInt64_t width = 0, height = 0;
    getFeatureInt(camera, "Width",  width);
    getFeatureInt(camera, "Height", height);
    log("Resolution: " + std::to_string(width) + " x " + std::to_string(height));

    // Continuous acquisition mode
    err = setFeatureEnum(camera, "AcquisitionMode", "Continuous");
    if (err != VmbErrorSuccess)
        log("Warning: Could not set AcquisitionMode (err=" + std::to_string(err) + ")");

    // GigE-specific: set GVSP burst size for higher bandwidth on 10GigE
    // (recommended >= 32 for 5GigE+ cameras per System Settings doc)
    setFeatureInt(camera, "GVSPBurstSize", 64);

    // -----------------------------------------------------------------------
    // 4. Allocate frame buffers and set up asynchronous capture
    // -----------------------------------------------------------------------
    VmbUint32_t payloadSize = 0;
    err = camera->GetPayloadSize(payloadSize);
    if (err != VmbErrorSuccess)
    {
        log("GetPayloadSize failed: " + std::to_string(err));
        camera->Close();
        system.Shutdown();
        return 1;
    }
    log("Payload size: " + std::to_string(payloadSize) + " bytes ("
        + std::to_string(payloadSize / 1024 / 1024) + " MB per frame)");

    IFrameObserverPtr observer(new FrameObserver(camera));
    FramePtrVector frames(g_config.bufferCount);

    for (auto& frame : frames)
    {
        frame.reset(new Frame(payloadSize));
        frame->RegisterObserver(observer);
        err = camera->AnnounceFrame(frame);
        if (err != VmbErrorSuccess)
        {
            log("AnnounceFrame failed: " + std::to_string(err));
            camera->Close();
            system.Shutdown();
            return 1;
        }
    }

    err = camera->StartCapture();
    if (err != VmbErrorSuccess)
    {
        log("StartCapture failed: " + std::to_string(err));
        camera->Close();
        system.Shutdown();
        return 1;
    }

    for (auto& frame : frames)
        camera->QueueFrame(frame);

    // -----------------------------------------------------------------------
    // 5. Start acquisition on the camera
    // -----------------------------------------------------------------------
    err = runCommand(camera, "AcquisitionStart");
    if (err != VmbErrorSuccess)
    {
        log("AcquisitionStart failed: " + std::to_string(err));
        camera->EndCapture();
        camera->FlushQueue();
        camera->RevokeAllFrames();
        camera->Close();
        system.Shutdown();
        return 1;
    }

    log("Acquisition started — press Ctrl+C to stop");

    // -----------------------------------------------------------------------
    // 6. Run until done or interrupted
    // -----------------------------------------------------------------------
    while (g_running)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // -----------------------------------------------------------------------
    // 7. Clean shutdown sequence (per C++ API Manual)
    // -----------------------------------------------------------------------
    log("Stopping acquisition...");

    runCommand(camera, "AcquisitionStop");
    camera->EndCapture();
    camera->FlushQueue();
    camera->RevokeAllFrames();
    camera->Close();
    system.Shutdown();

    log("Done. Captured " + std::to_string(g_framesCaptured.load()) + " frame(s).");
    return 0;
}
