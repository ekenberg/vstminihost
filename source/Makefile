VST_SDK = ${HOME}/SDKs/vstsdk2.4

# Note: builds with debug symbols and pthreads, so that you can debug a plugin with gdb

all:	vstminihost vstminihost32

vstminihost: objects
	g++ -I${VST_SDK} -g -ggdb3 -ldl -lpthread -lX11 -D_LINUX -o vstminihost *.o

objects:
	g++ -I${VST_SDK} -g -ggdb3 -D_LINUX -c minieditor.cpp
	g++ -I${VST_SDK} -g -ggdb3 -D_LINUX -c minihost.cpp

# Loading 32-bit vst plugins on a 64-bit platform:
vstminihost32: objects32
	g++ -I${VST_SDK} -m32 -g -ggdb3 -ldl -lpthread -lX11 -D_LINUX -o vstminihost32 *.o

objects32:
	g++ -I${VST_SDK} -m32 -g -ggdb3 -D_LINUX -c minieditor.cpp
	g++ -I${VST_SDK} -m32 -g -ggdb3 -D_LINUX -c minihost.cpp

clean:
	/bin/rm -vf *.o vstminihost vstminihost32