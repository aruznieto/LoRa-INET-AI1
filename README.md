# Flora, INET, OPAL en el mismo proyecto
## VERSIONES
* **OMNET++ 5.6** ([Descargar](https://github.com/omnetpp/omnetpp/releases/download/omnetpp-5.6/omnetpp-5.6-src-linux.tgz))
* **INET 3.6.8** ([Descargar](https://drive.google.com/file/d/1Y3piMtrX1nV4aT_69csTkULYM_kxUyQm/view?usp=sharing))
* **FLORA 0.8**  ([Descargar](https://drive.google.com/file/d/19bO7VG52wIU02MZ07ztqdHwCintY4AEb/view?usp=sharing))

## PATHS & SYMBOLS
_Recuerda cambiar los PATHS a los de tu ordenador._
* **LIBRARY PATHS:**
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/lib64
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/SDK/lib
	* /usr/local/cuda-10.2/lib64
* **INCLUDES:**
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/include
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/SDK/opal
	* /usr/local/cuda-10.2/include
* **LIBRARIES:**
	* /usr/lib/x86_64-linux-gnu/libboost_system.so
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/lib64/liboptix.so
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/lib64/liboptixu.so
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/SDK/lib/libopal_s.so
	* /usr/local/cuda-10.2/lib64/libcudart.so

## MAKEMAKE OPTIONS
* **COMPILE:**
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/include
	* /home/anrunie/optix/NVIDIA-OptiX-SDK-6.5.0-linux64/SDK/opal
	* /usr/local/cuda-10.2/include 
* **LINK:** (esto se escribe así directamente)
	* opal_s
	* boost_system 

## CAMBIOS REALIZADOS PARA LA ADAPTACIÓN
* Se copian las carpetas **LoRa* y misc** de **flora/src en inet/src**
* Se copia la carpeta **simulations** en **inet/**
* En todos los .ned de FloRa, se ha cambiado el **@class** eliminando la parte que hacía referencia a inet, por ejemplo: **@class(inet::SimpleLoRaApp) ahora es @class(SimpleLoRaApp)**
* Se han creado 2 .ned ([**OpalLoRaNode**](https://github.com/aruznieto/LoRa-INET-AI1/blob/master/inet/src/inet/LoraNode/OpalLoRaNode.ned) y [**OpalLoRaGW**](https://github.com/aruznieto/LoRa-INET-AI1/blob/master/inet/src/inet/LoraNode/OpalLoRaGW.ned), ambos como módulos compuesto) para añadir 2 parametros que necesita Opal para realizar los cálculos
* En [LoRaPhy](https://github.com/aruznieto/LoRa-INET-AI1/tree/master/inet/src/inet/LoRaPhy) se crea **OpalLoRaAnalogModel** y **OpalLoRaRadioMedium** (ambos como módulos simples)
* En [LoRaMedium.ned](https://github.com/aruznieto/LoRa-INET-AI1/blob/master/inet/src/inet/LoRaPhy/LoRaMedium.ned) se cambia el default de **analogModelType** por **OpalLoRaAnalogModel**
