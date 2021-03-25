# Integración de OPAL en FloRa
## VERSIONES
* **OMNET++ 5.6** ([Descargar](https://github.com/omnetpp/omnetpp/releases/download/omnetpp-5.6/omnetpp-5.6-src-linux.tgz))
* **INET 3.6.8** ([Descargar](https://drive.google.com/file/d/1Y3piMtrX1nV4aT_69csTkULYM_kxUyQm/view?usp=sharing))
* **FLORA 0.8**  ([Descargar](https://drive.google.com/file/d/19bO7VG52wIU02MZ07ztqdHwCintY4AEb/view?usp=sharing))
* **OPAL** ([Descargar](https://gitlab.com/esteban.egea/opal))

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
* Se copian las carpetas LoRa* y misc de **flora/src en inet/src**
* Se copia la carpeta **simulations** en **inet/**
* En todos los .ned de FloRa, se ha cambiado el **@class** eliminando la parte que hacía referencia a inet, por ejemplo: **@class(inet::SimpleLoRaApp) ahora es @class(SimpleLoRaApp)**
* Se han creado 2 .ned ([**OpalLoRaNode**](https://github.com/aruznieto/LoRa-INET-AI1/blob/master/inet/src/inet/LoraNode/OpalLoRaNode.ned) y [**OpalLoRaGW**](https://github.com/aruznieto/LoRa-INET-AI1/blob/master/inet/src/inet/LoraNode/OpalLoRaGW.ned), ambos como módulos compuesto) para añadir 2 parametros que necesita Opal para realizar los cálculos
* En el [package.ned](https://github.com/aruznieto/LoRa-INET-AI1/blob/master/inet/simulations/package.ned) que lanza la simulación de LoRa se tiene que cambiar LoRaNode y LoRaGW por los módulos que se han creado en el punto anterior.
* En [LoRaPhy](https://github.com/aruznieto/LoRa-INET-AI1/tree/master/inet/src/inet/LoRaPhy) se crea **OpalLoRaAnalogModel** y **OpalLoRaRadioMedium** (ambos como módulos simples), estos contendrán todas las funciones necesarias para realizar los calculos que realiza FloRa con OPAL
* En [LoRaMedium.ned](https://github.com/aruznieto/LoRa-INET-AI1/blob/master/inet/src/inet/LoRaPhy/LoRaMedium.ned) se cambia el default de **analogModelType** por **OpalLoRaAnalogModel**

## CÁLCULOS Y PRUEBAS PARA LA COMPROBACIÓN DEL FUNCIONAMIENTO

Cálculos usando la Fórmula de Friis para comprobar la potencia recibida con un rayo

![IMAGEN](https://i.imgur.com/lwvqTEi.png)

Comprobación TwoRayInterference con Meshes en escenario Chicago. Nodo y GW se han colocado en el hueco de un edificio, para favorecer los rebotes

![EDIFICIO](https://i.imgur.com/zXfSxRd.png)

![CHICAGO+MESHES](https://i.imgur.com/fof3s9O.png)

Comprobación FreeSpaceLoss sin Meshes

![SINMESHES](https://i.imgur.com/G8so0zV.png)

Se puede observar que la potencia recibida en el primer caso es mayor

## ACELERACIÓN PARA RAY-TRACING ESTÁTICO

Se modifica OpalLoRaRadioMedium:
* .ned:
```
bool isStatic = default(false);
```
* .h:
```
 bool isStatic;

typedef std::map<const IRadio*,float> CachedPowerEntry;
typedef std::map<const IRadio*, CachedPowerEntry*> CachedPowerMap;

CachedPowerMap mapReceptions;
```
* .cc:
** stage == INITSTAGE_LOCAL
```
isStatic = par("isStatic");
```
** transmitPacket
```
        if(isStatic){
            auto tx = mapReceptions.find(transmission->getTransmitter());
                if (tx!=mapReceptions.end()) {
                    //std::cout << "transmitPacket() -> Transmitter " << receiversRadios.at(tx->first)->opalReceiverId << " already exist on the map. Searching CPE"<<endl;
                    CachedPowerEntry* CPE = tx->second;
                    int txOpalid = receiversRadios.at(tx->first)->opalReceiverId;
                    for (auto rx = CPE->begin(); rx != CPE->end(); ++rx){
                        if(rx->second != 0){
                            OpalReceiverCallback* callback;
                            try  {
                                callback=receiversRadios.at(rx->first);
                                callback->createReception(rx->second, txOpalid);
                                //std::cout<<"transmitPacket() -> Reception created between "<< txOpalid << " and " << receiversRadios.at(rx->first)->opalReceiverId << " . Power: "<< rx->second <<endl;

                            } catch (std::out_of_range &e) {
                                std::stringstream s;
                                s<<"sendToRadio():: radio "<<radio<<" is not registered with opal";
                                throw cRuntimeError(s.str().c_str());
                            }
                        }
                    }
                }
                else {
                    //std::cout<<"transmitPacket() -> Transmitter " << transmission->getTransmitter()->getId() << " doesnt exist on CachedPowerMap "<< " . Executing transmitInOpal"<<endl;;
                    CachedPowerEntry* CPE = new CachedPowerEntry();
                    mapReceptions.insert(std::pair<const IRadio*, CachedPowerEntry*>(transmission->getTransmitter(), CPE));
                    transmitInOpal(radio,transmission);
                }
        }

        else{
            transmitInOpal(radio,transmission);
        }
```
** opalComputeReception
```
        if(isStatic){
            auto tx = mapReceptions.find(transmission->getTransmitter());
                if (tx != mapReceptions.end()) {
                    // Transmitter exists inside map
                    //ocb=receiversRadios.at(transmission->getTransmitter());
                    //int idtx = ocb->opalReceiverId;
                    //std::cout<<"opalComputeReception() -> Transmitter " << idtx << " already exist on the map. Searching CPE"<<endl;
                    CachedPowerEntry* CPE = tx->second;
                    auto rx = CPE->find(receiver);
                    if (rx == CPE->end()) {
                        //ocb=receiversRadios.at(receiver);
                        //int idrx = ocb->opalReceiverId;
                        //std::cout<<"opalComputeReception() -> Power between "<< idrx << " and " << idtx << " doesnt exist. Inserting power: "<< (float)receptionPower.get() <<endl;
                        CPE->insert(std::pair<const IRadio*,float>(receiver, (float)receptionPower.get()));
                    }
                    else {
                        //ocb=receiversRadios.at(receiver);
                        //int idrx = ocb->opalReceiverId;
                        //std::cout<<"opalComputeReception() -> Power between "<< idrx << " and " << idtx << " exists. Power: "<< (float)rx->second <<endl;
                    }
                }
        }
```
