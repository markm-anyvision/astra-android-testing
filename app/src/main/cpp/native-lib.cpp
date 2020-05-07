#include <jni.h>
#include <string>
#include <astra/astra.hpp>
#include <astra_core/astra_core.hpp>
#include <astra_core/StreamReader.hpp>
#include <astra_core/capi/astra_host_events.h>
#include <astra_core/capi/astra_core.h>
#include <android/log.h>
#include "opencv2/core.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#define TAG "CCode"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,    TAG, __VA_ARGS__)
int a=0;
bool isRunning = true;

static const int32_t m_depthMaxValue = (256 * 24); // Depth max value

class AstraFrameListener : public astra::FrameListener {
public:
    AstraFrameListener() {
    }

    virtual void on_frame_ready(astra::StreamReader &reader, astra::Frame &frame) override {
        LOGD("Frame ready");
        const astra::ColorFrame colorFrame = frame.get<astra::ColorFrame>();

        int width = colorFrame.width();
        int height = colorFrame.height();
        LOGD("RGB Height: %d", height);

        cv::Mat rgb(height, width, CV_8UC3, (uint8_t *) colorFrame.data());
        cv::cvtColor(rgb, rgb, cv::COLOR_BGRA2RGB);
        /*if (a % 50 == 0) {
            LOGD("Saving image");
            cv::imwrite("/storage/self/primary/Download/" + std::to_string(a) + "_rgb.png", rgb);
            rgb.release();
        }*/
        const astra::DepthFrame depthFrame = frame.get<astra::DepthFrame>();
        int depthHeight = depthFrame.height();
        LOGD("Depth Height: %d", depthHeight);

        /*if (a % 50 == 0) {
            cv::Mat image(400, width, CV_16UC1, (uint16_t *) depthFrame.data());
            image.convertTo(image, CV_8U, 256.0 / m_depthMaxValue);
            std::vector<int> compression_params;
            compression_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
            compression_params.push_back(0);
            cv::imwrite("/storage/self/primary/Download/" + std::to_string(a) + "_depth.png", image,
                        compression_params);
        }*/
        a++;

    }
};

extern "C" JNIEXPORT void JNICALL
Java_com_example_myapplication_MainActivity_openAstraStream(
        JNIEnv *env,
        jobject /* this */) {
    isRunning = true;
    LOGD("Astra initialized");
    astra::StreamSet streamSet;
    astra::StreamReader reader= streamSet.create_reader();

    auto depthStream = reader.stream<astra::DepthStream>();
    astra::ImageStreamMode depthMode;
    depthMode.set_width(640);
    depthMode.set_height(400);
    depthMode.set_pixel_format(astra_pixel_formats::ASTRA_PIXEL_FORMAT_DEPTH_MM);
    depthMode.set_fps(30);
    depthStream.set_mode(depthMode);
    depthStream.start();

    auto rgbStream = reader.stream<astra::ColorStream>();
    rgbStream.start();

    AstraFrameListener listener;
    reader.add_listener(listener);

    while (isRunning) {
        astra_update();
    }
    rgbStream.stop();
    depthStream.stop();
    LOGD("RGB Stream stopped");
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_myapplication_MainActivity_stopDepthStream(
        JNIEnv *env,
        jobject /* this */) {
    LOGD("Stopping steam");
    isRunning = false;
}
