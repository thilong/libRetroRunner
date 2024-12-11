#include <jni.h>
#include "utils/jnistring.h"
#include "types/log.h"
#include "app/app_context.h"
#include "app/environment.h"
#include "video/video_context.h"
#include "input/input_context.h"
#include "types/error.h"
#include "app/paths.h"

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
    const std::shared_ptr<AppContext> &app = AppContext::CreateNew();
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
    const std::shared_ptr<AppContext> &app = AppContext::Current();
    if (app.get() != nullptr) app->Pause();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_resume(JNIEnv *env, jclass clazz) {
    const std::shared_ptr<AppContext> &app = AppContext::Current();
    if (app.get() != nullptr) app->Resume();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_reset(JNIEnv *env, jclass clazz) {
    const std::shared_ptr<AppContext> &app = AppContext::Current();
    if (app.get() != nullptr) app->Reset();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_stop(JNIEnv *env, jclass clazz) {
    const std::shared_ptr<AppContext> &app = AppContext::Current();
    if (app.get() != nullptr) app->Stop();
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

extern "C" JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_setFastForward(JNIEnv *env, jclass clazz, jdouble multiplier) {
    DeclareEnvironment();
    LOGD_JNI("set fast forward speed to x%f", multiplier);
    environment->SetFastForwardSpeed(multiplier);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_aidoo_retrorunner_RRNative_updateButtonState(JNIEnv *env, jclass clazz, jint player, jint key, jboolean down) {
    auto app = AppContext::Current();
    if (app) {
        auto input = app->GetInput();
        if (input) {
            return input->UpdateButton(player, key, down);
        }
    }
    return false;
}

extern "C" JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_updateAxisState(JNIEnv *env, jclass clazz, jint player, jint analog, jint axis_button, jfloat value) {
    auto app = AppContext::Current();
    if (app) {
        auto input = app->GetInput();
        if (input) {
            input->UpdateAxis(player, analog, axis_button, value);
        }
    }
}

extern "C" JNIEXPORT jdouble JNICALL
Java_com_aidoo_retrorunner_RRNative_getAspectRatio(JNIEnv *env, jclass clazz) {
    DeclareEnvironment(0);
    return environment->GetAspectRatio();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_getGameWidth(JNIEnv *env, jclass clazz) {
    DeclareEnvironment(0);
    return environment->GetGameWidth();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_getGameHeight(JNIEnv *env, jclass clazz) {
    DeclareEnvironment(0);
    return environment->GetGameHeight();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_takeScreenshot(JNIEnv *env, jclass clazz, jstring path, jboolean wait_for_result) {
    auto app = AppContext::Current();
    if (app.get() == nullptr) return RRError::kAppNotRunning;
    JString pathVal(env, path);
    std::string savePath = pathVal.stdString();
    return app->AddTakeScreenshotCommand(savePath, wait_for_result);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_saveRam(JNIEnv *env, jclass clazz, jstring path, jboolean wait_for_result) {
    auto app = AppContext::Current();
    if (app.get() == nullptr) return RRError::kAppNotRunning;
    JString pathVal(env, path);
    std::string savePath = pathVal.stdString();
    return app->AddSaveSRAMCommand(savePath, wait_for_result);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_loadRam(JNIEnv *env, jclass clazz, jstring path, jboolean wait_for_result) {
    auto app = AppContext::Current();
    if (app.get() == nullptr) return RRError::kAppNotRunning;
    JString pathVal(env, path);
    std::string savePath = pathVal.stdString();
    return app->AddLoadSRAMCommand(savePath, wait_for_result);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_saveState(JNIEnv *env, jclass clazz, jint idx, jboolean wait_for_result) {
    auto app = AppContext::Current();
    if (app.get() == nullptr) return RRError::kAppNotRunning;
    auto paths = app->GetPaths();
    auto savePath = paths->GetSaveStatePath(idx);
    return app->AddSaveStateCommand(savePath, wait_for_result);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_loadState(JNIEnv *env, jclass clazz, jint idx, jboolean wait_for_result) {
    auto app = AppContext::Current();
    if (app.get() == nullptr) return RRError::kAppNotRunning;
    auto paths = app->GetPaths();
    auto savePath = paths->GetSaveStatePath(idx);
    return app->AddLoadStateCommand(savePath, wait_for_result);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_saveStateWithPath(JNIEnv *env, jclass clazz, jstring path, jboolean wait_for_result) {
    auto app = AppContext::Current();
    if (app.get() == nullptr) return RRError::kAppNotRunning;
    JString pathVal(env, path);
    std::string savePath = pathVal.stdString();
    return app->AddLoadStateCommand(savePath, wait_for_result);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_loadStateWithPath(JNIEnv *env, jclass clazz, jstring path, jboolean wait_for_result) {
    auto app = AppContext::Current();
    if (app.get() == nullptr) return RRError::kAppNotRunning;
    JString pathVal(env, path);
    std::string savePath = pathVal.stdString();
    return app->AddLoadStateCommand(savePath, wait_for_result);
}