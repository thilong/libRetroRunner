#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "utils/jnistring.h"
#include "types/log.h"
#include "app/app_context.h"
#include "app/environment.h"
#include "video/video_context.h"
#include "input/input_context.h"
#include "types/error.h"
#include "app/paths.h"
#include "types/app_state.h"


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
    extern "C" jobject gRRNativeRef = nullptr;

}

using namespace libRetroRunner;

//this method is called on sub thread.
void OnFrontendNotifyCallback(void *data) {
    auto notifyObj = (FrontendNotifyObject *) data;
    int notifyType = notifyObj->GetNotifyType();
    JNIEnv *env = nullptr;
    gVm->AttachCurrentThread(&env, nullptr);
    jclass rrNativeClass = env->GetObjectClass(gRRNativeRef);
    jmethodID gRRNative_onFrontendNotify = env->GetStaticMethodID(rrNativeClass, "onEmuAppNotification", "(I)V");
    env->CallStaticVoidMethod(rrNativeClass, gRRNative_onFrontendNotify, notifyType);
    gVm->DetachCurrentThread();

    switch (notifyType) {
        case kAppNotificationContentLoaded:
            LOGD_JNI("frontend notify: content loaded");
            break;
        case kAppNotificationTerminated:
            LOGD_JNI("frontend notify: app terminated");
            break;
        case kAppNotificationGameGeometryChanged:
            LOGD_JNI("frontend notify: game geometry changed");
            break;
        default:
            break;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_initEnv(JNIEnv *env, jclass clazz, jobject rrNative) {
    env->GetJavaVM(&(gVm));
    gRRNativeRef = env->NewGlobalRef(rrNative);
}

extern "C" JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_create(JNIEnv *env, jclass clazz, jstring rom_path, jstring core_path, jstring system_path, jstring save_path) {
    const std::shared_ptr<AppContext> &app = AppContext::CreateNew();
    JString rom(env, rom_path);
    JString core(env, core_path);
    JString system(env, system_path);
    JString save(env, save_path);
    app->CreateWithPaths(rom.stdString(), core.stdString(), system.stdString(), save.stdString());
    LOGD_JNI("new app context created: \n\trom:\t%s \n\tcore:\t%s \n\tsystem:\t%s \n\tsave:\t%s", rom.cString(), core.cString(), system.cString(), save.cString());
    app->SetFrontendNotify(OnFrontendNotifyCallback);

    app->AddCommand(AppCommands::kInitApp);
    app->AddCommand(AppCommands::kLoadCore);
    app->AddCommand(AppCommands::kLoadContent);
    app->AddCommand(AppCommands::kInitComponents);
    LOGD_JNI("app created and some commands has been added.");
}

extern "C" JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_addVariable(JNIEnv *env, jclass clazz, jstring key, jstring value, jboolean notify_core) {
    DeclareEnvironment();
    JString keyVal(env, key);
    JString valueVal(env, key);
    environment->UpdateVariable(keyVal.stdString(), valueVal.stdString(), notify_core);
}


extern "C" JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_pause(JNIEnv *env, jclass clazz) {
    const std::shared_ptr<AppContext> &app = AppContext::Current();
    if (app.get() != nullptr) app->Pause();
}

extern "C" JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_resume(JNIEnv *env, jclass clazz) {
    const std::shared_ptr<AppContext> &app = AppContext::Current();
    if (app.get() != nullptr) app->Resume();
}

extern "C" JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_reset(JNIEnv *env, jclass clazz) {
    const std::shared_ptr<AppContext> &app = AppContext::Current();
    if (app.get() != nullptr) app->Reset();
}

extern "C" JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_stop(JNIEnv *env, jclass clazz) {
    const std::shared_ptr<AppContext> &app = AppContext::Current();
    if (app.get() != nullptr) app->Stop();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_aidoo_retrorunner_RRNative_step(JNIEnv *env, jclass clazz) {
    const std::shared_ptr<AppContext> &app = AppContext::Current();
    if (app) return app->Step();
    return false;
}

extern "C" JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_setFastForward(JNIEnv *env, jclass clazz, jfloat multiplier) {
    auto app = AppContext::Current();
    if (app.get() == nullptr) return;
    auto gameCtx = app->GetGameRuntimeContext();
    gameCtx->SetGameSpeed(multiplier);
    LOGD_JNI("set game speed to x%f", multiplier);
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
    auto app = AppContext::Current();
    if (!app) return 0;
    auto gameCtx = app->GetGameRuntimeContext();
    return gameCtx->GetGeometryRotation();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_getGameWidth(JNIEnv *env, jclass clazz) {
    auto app = AppContext::Current();
    if (!app) return 0;
    auto gameCtx = app->GetGameRuntimeContext();
    return (int) gameCtx->GetGeometryWidth();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_getGameHeight(JNIEnv *env, jclass clazz) {
    auto app = AppContext::Current();
    if (!app) return 0;
    auto gameCtx = app->GetGameRuntimeContext();
    return (int) gameCtx->GetGeometryHeight();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_takeScreenshot(JNIEnv *env, jclass clazz, jstring path, jboolean wait_for_result) {
    auto app = AppContext::Current();
    if (!app) return RRError::kAppNotRunning;
    JString pathVal(env, path);
    std::string savePath = pathVal.stdString();
    return app->AddTakeScreenshotCommand(savePath, wait_for_result);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_saveRam(JNIEnv *env, jclass clazz, jstring path, jboolean wait_for_result) {
    auto app = AppContext::Current();
    if (!app) return RRError::kAppNotRunning;
    JString pathVal(env, path);
    std::string savePath = pathVal.stdString();
    return app->AddSaveSRAMCommand(savePath, wait_for_result);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_loadRam(JNIEnv *env, jclass clazz, jstring path, jboolean wait_for_result) {
    auto app = AppContext::Current();
    if (!app) return RRError::kAppNotRunning;
    JString pathVal(env, path);
    std::string savePath = pathVal.stdString();
    return app->AddLoadSRAMCommand(savePath, wait_for_result);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_saveState(JNIEnv *env, jclass clazz, jint idx, jboolean wait_for_result) {
    auto app = AppContext::Current();
    if (!app) return RRError::kAppNotRunning;
    auto gameCtx = app->GetGameRuntimeContext();
    auto savePath = gameCtx->GetSaveStateFilePath(idx);
    return app->AddSaveStateCommand(savePath, wait_for_result);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_loadState(JNIEnv *env, jclass clazz, jint idx, jboolean wait_for_result) {
    auto app = AppContext::Current();
    if (!app) return RRError::kAppNotRunning;
    auto gameCtx = app->GetGameRuntimeContext();
    auto savePath = gameCtx->GetSaveStateFilePath(idx);
    return app->AddLoadStateCommand(savePath, wait_for_result);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_saveStateWithPath(JNIEnv *env, jclass clazz, jstring path, jboolean wait_for_result) {
    auto app = AppContext::Current();
    if (!app) return RRError::kAppNotRunning;
    JString pathVal(env, path);
    std::string savePath = pathVal.stdString();
    return app->AddLoadStateCommand(savePath, wait_for_result);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_aidoo_retrorunner_RRNative_loadStateWithPath(JNIEnv *env, jclass clazz, jstring path, jboolean wait_for_result) {
    auto app = AppContext::Current();
    if (!app) return RRError::kAppNotRunning;
    JString pathVal(env, path);
    std::string savePath = pathVal.stdString();
    return app->AddLoadStateCommand(savePath, wait_for_result);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_OnSurfaceChanged(JNIEnv *env, jclass clazz, jobject surface, jlong surfaceId, jint width, jint height) {
    auto app = AppContext::Current();
    if (app) {
        app->OnSurfaceChanged(env, surface, surfaceId, width, height);
    } else {
        LOGE_JNI("Emu app context is null.");
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_aidoo_retrorunner_RRNative_destroy(JNIEnv *env, jclass clazz) {
    auto app = AppContext::Current();
    if (app) app->Destroy();
}