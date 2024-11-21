//
// Created by aidoo on 2024/11/21.
//

#ifndef _LOG_H
#define _LOG_H

#define LOG_TAG "RetroRunner"

#ifdef ANDROID

#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#else
#define LOGI(...) printf(__VA_ARGS__)
#define LOGD(...) printf(__VA_ARGS__)
#define LOGW(...) printf(__VA_ARGS__)
#define LOGE(...) printf(__VA_ARGS__)

#endif

/**
 * 是否输出核心返回的DEBUG级别日志
 */
#define CORE_LOG_DEBUG 1


#define LOGI_IF(_COND_, ...) \
    do{                      \
        if (_COND_){         \
          LOGI(__VA_ARGS__)                   \
        }       \
    }while(false)

#define LOGD_IF(_COND_, ...) \
    do{                      \
        if (_COND_){         \
          LOGD("at %s:%d", __FILE_NAME__, __LINE__);                   \
          LOGD(__VA_ARGS__);                   \
        }       \
    }while(false)

#define LOGE_IF(_COND_, ...) \
    do{                      \
        if (_COND_){         \
          LOGE("at %s:%d", __FILE_NAME__, __LINE__);                   \
          LOGE(__VA_ARGS__);                   \
        }       \
    }while(false)

#define GL_ERROR_STRING(_ERR_) \
    ((_ERR_ != 1280 && _ERR_ != 1282) ? glGetString(_ERR_) : (const GLubyte*)"GL_INVALID_ENUM")

#define GL_CHECK(_FMT_) \
     do { \
        GLenum err = glGetError(); \
        if (err != GL_NO_ERROR) { \
            LOGE("[OPENGL] " _FMT_ " glCheckError: %d - %s at: %s:%d", err, GL_ERROR_STRING(err), __FILE_NAME__, __LINE__); \
        } \
    } while (false);

#define GL_CHECK2(_FMT_, _FMT2_, ...) \
     do { \
        GLenum err = glGetError(); \
        if (err != GL_NO_ERROR) { \
            LOGE("[OPENGL] " _FMT_ " glCheckError: %d - %s at: %s:%d, " _FMT2_, err, GL_ERROR_STRING(err), __FILE_NAME__, __LINE__, __VA_ARGS__); \
        } \
    } while (false);


#endif
