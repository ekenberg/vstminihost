
#ifndef MINIHOST_H_746583
#define MINIHOST_H_746583

#include <string>
#include "pluginterfaces/vst2.x/aeffectx.h"

namespace minihost
{

typedef AEffect* (*PluginEntryProc)(audioMasterCallback audioMaster);
VstIntPtr VSTCALLBACK HostCallback(AEffect* effect, VstInt32 opcode,
                                   VstInt32 index, VstIntPtr value, void* ptr, float opt);

class PluginLoader
{
public:
    PluginLoader() : _module(0), _mainEntry(0), _blockSize(0), _effect(NULL) {}
    ~PluginLoader();
    
    int loadPlugin(const char *pluginPath);
    int createEffect(float sampleRate, int blockSize);
    void closeEffect();
    
    void setParameter(int index, float param);
    float getParameter(int index);
    void getParameterDisplay(int paramIndex, char *paramDisplay);
    void showEffectProperties();
    void effectProcessing(float **samples, int size, int ch);

public:
    static bool checkPlatform();
    static unsigned long convertStringIdToInt(const char *subId);
    static void convertIntIdToString(const unsigned long id, char *result);
    static void setCurrentShellSubPluginId(const char *id) { _currentShellSubPluginId = convertStringIdToInt(id); }
    static void setCurrentShellSubPluginId(VstInt32 id) { _currentShellSubPluginId = id; }
    static VstInt32 getCurrentShellSubPluginId() { return _currentShellSubPluginId; }
    
protected:
    bool loadLibrary ();
    int getMainEntry();
    
protected:
    std::string _filename;
    void* _module;
    PluginEntryProc _mainEntry;
    int _blockSize;
    AEffect* _effect;
    static VstInt32 _currentShellSubPluginId;
};

}

#endif // MINIHOST_H_746583
