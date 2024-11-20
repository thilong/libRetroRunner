//
// Created by aidoo on 2024/10/31.
//

#ifndef LIBRETROSD_LOG_H
#define LIBRETROSD_LOG_H

#include <android/log.h>

/*是否输出核心的日志中debug级别的信息*/
#define CORE_DEUBG_LOG 1

#define LOG_TAG "RetroRunner"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

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

#define GL_ERROR_STRING2(_ERR_) glGetString(_ERR_)


#define GL_CHECK(_FMT_) \
     do { \
        GLenum err = glGetError(); \
        if (err != GL_NO_ERROR) { \
            LOGE("[VIDEO] " _FMT_ " glCheckError: %d - %s at: %s:%d", err, GL_ERROR_STRING(err), __FILE_NAME__, __LINE__); \
        } \
    } while (false);

#define GL_CHECK2(_FMT_, _FMT2_, ...) \
     do { \
        GLenum err = glGetError(); \
        if (err != GL_NO_ERROR) { \
            LOGE("[VIDEO] " _FMT_ " glCheckError: %d - %s at: %s:%d, " _FMT2_, err, GL_ERROR_STRING(err), __FILE_NAME__, __LINE__, __VA_ARGS__); \
        } \
    } while (false);



#endif //LIBRETROSD_LOG_H
