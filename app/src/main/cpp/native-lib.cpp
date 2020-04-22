#include <jni.h>
#include <string>
#include <astra/astra.hpp>
#include <astra_core/astra_core.hpp>
#include <astra_core/StreamReader.hpp>
#include <astra_core/capi/astra_host_events.h>
#include <astra_core/capi/astra_core.h>
#include <android/log.h>

#define TAG "CCode"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,    TAG, __VA_ARGS__)
int a=0;
bool isRunning = true;

static const int32_t m_depthMaxValue = (256 * 24); // Depth max value

enum ColorMode
{
    MODE_COLOR,
    MODE_IR_16,
    MODE_IR_RGB,
};

class ColorFrameListener : public astra::FrameListener {
public:
    ColorFrameListener() {
    }

    virtual void on_frame_ready(astra::StreamReader &reader, astra::Frame &frame) override {
        LOGD("Frame ready");
        const astra::ColorFrame colorFrame = frame.get<astra::ColorFrame>();

        int width = colorFrame.width();
        int height = colorFrame.height();
        LOGD("RGB Height: %d", height);

        const astra::RgbPixel *colorData = colorFrame.data();

        const astra::DepthFrame depthFrame = frame.get<astra::DepthFrame>();
        int depthHeight = depthFrame.height();
        LOGD("Depth Height: %d", depthHeight);
        /*for (int i = 0; i < width * height; i++) {
            int rgbaOffset = i * 4;
        }*/

    }
};

class DepthFrameListener : public astra::FrameListener {
public:
    DepthFrameListener() {
    }

    virtual void on_frame_ready(astra::StreamReader &reader, astra::Frame &frame) override {
        LOGD("Frame ready");
        const astra::DepthFrame depthFrame = frame.get<astra::DepthFrame>();

        int width = depthFrame.width();
        int height = depthFrame.height();
        //LOGD("byte_length: %d", depthFrame.byte_length());
        //LOGD("bytes_per_pixel: %d", depthFrame.bytes_per_pixel());
        //LOGD("length: %d", depthFrame.length());
        //LOGD("Image size: %d", image.size);
        //LOGD("Image rows: %d", image.rows);
        //image.convertTo(image, CV_8U, 256.0 / m_depthMaxValue);
        /*if (a % 100 == 0) {
            cv::Mat image(height, width, CV_16UC1, (void *) depthFrame.data());
            image.convertTo(image, CV_8U, 256.0 / m_depthMaxValue);
            std::vector<int> compression_params;
            compression_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
            compression_params.push_back(0);
            cv::imwrite("/storage/self/primary/Download/" + std::to_string(a) + ".png", image,
                        compression_params);
        }
        a++;*/
        /*cv::Mat image(height, width, CV_16UC1, (void*)depthFrame.data());
        std::vector<int> compression_params;
        compression_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
        compression_params.push_back(0);
        cv::imwrite("/storage/self/primary/Download/"+std::to_string(a)+".png", compression_params);
        a++;*/

    }
};


extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myapplication_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());

}

extern "C" JNIEXPORT void JNICALL
Java_com_example_myapplication_MainActivity_openDepthStream(
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

    DepthFrameListener listener;
    reader.add_listener(listener);

    while (isRunning) {
        astra_update();
    }
    depthStream.stop();
    LOGD("Stream stopped");
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_myapplication_MainActivity_openRGBStream(
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

    ColorFrameListener listener;
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
