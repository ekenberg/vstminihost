
#include <iostream>
#include <iterator>
#include "minihost/minihost.h"

void test();

int main()
{
    test();
    return 0;
}

void test()
{
    const int SIZE = 5120;
    const int BUFSIZE = 512;
    minihost::PluginLoader pluginLoader;
    
    // if you use shell plugins like waves than just set subid here
    // minihost::PluginLoader::setCurrentShellSubPluginId("76CM");
    
    // dll or vst
    pluginLoader.loadPlugin("/Library/Audio/Plug-ins/VST/T-RackS CS Classic EQ.vst");
    
    // set sample rate and buffer size
    pluginLoader.createEffect(44100, BUFSIZE);
    
    // set first parameter value
    pluginLoader.setParameter(0, 0.432f);
    
    // set second parameter value
    pluginLoader.setParameter(1, 0.9f);
    
    // show plugin's parameters (printf)
    pluginLoader.showEffectProperties();
    
    float *inputs[2];
    inputs[0] = new float[SIZE];
    inputs[1] = new float[SIZE];
    
    // zero buffers
    std::fill(&inputs[0][0], &inputs[0][SIZE], 0);
    std::fill(&inputs[1][0], &inputs[0][SIZE], 0);

    // copy your data to the buffer
    //
    // memcpy(inputs[0], yourSrc[0], 5120 * sizeof(float) );
    // memcpy(inputs[1], yourSrc[1], 5120 * sizeof(float) );
    //
    //
    
    // std::copy(&inputs[0][0], &inputs[0][SIZE], std::ostream_iterator<float>(std::cout, " "));
    // std::copy(&inputs[1][0], &inputs[1][SIZE], std::ostream_iterator<float>(std::cout, " "));
    
    // process your buffers inputs, your buffer size and your channel number
    pluginLoader.effectProcessing(inputs, SIZE, 2);
    
    delete [] inputs[1];
    delete [] inputs[0];
    
    pluginLoader.closeEffect();
}
