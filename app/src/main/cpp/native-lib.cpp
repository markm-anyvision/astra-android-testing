#include <jni.h>
#include <string>
#include "astra/astra.hpp"
#include "astra_core/astra_core.hpp"
#include "astra_core/capi/astra_host_events.h"

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
        const astra::ColorFrame colorFrame = frame.get<astra::ColorFrame>();

        int width = colorFrame.width();
        int height = colorFrame.height();

        const astra::RgbPixel *colorData = colorFrame.data();

        for (int i = 0; i < width * height; i++) {
            int rgbaOffset = i * 4;
        }

    }
};

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myapplication_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";

    astra_notify_resource_available("usb/0x2bc5/0608/001/031");
    astra::initialize();
    astra::StreamSet streamSet;
    astra::StreamReader reader = streamSet.create_reader();

    reader.stream<astra::ColorStream>().start();

    ColorFrameListener listener;
    reader.add_listener(listener);

    astra_update();


    if (reader.has_new_frame()) {
        hello = "Got a Frame";
    }

    return env->NewStringUTF(hello.c_str());

}