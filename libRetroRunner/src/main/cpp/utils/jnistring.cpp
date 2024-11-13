

#include "jnistring.h"

JString::JString(JNIEnv *env, jstring javaString) :
        env(env), javaString(javaString) {
    nativeString = env->GetStringUTFChars(javaString, 0);
}

JString::~JString() {
    env->ReleaseStringUTFChars(javaString, nativeString);
}

const char *JString::cString() {
    return strdup(nativeString);
}

std::string JString::stdString() {
    return {nativeString};
}
