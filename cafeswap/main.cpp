#include "Windows.h"
#include <iostream>
#include "cafeswap/cafeswap.h"
#include "jni.h"

#include <chrono>
#include <thread>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        // Define JVM and ENV
        JavaVM* jvm = NULL;
        JNIEnv* env = NULL;

        jsize javaVMcount;
        if (JNI_GetCreatedJavaVMs(&jvm, 1, &javaVMcount) != JNI_OK || javaVMcount == 0)
            return false;

        jint res = jvm->GetEnv((void**)&env, JNI_VERSION_1_6);

        if (res == JNI_EDETACHED) {
            res = jvm->AttachCurrentThread((void**)&env, nullptr);
        }

        if (res != JNI_OK) {
            return false;
        }

        jmethodID update = env->GetMethodID(env->FindClass("net/motion/main"), "update", "(I)V");
        jmethodID updateHook = env->GetMethodID(env->FindClass("net/motion/hook"), "updateHook", "(I)V");

        CafeSwap::setup(true);
        CafeSwap::addSwap(update, updateHook);

        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        //CafeSwap::removeSwap(update);
        CafeSwap::shutdown(true);

        return false;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return true;
}

