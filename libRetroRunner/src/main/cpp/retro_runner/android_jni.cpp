#include <jni.h>
#include "utils/jnistring.h"
#include "types/log.h"
#include "app/app_context.h"
#include "app/environment.h"
#include "video/video_context.h"

#define LOGD_JNI(...) LOGD("[JNI] " __VA_ARGS__)
#define LOGW_JNI(...) LOGW("[JNI] " __VA_ARGS__)
#define LOGE_JNI(...) LOGE("[JNI] " __VA_ARGS__)
#define LOGI_JNI(...) LOGI("[JNI] " __VA_ARGS__)

#define DeclareEnvironment(_RET_FAIL) \
    const std::shared_ptr<AppContext>& app = AppContext::Current(); \
    if (app.get() == nullptr) return _RET_FAIL; \
    auto environment = app->GetEnvironment(); \
    if (env == nullptr) return _RET_FAIL

namespace libRetroRunner {
    extern "C" JavaVM *gVm = nullptr;


}

using namespace libRetroRunner;

extern "C" JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_initEnv(JNIEnv *env, jclass clazz) {
    env->GetJavaVM(&(gVm));
}

extern "C" JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_create(JNIEnv *env, jclass clazz, jstring rom_path, jstring core_path, jstring system_path, jstring save_path) {
    const std::shared_ptr<AppContext> &app = AppContext::CreateInstance();
    JString rom(env, rom_path);
    JString core(env, core_path);
    JString system(env, system_path);
    JString save(env, save_path);
    app->SetPaths(rom.stdString(), core.stdString(), system.stdString(), save.stdString());
    LOGD_JNI("new app context created: \n\trom:\t%s \n\tcore:\t%s \n\tsystem:\t%s \n\tsave:\t%s", rom.cString(), core.cString(), system.cString(), save.cString());
}

extern "C" JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_addVariable(JNIEnv *env, jclass clazz, jstring key, jstring value, jboolean notify_core) {
    DeclareEnvironment();
    JString keyVal(env, key);
    JString valueVal(env, key);
    environment->UpdateVariable(keyVal.stdString(), valueVal.stdString(), notify_core);
}
extern "C" JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_start(JNIEnv *env, jclass clazz) {
    const std::shared_ptr<AppContext> &app = AppContext::Current();
    if (app.get() != nullptr) app->Start();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_pause(JNIEnv *env, jclass clazz) {
    // TODO: implement pause()
}
extern "C"
JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_resume(JNIEnv *env, jclass clazz) {
    // TODO: implement resume()
}
extern "C"
JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_reset(JNIEnv *env, jclass clazz) {
    // TODO: implement reset()
}
extern "C"
JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_stop(JNIEnv *env, jclass clazz) {
    // TODO: implement stop()
}
extern "C" JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_setVideoSurface(JNIEnv *env, jclass clazz, jobject surface) {
    auto app = AppContext::Current();
    if (app) {
        void *argv[] = {env, surface};
        app->SetVideoSurface(2, argv);
    }
}
extern "C" JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_setVideoSurfaceSize(JNIEnv *env, jclass clazz, jint width, jint height) {
    auto app = AppContext::Current();
    if (app) {
        auto video = app->GetVideo();
        if (video) {
            video->SetSurfaceSize(width, height);
        }
    }
}