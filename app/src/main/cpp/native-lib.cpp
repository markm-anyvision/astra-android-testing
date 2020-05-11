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
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <pthread.h>

#define TAG "CCode"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,    TAG, __VA_ARGS__)
int a=0;
bool isRunning = true;

static const int32_t m_depthMaxValue = (256 * 24); // Depth max value


GST_DEBUG_CATEGORY_STATIC (debug_category);
#define GST_CAT_DEFAULT debug_category

/*
 * These macros provide a way to store the native pointer to CustomData, which might be 32 or 64 bits, into
 * a jlong, which is always 64 bits, without warnings.
 */
#if GLIB_SIZEOF_VOID_P == 8
# define GET_CUSTOM_DATA(env, thiz, fieldID) (CustomData *)env->GetLongField (thiz, fieldID)
# define SET_CUSTOM_DATA(env, thiz, fieldID, data) env->SetLongField (thiz, fieldID, (jlong)data)
#else
# define GET_CUSTOM_DATA(env, thiz, fieldID) (CustomData *)(jint)(*env)->GetLongField (env, thiz, fieldID)
# define SET_CUSTOM_DATA(env, thiz, fieldID, data) (*env)->SetLongField (env, thiz, fieldID, (jlong)(jint)data)
#endif

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
    jobject app;            /* Application instance, used to call its methods. A global reference is kept. */
    GstElement *pipeline;   /* The running pipeline */
    GMainContext *context;  /* GLib context used to run the main loop */
    GMainLoop *main_loop;   /* GLib main loop */
    gboolean initialized;   /* To avoid informing the UI multiple times about the initialization */
    GstElement *video_sink; /* The video sink element which receives VideoOverlay commands */
    GstElement *source_sink; /* The source element */
    GstElement *videoconverter1;
    ANativeWindow *native_window; /* The Android native window where video will be rendered */
} CustomData;

/* These global variables cache values which are not changing during execution */
static pthread_t gst_app_thread;
static pthread_key_t current_jni_env;
static JavaVM *java_vm;
static jfieldID custom_data_field_id;
static jmethodID set_message_method_id;
static jmethodID on_gstreamer_initialized_method_id;


static JNIEnv *attach_current_thread(void)
{
    JNIEnv *env;
    JavaVMAttachArgs args;

    GST_DEBUG ("Attaching thread %p", g_thread_self());
    args.version = JNI_VERSION_1_4;
    args.name = NULL;
    args.group = NULL;

    if (java_vm->AttachCurrentThread (&env, &args) < 0)
    {
        GST_ERROR ("Failed to attach current thread");
        return NULL;
    }

    return env;
}

static void detach_current_thread(void *env)
{
    GST_DEBUG ("Detaching thread %p", g_thread_self());
    java_vm->DetachCurrentThread();
}

static JNIEnv *get_jni_env(void)
{
    JNIEnv *env;

    if ((env = (JNIEnv*)pthread_getspecific(current_jni_env)) == NULL)
    {
        env = attach_current_thread();
        LOGD("Attaching current thread");
        pthread_setspecific(current_jni_env, env);
    }

    return env;
}

static void error_cb(GstBus *bus, GstMessage *message, CustomData *jni_data)
{
    gchar *message_string;
    GError *err;
    gchar *debug_info;
    jstring jmessage;
    JNIEnv *env = get_jni_env();

    gst_message_parse_error(message, &err, &debug_info);
    message_string =
            g_strdup_printf("Error received from element %s: %s",
                            GST_OBJECT_NAME(message->src),
                            err->message);

    g_clear_error(&err);
    g_free(debug_info);

    LOGD("Error message: %s",message_string);

    jmessage = env->NewStringUTF(message_string);

    if (env->ExceptionCheck())
    {
        GST_ERROR("Failed to call Java method");
        env->ExceptionClear();
    }
    env->DeleteLocalRef(jmessage);

    g_free(message_string);
    gst_element_set_state(jni_data->pipeline, GST_STATE_NULL);
}

