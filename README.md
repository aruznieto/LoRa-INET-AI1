# Flora, INET, OPAL en el mismo proyecto
#### VERSIONES
* OMNET++ 5.6
* INET 3.6.8
* FLORA : [https://github.com/mariuszslabicki/flora](https://github.com/mariuszslabicki/flora)

#### PATHS & SYMBOLS
Recuerda cambiar los PATHS a los de tu ordenador.
* LIBRARY PATHS:
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/lib64
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/SDK/lib
	* /usr/local/cuda-10.2/lib64
* INCLUDES:
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/include
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/SDK/opal
	* /usr/local/cuda-10.2/include
* LIBRARIES:
	* /usr/lib/x86_64-linux-gnu/libboost_system.so
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/lib64/liboptix.so
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/lib64/liboptixu.so
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/SDK/lib/libopal_s.so
	* /usr/local/cuda-10.2/lib64/libcudart.so

#### MAKEMAKE OPTIONS
* COMPILE
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/include
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/SDK/opal
	* /usr/local/cuda-10.2/include 
* LINK
	* opal_s
	* boost_system 
