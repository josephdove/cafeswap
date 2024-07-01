#include "Windows.h"
#include <iostream>
#include <psapi.h>
#include <vector>
#include <list>

#include "../minhook/MinHook.h"
#include "cafeswap.h"
#include "jni.h"

extern HMODULE JVMHandle = GetModuleHandleA("jvm.dll");

typedef void* (__fastcall* def_compileMethod)(JavaH::Method**, int, int, JavaH::Method**, int, int, void*);

extern def_compileMethod compileMethodOriginal = NULL;
extern LPVOID compileFunction = NULL;

// https://github.com/openjdk/jdk17u/blob/master/src/hotspot/share/compiler/compileBroker.cpp#L1388
void* __fastcall compileMethodDetour(JavaH::Method** method, int osr_bci, int comp_level, JavaH::Method** hot_method, int hot_count, int compile_reason, void* thread) {
    //if ((*method)->code != nullptr)
    //    std::cout << std::hex << ((*method)->code)->getInternalCodeEntry() << std::endl;

    for (std::list<CafeSwap::Swap*>::iterator swapPtr = CafeSwap::swapList.begin(); swapPtr != CafeSwap::swapList.end(); ++swapPtr) {
        CafeSwap::Swap* swap = *swapPtr;
        if (!swap->wasCompiled) {
            // Compile both of them
            compileMethodOriginal((JavaH::Method**)&swap->toSwap, -1, 1, hot_method, 0, 6, thread);
            compileMethodOriginal((JavaH::Method**)&swap->swapTo, -1, 1, hot_method, 0, 6, thread);

            swap->wasCompiled = true;
        }

        if (!swap->wasSwapped) {
            // Check if they have been compiled
            void* toSwapCodeEntry = (swap->toSwap->code)->getInternalCodeEntry();
            void* swapToCodeEntry = (swap->swapTo->code)->getInternalCodeEntry();
            if (toSwapCodeEntry != nullptr && swapToCodeEntry != nullptr) {
                // Get and save original bytes and address
                swap->originalAddress = toSwapCodeEntry;
                swap->originalBytes = new uint8_t[5];
                CafeSwap::unsafeMemcpy(swap->originalBytes, toSwapCodeEntry, 5);

                // Build payload
                uint8_t payload[5];
                payload[0] = 0xE9; // jmp

                // Create address of jmp
                int32_t jmpAddress = ((uint64_t)swapToCodeEntry- (uint64_t)toSwapCodeEntry - 5);

                // Copy jmp address to the payload
                CafeSwap::unsafeMemcpy(&payload[1], &jmpAddress, sizeof(jmpAddress));

                // Overwrite code entry with jmp to new entry
                CafeSwap::unsafeMemcpy(toSwapCodeEntry, payload, sizeof(payload));

                swap->wasSwapped = true;
            }
        }
    }

    return compileMethodOriginal(method, osr_bci, comp_level, hot_method, hot_count, compile_reason, thread);
};


namespace CafeSwap {
    std::list<CafeSwap::Swap*> swapList;