/* Notify UI about pipeline state changes */
static void state_changed_cb (GstBus *bus, GstMessage *msg, CustomData *data) {
    LOGD("state_changed_cb");
    GstState old_state, new_state, pending_state;
    gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
    /* Only pay attention to messages coming from the pipeline, not its children */
    if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
        gchar *message = g_strdup_printf("State changed to %s", gst_element_state_get_name(new_state));
        LOGD("State changed: %s", message);
        g_free (message);
    }
}

/* Check if all conditions are met to report GStreamer as initialized.
 * These conditions will change depending on the application */
static void check_initialization_complete (CustomData *data) {
    LOGD("check_initialization_complete");
    JNIEnv *env = get_jni_env ();
    if (!data->initialized && data->native_window && data->main_loop) {
        GST_DEBUG ("Initialization complete, notifying application. native_window:%p main_loop:%p", data->native_window, data->main_loop);

        /* The main loop is running and we received a native window, inform the sink about it */
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (data->video_sink), (guintptr)data->native_window);

        env->CallVoidMethod (data->app, on_gstreamer_initialized_method_id);
        if (env->ExceptionCheck ()) {
            GST_ERROR ("Failed to call Java method");
            env->ExceptionClear ();
        }
        data->initialized = TRUE;
    }
}

static void eos_cb(GstBus const *bus, GstMessage const *msg, CustomData *jni_data)
{
    gst_element_set_state(jni_data->pipeline, GST_STATE_PAUSED);
}

/* main */
static void *app_function(void *userdata)
{
    GstBus *bus;
    GstMessage *message;
    CustomData *gst_data = (CustomData *)userdata;
    GSource *bus_source;
    GMainContext *context;

    context = g_main_context_new();

    gst_data->pipeline = gst_pipeline_new("detector_pipeline");
    gst_data->videoconverter1 = gst_element_factory_make("videoconvert", "videoconvert1");
    gst_data->source_sink = gst_element_factory_make("astrasrc", "astrasrc");
    if(gst_data->source_sink) {
        LOGD("astrasrc src is created");
    }
    else {
        LOGD("astrasrc src not created");
    }
    gst_data->video_sink = gst_element_factory_make("glimagesink", "vsink");


    g_object_set (gst_data->video_sink,
                  "sync", false,
                  NULL);

    g_object_set(gst_data->video_sink, "rotate-method", 1, NULL);


    gst_bin_add_many(GST_BIN(gst_data->pipeline),
                     gst_data->source_sink,
                     gst_data->video_sink,
                     gst_data->videoconverter1,
                     NULL);

    gst_element_link_many(gst_data->source_sink,
                          gst_data->videoconverter1,
                          gst_data->video_sink,
                          NULL);

    if(gst_data->native_window)
    {
        gst_video_overlay_set_window_handle(
                GST_VIDEO_OVERLAY(gst_data->video_sink),
                (guintptr)gst_data->native_window);
    };

    bus = gst_element_get_bus(gst_data->pipeline);
    bus_source = gst_bus_create_watch(bus);

    g_source_set_callback(bus_source,
                          (GSourceFunc)gst_bus_async_signal_func,
                          NULL,
                          NULL);

    g_source_attach(bus_source, context);
    g_source_unref(bus_source);

    g_signal_connect(G_OBJECT(bus),
                     "message::error",
                     G_CALLBACK(error_cb),
                     gst_data);

    g_signal_connect(G_OBJECT(bus),
                     "message::eos",
                     (GCallback)eos_cb,
                     gst_data);

    g_signal_connect(G_OBJECT(bus),
                     "message::state-changed",
                     (GCallback) state_changed_cb,
                     gst_data);

    gst_object_unref(bus);

    gst_data->main_loop = g_main_loop_new(context, FALSE);
    check_initialization_complete(gst_data);
    g_main_loop_run(gst_data->main_loop);


    g_main_loop_unref(gst_data->main_loop);
    gst_data->main_loop = NULL;

    g_main_context_unref(context);
    gst_element_set_state(gst_data->pipeline, GST_STATE_NULL);
    gst_object_unref(gst_data->source_sink);
    gst_object_unref(gst_data->video_sink);
    gst_object_unref(gst_data->videoconverter1);
    gst_object_unref(gst_data->pipeline);

    return NULL;
}

