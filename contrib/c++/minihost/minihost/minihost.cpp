// 2012: Added support for Linux/Xlib
// Johan Ekenberg, johan@ekenberg.se

//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.4		$Date: 2006/11/13 09:08:28 $
//
// Category     : VST 2.x SDK Samples
// Filename     : minihost.cpp
// Created by   : Steinberg
// Description  : VST Mini Host
//
// � 2006, Steinberg Media Technologies, All Rights Reserved
//-------------------------------------------------------------------------------------------------------

#if _LINUX
#define __cdecl
#endif

#if _WIN32
#include <windows.h>
#elif TARGET_API_MAC_CARBON
#include <CoreFoundation/CoreFoundation.h>
#elif _LINUX
#include <dlfcn.h>
#include <X11/Xlib.h>
#endif

#include <stdio.h>
#include <memory>

#include "minihost.h"

namespace minihost {

VstInt32 PluginLoader::_currentShellSubPluginId = 0;
    
PluginLoader::~PluginLoader()
{
    if (_module)
    {
#if _WIN32
        FreeLibrary ((HMODULE)_module);
#elif TARGET_API_MAC_CARBON
        CFBundleUnloadExecutable ((CFBundleRef)_module);
        CFRelease ((CFBundleRef)_module);
#elif _LINUX
        dlclose(_module);
#endif
    }
}
    
bool PluginLoader::loadLibrary()
{
#if _WIN32
    _module = LoadLibrary (_filename.c_str());
#elif TARGET_API_MAC_CARBON
    CFStringRef fileNameString = CFStringCreateWithCString (NULL, _filename.c_str(), kCFStringEncodingUTF8);
    if (fileNameString == 0)
        return false;
    CFURLRef url = CFURLCreateWithFileSystemPath (NULL, fileNameString, kCFURLPOSIXPathStyle, false);
    CFRelease (fileNameString);
    if (url == 0)
        return false;
    _module = CFBundleCreate (NULL, url);
    CFRelease (url);
    if (_module && CFBundleLoadExecutable ((CFBundleRef)_module) == false)
        return false;
#elif _LINUX
    _module = dlopen(_filename.c_str(), RTLD_LAZY);
    if (! _module) {
        printf("dlopen error: %s\n", dlerror());
        return false;
    }
#endif
    return _module != 0;
}

int PluginLoader::getMainEntry()
{
#if _WIN32
    _mainEntry = (PluginEntryProc)GetProcAddress ((HMODULE)_module, "VSTPluginMain");
    if (!_mainEntry)
        _mainEntry = (PluginEntryProc)GetProcAddress ((HMODULE)_module, "main");
#elif TARGET_API_MAC_CARBON
    _mainEntry = (PluginEntryProc)CFBundleGetFunctionPointerForName ((CFBundleRef)_module, CFSTR("VSTPluginMain"));
    if (!_mainEntry)
        _mainEntry = (PluginEntryProc)CFBundleGetFunctionPointerForName ((CFBundleRef)_module, CFSTR("main_macho"));
#elif _LINUX
    _mainEntry = (PluginEntryProc) dlsym(_module, "VSTPluginMain");
    if (!_mainEntry) _mainEntry = (PluginEntryProc) dlsym(_module, "main");
#endif
    if (!_mainEntry)
    {
        printf("VST Plugin main entry not found!\n");
        return -1;
    }
    
    return 0;
}
 
bool PluginLoader::checkPlatform()
{
#if VST_64BIT_PLATFORM
    ;//	printf ("*** This is a 64 Bit Build! ***\n");
#else
    ;//	printf ("*** This is a 32 Bit Build! ***\n");
#endif
    
    int sizeOfVstIntPtr = sizeof (VstIntPtr);
//    int sizeOfVstInt32 = sizeof (VstInt32);
    int sizeOfPointer = sizeof (void*);
//    int sizeOfAEffect = sizeof (AEffect);
    
    //	printf ("VstIntPtr = %d Bytes, VstInt32 = %d Bytes, Pointer = %d Bytes, AEffect = %d Bytes\n\n",
    //			sizeOfVstIntPtr, sizeOfVstInt32, sizeOfPointer, sizeOfAEffect);
    
    return sizeOfVstIntPtr == sizeOfPointer;    
}

unsigned long PluginLoader::convertStringIdToInt(const char *subId)
{
    unsigned long result = 0;
    int i;
    if(subId != NULL && strlen(subId) == 4) {
        for(i = 0; i < 4; i++) {
            result |= (unsigned long)(subId[i]) << ((3 - i) * 8);
        }
    }
    return result;
}

void PluginLoader::convertIntIdToString(const unsigned long id, char *result)
{
    for(int i = 0; i < 4; i++) {
        result[i] = (char)(id >> ((3 - i) * 8) & 0xff);
    }
}

int PluginLoader::loadPlugin(const char *fileName)
{
    _filename = fileName;
    if (!checkPlatform ())
    {
        printf ("Platform verification failed! Please check your Compiler Settings!\n");
        return -1;
    }
    
#if _LINUX
    // prepare Xlib for threads
    XInitThreads();
#endif
    
    //	printf ("HOST> Load library...\n");
    
    if (!loadLibrary())
    {
        printf("Failed to load VST Plugin library!\n");
        return -1;
    }

    if (getMainEntry())
    {
        return -1;
    }
    
    return 0;
}
    
int PluginLoader::createEffect(float sampleRate, int blockSize)
{
    _blockSize = blockSize;
    //	printf ("HOST> Create effect...\n");
    _effect = _mainEntry(HostCallback);
    if (!_effect)
    {
        printf ("Failed to create effect instance!\n");
        return -1;
    }
    
    //    printf ("HOST> Init sequence...\n");
    _effect->dispatcher (_effect, effOpen, 0, 0, 0, 0);
    _effect->dispatcher (_effect, effSetSampleRate, 0, 0, 0, sampleRate);
    _effect->dispatcher (_effect, effSetBlockSize, 0, blockSize, 0, 0);
    
    return 0;
}

void PluginLoader::closeEffect()
{
    //	printf ("HOST> Close effect...\n");
    _effect->dispatcher (_effect, effClose, 0, 0, 0, 0);
}

void PluginLoader::setParameter(int index, float param)
{
    _effect->setParameter(_effect, index, param);
}

float PluginLoader::getParameter(int index)
{
    return _effect->getParameter(_effect, index);
}

void PluginLoader::getParameterDisplay(int paramIndex, char *paramDisplay)
{
    _effect->dispatcher(_effect, effGetParamDisplay, paramIndex, 0, paramDisplay, 0);
}

void PluginLoader::showEffectProperties()
{
    //	printf ("HOST> Gathering properties...\n");
    
    char effectName[256] = {0};
    char vendorString[256] = {0};
    char productString[256] = {0};
    
    _effect->dispatcher (_effect, effGetEffectName, 0, 0, effectName, 0);
    _effect->dispatcher (_effect, effGetVendorString, 0, 0, vendorString, 0);
    _effect->dispatcher (_effect, effGetProductString, 0, 0, productString, 0);
    
    printf ("Name = %s\nVendor = %s\nProduct = %s\n\n", effectName, vendorString, productString);
    
    printf ("numPrograms = %d\nnumParams = %d\nnumInputs = %d\nnumOutputs = %d\n\n",
            _effect->numPrograms, _effect->numParams, _effect->numInputs, _effect->numOutputs);
    
    // Iterate programs...
    for (VstInt32 progIndex = 0; progIndex < _effect->numPrograms; progIndex++)
    {
        char progName[256] = {0};
        if (!_effect->dispatcher (_effect, effGetProgramNameIndexed, progIndex, 0, progName, 0))
        {
            _effect->dispatcher (_effect, effSetProgram, 0, progIndex, 0, 0); // Note: old program not restored here!
            _effect->dispatcher (_effect, effGetProgramName, 0, 0, progName, 0);
        }
        printf ("Program %03d: %s\n", progIndex, progName);
    }
    
    printf ("\n");
    
    // Iterate parameters...
    for (VstInt32 paramIndex = 0; paramIndex < _effect->numParams; paramIndex++)
    {
        char paramName[256] = {0};
        char paramLabel[256] = {0};
        char paramDisplay[256] = {0};
        
        _effect->dispatcher (_effect, effGetParamName, paramIndex, 0, paramName, 0);
        _effect->dispatcher (_effect, effGetParamLabel, paramIndex, 0, paramLabel, 0);
        _effect->dispatcher (_effect, effGetParamDisplay, paramIndex, 0, paramDisplay, 0);
        float value = _effect->getParameter(_effect, paramIndex);
        
        printf ("Param %03d: %s [%s %s] (normalized = %f)\n", paramIndex, paramName, paramDisplay, paramLabel, value);
    }
    
    printf ("\n");
    
    // Can-do nonsense...
    static const char* canDos[] =
    {
        "receiveVstEvents",
        "receiveVstMidiEvent",
        "midiProgramNames"
    };
    
    for (VstInt32 canDoIndex = 0; canDoIndex < sizeof (canDos) / sizeof (canDos[0]); canDoIndex++)
    {
        printf ("Can do %s... ", canDos[canDoIndex]);
        VstInt32 result = (VstInt32)_effect->dispatcher(_effect, effCanDo, 0, 0, (void*)canDos[canDoIndex], 0);
        switch (result)
        {
            case 0  : printf ("don't know"); break;
            case 1  : printf ("yes"); break;
            case -1 : printf ("definitely not!"); break;
            default : printf ("?????");
        }
        printf ("\n");
    }
    
    printf ("\n");
}

void PluginLoader::effectProcessing(float **samples, int size, int ch)
{
    float** inputs = 0;
    float** outputs = 0;
    VstInt32 numInputs = _effect->numInputs;
    VstInt32 numOutputs = _effect->numOutputs;
    
    if (numInputs > 0)
    {
        inputs = new float*[numInputs];
        for (VstInt32 i = 0; i < numInputs; i++)
        {
            inputs[i] = new float[_blockSize];
            memset(inputs[i], 0, _blockSize * sizeof (float));
        }
    }
    
    if (numOutputs > 0)
    {
        outputs = new float*[numOutputs];
        for (VstInt32 i = 0; i < numOutputs; i++)
        {
            outputs[i] = new float[_blockSize];
            memset (outputs[i], 0, _blockSize * sizeof (float));
        }
    }
    
    //	printf ("HOST> Resume effect...\n");
    _effect->dispatcher(_effect, effMainsChanged, 0, 1, 0, 0);
    
    int numProcessCycles = (size % _blockSize)?size / _blockSize + 1 : size / _blockSize;
    int blockLoop = 0;
    int blockLoopLastInput = 0;
    int blockLoopLastOutput = 0;
    
    //	printf ("HOST> Process Replacing...\n");
    for (VstInt32 processCount = 0; processCount < numProcessCycles; processCount++)
    {
        blockLoop = processCount * _blockSize;
        
        for (int i = 0; i < ch; ++i) {
            std::fill(&inputs[i][0], &inputs[i][_blockSize], 0);
            
            if (blockLoop + _blockSize > size) {
                blockLoopLastInput = size;
                blockLoopLastOutput = size % _blockSize;
            } else {
                blockLoopLastInput = blockLoop + _blockSize;
                blockLoopLastOutput = _blockSize;
            }
            
            std::copy(&samples[i][blockLoop], &samples[i][blockLoopLastInput], &inputs[i][0]);
            std::fill(&outputs[i][0], &outputs[i][_blockSize], 0);
        }
        
        _effect->processReplacing(_effect, inputs, outputs, _blockSize);
        
        for (int i = 0; i < ch; ++i)
            std::copy(&outputs[i][0], &outputs[i][blockLoopLastOutput], &samples[i][blockLoop]);
    }
    
    //	printf ("HOST> Suspend effect...\n");
    _effect->dispatcher(_effect, effMainsChanged, 0, 0, 0, 0);
    
    if (numInputs > 0)
    {
        for (VstInt32 i = 0; i < numInputs; i++)
            delete [] inputs[i];
        delete [] inputs;
    }
    
    if (numOutputs > 0)
    {
        for (VstInt32 i = 0; i < numOutputs; i++)
            delete [] outputs[i];
        delete [] outputs;
    }
}

//-------------------------------------------------------------------------------------------------------
static int _canHostDo(const char* pluginName, const char* canDoString) {
    bool supported = false;
    
    printf("Plugin '%s' asked if we can do '%s'", pluginName, canDoString);
    if(!strcmp(canDoString, "")) {
        printf("Plugin '%s' asked if we can do an empty string. This is probably a bug.", pluginName);
    }
    else if(!strcmp(canDoString, "sendVstEvents")) {
        supported = false;
    }
    else if(!strcmp(canDoString, "sendVstMidiEvent")) {
        supported = false;
    }
    else if(!strcmp(canDoString, "sendVstTimeInfo")) {
        supported = false;
    }
    else if(!strcmp(canDoString, "receiveVstEvents")) {
        supported = false;
    }
    else if(!strcmp(canDoString, "receiveVstMidiEvent")) {
        supported = false;
    }
    else if(!strcmp(canDoString, "reportConnectionChanges")) {
        supported = false;
    }
    else if(!strcmp(canDoString, "acceptIOChanges")) {
        supported = false;
    }
    else if(!strcmp(canDoString, "sizeWindow")) {
        supported = false;
    }
    else if(!strcmp(canDoString, "offline")) {
        supported = false;
    }
    else if(!strcmp(canDoString, "openFileSelector")) {
        supported = false;
    }
    else if(!strcmp(canDoString, "closeFileSelector")) {
        supported = false;
    }
    else if(!strcmp(canDoString, "startStopProcess")) {
        supported = false;
    }
    else if(!strcmp(canDoString, "shellCategory")) {
        supported = false;
    }
    else if(!strcmp(canDoString, "sendVstMidiEventFlagIsRealtime")) {
        supported = false;
    }
    else {
        printf("Plugin '%s' asked if host canDo '%s' (unimplemented)", pluginName, canDoString);
    }
    
    return supported;
}
    
//-------------------------------------------------------------------------------------------------------
VstIntPtr VSTCALLBACK HostCallback (AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
//    printf("opcode : %d\n", opcode);
    
    // This string is used in a bunch of logging calls below
    char uniqueId[40];
    if(effect != NULL) {
        PluginLoader::convertIntIdToString(effect->uniqueID, uniqueId);
//        printf("vstid: %s\n", uniqueId);
    }
    else {
        // During plugin initialization, the dispatcher can be called without a
        // valid plugin instance, as the AEffect* struct is still not fully constructed
        // at that point.
        strcpy(uniqueId, "????");
    }
    
	VstIntPtr result = 0;

	// Filter idle calls...
	bool filtered = false;
	if (opcode == audioMasterIdle)
	{
		static bool wasIdle = false;
		if (wasIdle)
			filtered = true;
		else
		{
			printf ("(Future idle calls will not be displayed!)\n");
			wasIdle = true;
		}
	}
#if 0
	if (!filtered)
		printf ("PLUG> HostCallback (opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n",
                opcode, index, FromVstPtr<void> (value), ptr, opt);
#endif
	switch (opcode)
	{
		case audioMasterVersion :
			result = kVstVersion;
			break;
        case audioMasterCurrentId:
            result = PluginLoader::getCurrentShellSubPluginId();
            break;
        case audioMasterCanDo:
            result = _canHostDo(uniqueId, (char*)ptr);
            break;
	}
    
	return result;
}

}