    void unsafeMemcpy(void* dest, const void* src, size_t n) {
        DWORD oldProtect;
        // Change the protection of the destination memory to be writable
        if (VirtualProtect(dest, n, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            // Perform the copy using standard memcpy
            memcpy(dest, src, n);
            // Restore the old protection
            VirtualProtect(dest, n, oldProtect, &oldProtect);
        } else {
            std::cout << n << std::endl;
        }
    }

    uintptr_t patternScan(HMODULE hModule, const char* pattern) {
        auto patternToBytes = [](const char* pattern) -> std::vector<int> {
            std::vector<int> bytes;
            const char* current = pattern;
            while (*current) {
                if (*current == '?') {
                    bytes.push_back(-1);
                    current++;
                    if (*current == '?') {
                        current++;
                    }
                }
                else {
                    bytes.push_back(strtoul(current, nullptr, 16));
                    current += 2;
                }
                if (*current == ' ') {
                    current++;
                }
            }
            return bytes;
            };

        auto compareMemory = [](const BYTE* data, const std::vector<int>& pattern) -> bool {
            for (size_t i = 0; i < pattern.size(); ++i) {
                if (pattern[i] != -1 && data[i] != pattern[i]) {
                    return false;
                }
            }
            return true;
            };

        MODULEINFO moduleInfo;
        if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo))) {
            return 0;
        }

        uintptr_t start = (uintptr_t)moduleInfo.lpBaseOfDll;
        uintptr_t end = start + moduleInfo.SizeOfImage;
        std::vector<int> patternBytes = patternToBytes(pattern);

        for (uintptr_t i = start; i < end; i++) {
            if (compareMemory(reinterpret_cast<BYTE*>(i), patternBytes)) {
                return i;
            }
        }

        return 0;
    }

	bool setup(bool shouldInitMH) {
        if (shouldInitMH) {
            MH_Initialize();
        }

        uint64_t patternBase = CafeSwap::patternScan(JVMHandle, "E8 ?? ?? ?? ?? 48 8B AC 24 20 13 00 00");
        if (patternBase == 0)
            return false;

        uint32_t patternOffset = *(uint32_t*)(patternBase + 1);
        compileFunction = (LPVOID)(patternBase + 5 + patternOffset); // 5 is the length of the jmp instruction

        if (MH_CreateHook(compileFunction, compileMethodDetour, (LPVOID*)&compileMethodOriginal) != MH_OK) {
            return false;
        }

        if (MH_EnableHook(compileFunction) != MH_OK) {
            return false;
        }

        return true;
	}

    bool addSwap(jmethodID toSwap, jmethodID swapTo) {
        // Not initialized
        if (compileFunction == NULL)
            return false;

        // toSwap or swapTo is null (jmethodID not found)
        if (toSwap == NULL || swapTo == NULL)
            return false;

        CafeSwap::Swap* newSwap = new CafeSwap::Swap();
        newSwap->toSwap = *(JavaH::Method**)toSwap;
        newSwap->swapTo = *(JavaH::Method**)swapTo;
        newSwap->originalAddress = NULL;
        newSwap->originalBytes = NULL;
        newSwap->wasCompiled = false;
        newSwap->wasSwapped = false;

        swapList.push_back(newSwap);

        return true;
    }

    bool removeSwap(jmethodID originalFunc) {
        // Not initialized
        if (compileFunction == NULL)
            return false;

        CafeSwap::Swap* swap = NULL;
        for (std::list<CafeSwap::Swap*>::iterator swapPtr = CafeSwap::swapList.begin(); swapPtr != CafeSwap::swapList.end(); ++swapPtr) {
            if ((*swapPtr)->toSwap == *(JavaH::Method**)originalFunc) {
                swap = *swapPtr;
            }
        }

        if (swap != NULL) {
            // Check if there are original bytes and we already swapped
            if (swap->originalBytes == NULL || swap->originalAddress == NULL || !swap->wasSwapped)
                return false;

            // Remove swap from array
            CafeSwap::swapList.remove(swap);

            // Restore original bytes
            CafeSwap::unsafeMemcpy(swap->originalAddress, swap->originalBytes, 5);

            return true;
        }

        return false;
    }

    bool shutdown(bool shouldKillMH) {
        // Not initialized
        if (compileFunction == NULL)
            return false;

        if (MH_RemoveHook(compileFunction) != MH_OK)
            return false;

        if (shouldKillMH) {
            if (MH_Uninitialize() != MH_OK)
                return false;
        }

        for (std::list<CafeSwap::Swap*>::iterator swapPtr = CafeSwap::swapList.begin(); swapPtr != CafeSwap::swapList.end(); ++swapPtr) {
            CafeSwap::Swap* swap = *swapPtr;

            // Check if there are original bytes and we already swapped
            if (swap->originalBytes == NULL || swap->originalAddress == NULL || !swap->wasSwapped)
                continue;

            // Restore original bytes
            CafeSwap::unsafeMemcpy(swap->originalAddress, swap->originalBytes, 5);
        }

        // Clear swap lit
        CafeSwap::swapList.clear();

        return true;
    }
}