/*
 * Java Bindings
 */

/* Instruct the native code to create its internal data structure, pipeline and thread */
static void gst_native_init (JNIEnv* env, jobject thiz) {
    CustomData *data = g_new0 (CustomData, 1);
    SET_CUSTOM_DATA (env, thiz, custom_data_field_id, data);
    GST_DEBUG_CATEGORY_INIT (debug_category, "Astra", 0, "Android Astra");
    gst_debug_set_threshold_for_name("Astra", GST_LEVEL_DEBUG);
    GST_DEBUG ("Created CustomData at %p", data);
    data->app = env->NewGlobalRef (thiz);
    GST_DEBUG ("Created GlobalRef for app object at %p", data->app);
    pthread_create (&gst_app_thread, NULL, &app_function, data);
}

/* Quit the main loop, remove the native thread and free resources */
static void gst_native_finalize (JNIEnv* env, jobject thiz) {
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data) return;
    GST_DEBUG ("Quitting main loop...");
    g_main_loop_quit (data->main_loop);
    GST_DEBUG ("Waiting for thread to finish...");
    pthread_join (gst_app_thread, NULL);
    GST_DEBUG ("Deleting GlobalRef for app object at %p", data->app);
    env->DeleteGlobalRef (data->app);
    GST_DEBUG ("Freeing CustomData at %p", data);
    g_free (data);
    SET_CUSTOM_DATA (env, thiz, custom_data_field_id, NULL);
    GST_DEBUG ("Done finalizing");
}

/* Set pipeline to PLAYING state */
static void gst_native_play (JNIEnv* env, jobject thiz) {
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data) return;
    GST_DEBUG ("Setting state to PLAYING");
    gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
}

/* Set pipeline to PAUSED state */
static void gst_native_pause (JNIEnv* env, jobject thiz) {
    CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
    if (!data) return;
    GST_DEBUG ("Setting state to PAUSED");
    gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
}

/* Static class initializer: retrieve method and field IDs */
static jboolean gst_native_class_init (JNIEnv* env, jclass klass) {
    custom_data_field_id = env->GetFieldID (klass, "native_custom_data", "J");
    on_gstreamer_initialized_method_id = env->GetMethodID (klass, "onGStreamerInitialized", "()V");

    if (!custom_data_field_id || !set_message_method_id || !on_gstreamer_initialized_method_id) {
        /* We emit this message through the Android log instead of the GStreamer log because the later
         * has not been initialized yet.
         */
        __android_log_print (ANDROID_LOG_ERROR, "Astra", "The calling class does not implement all necessary interface methods");
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

void gst_native_surface_init(JNIEnv * env, jobject thiz, jobject surface)
{
    CustomData *gst_data = GET_CUSTOM_DATA(env, thiz, custom_data_field_id);

    if (!gst_data)
    {
        return;
    }

    GST_DEBUG("Received surface %p", surface);
    if (gst_data->native_window)
    {
        GST_DEBUG ("Releasing previous native window %p", gst_data->native_window);
        ANativeWindow_release (gst_data->native_window);
    }

    gst_data->native_window = ANativeWindow_fromSurface(env, surface);
    GST_DEBUG("Got Native Window %p", gst_data->native_window);

    if (gst_data->video_sink)
    {
        GST_DEBUG
        ("Pipeline already created, notifying the vsink about the native window.");
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY (gst_data->video_sink),
                                            (guintptr) gst_data->native_window);
    }
    else
    {
        GST_DEBUG("Pipeline not created yet, vsink will later be notified about the native window.");
    }

    check_initialization_complete(gst_data);
}

void gst_native_surface_finalize(JNIEnv * env, jobject thiz)
{
    CustomData *gst_data = GET_CUSTOM_DATA(env, thiz, custom_data_field_id);

    if (!gst_data)
    {
        GST_WARNING("Received surface finalize but there is no gst_data. Ignoring.");
        return;
    }

    GST_DEBUG("Releasing Native Window %p", gst_data->native_window);
    ANativeWindow_release(gst_data->native_window);
    gst_data->native_window = NULL;

    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(gst_data->video_sink),
                                        (guintptr)NULL);
}

