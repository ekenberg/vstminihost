VST_SDK = ${HOME}/SDKs/vstsdk2.4

minihost: objects
	g++ -I${VST_SDK} -ldl -lX11 -D_LINUX -o minihost *.o

objects:
	g++ -I${VST_SDK} -D_LINUX -c minieditor.cpp
	g++ -I${VST_SDK} -D_LINUX -c minihost.cpp

# Loading 32-bit vst plugins on a 64-bit platform:
minihost32: objects32
	g++ -I${VST_SDK} -m32 -ldl -lX11 -D_LINUX -o minihost32 *.o

objects32:
	g++ -I${VST_SDK} -m32 -D_LINUX -c minieditor.cpp
	g++ -I${VST_SDK} -m32 -D_LINUX -c minihost.cpp
