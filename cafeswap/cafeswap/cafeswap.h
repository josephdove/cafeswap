#pragma once
#include "Windows.h"
#include <list>

#include "jni.h"

namespace JavaH
{
    constexpr uintptr_t stackAlignment = 0x7000;

    class CMethod
    {
    public:
        void** vtable;
        uint8_t compileMode;

        void* getMainCode()
        {
            if (this == nullptr)
                return this;
            return *(void**)((uintptr_t)this + 0x20);
        }

        void* getInternalCodeEntry()
        {
            void* code = getMainCode();
            if (code == nullptr)
                return nullptr;

            uint8_t* codeBytes = (uint8_t*)code;
            uint32_t target = 0 - stackAlignment;
            for (uintptr_t i = 0; i < 0x1000; i++)
            {
                uint32_t dword = *(uint32_t*)(codeBytes + i);
                if (dword == target)
                {
                    void* entryPoint = (void*)((uintptr_t)codeBytes + i - 0x3);
                    return entryPoint;
                }
            }
            return nullptr;
        }
    };

    class Method
    {
    public:
        char padding[0x48];
        CMethod* code;
    };
}


namespace CafeSwap {
    struct Swap {
        JavaH::Method* toSwap;
        JavaH::Method* swapTo;
        void* originalAddress;
        uint8_t* originalBytes;
        bool wasCompiled;
        bool wasSwapped;
    };

    extern std::list<CafeSwap::Swap*> swapList;

    void unsafeMemcpy(void* dest, const void* src, size_t n);
	uintptr_t patternScan(HMODULE hModule, const char* pattern);
    bool addSwap(jmethodID toSwap, jmethodID swapFunc);
    bool removeSwap(jmethodID originalFunc);
    bool shutdown(bool shouldKillMH);
    bool setup(bool shouldInitMH);
;}