/* List of implemented native methods */
static JNINativeMethod native_methods[] = {
        { "nativeInit", "()V", (void *) gst_native_init},
        { "nativeFinalize", "()V", (void *) gst_native_finalize},
        { "nativePlay", "()V", (void *) gst_native_play},
        { "nativePause", "()V", (void *) gst_native_pause},
        { "nativeClassInit", "()Z", (void *) gst_native_class_init},
        {"nativeSurfaceInit",       "(Ljava/lang/Object;)V",    (void *)gst_native_surface_init},
        {"nativeSurfaceFinalize",   "()V",                      (void *)gst_native_surface_finalize}
};

/* Library initializer */
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;

    java_vm = vm;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        __android_log_print (ANDROID_LOG_ERROR, "tutorial-2", "Could not retrieve JNIEnv");
        return 0;
    }
    jclass klass = env->FindClass ("com/example/myapplication/MainActivity");
    env->RegisterNatives (klass, native_methods, G_N_ELEMENTS(native_methods));

    pthread_key_create (&current_jni_env, detach_current_thread);

    return JNI_VERSION_1_4;
}

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
        if (a % 50 == 0) {
            LOGD("Saving image");
            cv::imwrite("/storage/self/primary/Download/" + std::to_string(a) + "_rgb.png", rgb);
            rgb.release();
        }
        const astra::DepthFrame depthFrame = frame.get<astra::DepthFrame>();
        int depthHeight = depthFrame.height();
        LOGD("Depth Height: %d", depthHeight);

        if (a % 50 == 0) {
            cv::Mat image(400, width, CV_16UC1, (uint16_t *) depthFrame.data());
            image.convertTo(image, CV_8U, 256.0 / m_depthMaxValue);
            std::vector<int> compression_params;
            compression_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
            compression_params.push_back(0);
            cv::imwrite("/storage/self/primary/Download/" + std::to_string(a) + "_depth.png", image,
                        compression_params);
        }
        a++;

    }
};

extern "C" JNIEXPORT void JNICALL
Java_com_example_myapplication_MainActivity_startGstreamer(
        JNIEnv *env,
        jobject /* this */) {
    astra::initialize();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_myapplication_MainActivity_openAstraStream(
        JNIEnv *env,
        jobject /* this */) {
    isRunning = true;
    LOGD("Astra initialized");
    LOGD("1");
    astra::StreamSet streamSet;
    astra::StreamReader reader= streamSet.create_reader();
    LOGD("2");
    auto depthStream = reader.stream<astra::DepthStream>();
    astra::ImageStreamMode depthMode;
    depthMode.set_width(640);
    depthMode.set_height(400);
    depthMode.set_pixel_format(astra_pixel_formats::ASTRA_PIXEL_FORMAT_DEPTH_MM);
    depthMode.set_fps(30);
    depthStream.set_mode(depthMode);
    depthStream.start();
    LOGD("3");
    auto rgbStream = reader.stream<astra::ColorStream>();
    rgbStream.start();
    LOGD("4");
    AstraFrameListener listener;
    reader.add_listener(listener);
    LOGD("5");
    while (isRunning) {
        if(reader.has_new_frame()) {
            LOGD("New Frame");
        }
        else {
            LOGD("No Frame");
        }
        astra_update();
    }
    rgbStream.stop();
    depthStream.stop();
    //astra::terminate();
    LOGD("RGB Stream stopped");
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_myapplication_MainActivity_stopDepthStream(
        JNIEnv *env,
        jobject /* this */) {
    LOGD("Stopping steam");
    isRunning = false;
}

