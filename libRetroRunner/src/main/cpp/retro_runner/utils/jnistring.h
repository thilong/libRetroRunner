
#ifndef _JNISTRING_H
#define _JNISTRING_H

#include <jni.h>
#include <string>

class JString {
public:
    JString(JNIEnv *env, jstring javaString);
    ~JString();

    const char* cString();
    std::string stdString();

    JString(const JString& other) = delete;
    JString& operator=(const JString& other) = delete;

private:
    JNIEnv* env;
    jstring javaString;
    const char* nativeString;
};


#endif
