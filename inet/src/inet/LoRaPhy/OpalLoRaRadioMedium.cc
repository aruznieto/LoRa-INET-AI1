//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "OpalLoRaRadioMedium.h"

#include "inet/common/ModuleAccess.h"
#include "inet/mobility/base/MobilityBase.h"
#include "inet/physicallayer/communicationcache/VectorCommunicationCache.h"
#include <functional>
namespace inet::physicallayer {

    Define_Module(OpalLoRaRadioMedium);



    OpalReceiverCallback::OpalReceiverCallback(OpalLoRaRadioMedium* medium, int id, const IRadio* radio) {
        this->myMedium=medium;
        this->opalReceiverId=id;
        this->radio=radio;
        this->lastPower=0.0f;
        this->lastTransmitterId=-1;
        this->hasReceivedPower=false;
        this->transmission=nullptr;
        //Set the callback to be called among the suitable members of this class, or add your own custom callback
        this->myCallback=std::bind(&inet::physicallayer::OpalReceiverCallback::createReception, this, std::placeholders::_1,std::placeholders::_2);
    }
    void OpalReceiverCallback::operator()(float p, int id)  {
        //  std::cout<<"OpalReceiverCallback["<< opalReceiverId<<"].p="<<p<<"transmitter id="<<id<<std::endl;
        this->lastPower=p;
        this->lastTransmitterId=id;
        this->hasReceivedPower=true;
        //std::cout<<"OpalReceiverCallback["<< opalReceiverId<<"].p="<<this->lastPower<<"transmissionId="<<this->lastTransmissionId<<"hasReceivedPower="<<this->hasReceivedPower<<std::endl;
        //printf("Address of this is %p\n", (void *)this);
    }
    void OpalReceiverCallback::getPower(float p, int id)  {

        this->lastPower=p;
        this->lastTransmitterId=id;
        this->hasReceivedPower=true;

        //printf("Address of this is %p\n", (void *)this);
    }
    void OpalReceiverCallback::createReception(float p, int id)  {
        //With this callback we create a new Reception when power is received
        std::cout<<"OpalReceiverCallback["<< opalReceiverId<<"].p="<<p<<" from transmitter  with id="<<id<<std::endl;
        this->lastPower=p;
        this->lastTransmitterId=id;
        this->hasReceivedPower=true;
        /* if (this->transmission!=nullptr) {

            std::cout<<"t="<<simTime()<<":Opal::Reception created for  OpalReceiverCallback["<< opalReceiverId<<"], power="<<this->lastPower<<", transmitter="<<this->lastTransmitterId<<",  hasReceivedPower="<<this->hasReceivedPower<<std::endl;
        }
         */
        myMedium->getReception(this->radio,this->transmission);


        // printf("Address of this is %p\n", (void *)this);
    }
    void  OpalReceiverCallback::reset() {
        this->lastPower=0.0f;
        this->lastTransmitterId=-1;
        this->hasReceivedPower=false;
    }



    simsignal_t OpalLoRaRadioMedium::mobilityStateChangedSignal = cComponent::registerSignal("mobilityStateChanged");

    OpalLoRaRadioMedium::OpalLoRaRadioMedium() : LoRaMedium(), OpalSceneManager()
    {
        std::cout<<"OpalLoRaRadioMedium() with global called"<<std::endl;
    }

    void OpalLoRaRadioMedium::initialize(int stage)
    {
        LoRaMedium::initialize(stage);
        if (stage == INITSTAGE_LOCAL) {
            //First, set the environment to read the CUDA program files from our custom directory.
            std::string pf(cudaDir);
            if (!pf.empty()) {
#ifdef _WIN32
                _putenv_s("OPTIX_SAMPLES_SDK_DIR", cudaDir);

#else
                setenv("OPTIX_SAMPLES_SDK_DIR", cudaDir, 1);
#endif // _WIN32
            }


            //Initialize opal
            carrierFrequency = par("carrierFrequency");
            double r= par("receptionRadius");
            maxNumberOfReflections = par("maxNumberOfReflections");
            double me=par("minEpsilon");
            receptionRadius= (float)r;
            minEpsilon=(float)me;
            setMaxReflections(static_cast<unsigned int>(maxNumberOfReflections));
            initContext((float)carrierFrequency, true); //With exact ligthspeed
            std::cout<<"Opal initContext called"<<endl;
            getParentModule()->subscribe(mobilityStateChangedSignal, this);

        }
        if (stage==INITSTAGE_PHYSICAL_ENVIRONMENT) {
            //Load meshes from server (from Unity), any other framework or load your own files

            //Build static meshes
            loadFromFiles=par("loadMeshesFromFile");
            if (loadFromFiles) {
                loadMeshesFromFiles();
                EV_DEBUG<<"Loaded static meshes into scene"<<endl;
            }
            //else static meshes will be loaded by Veneris server or any other framework
        }
        if (stage== INITSTAGE_LAST ) {
            //if (loadFromFiles) {
                //if we have read the static meshes from files we can end it now, otherwise, let the server or framework call it when available.
                finishOpalContext();
            //}
            //Print configuration

            //std::cout<<"RadioMedium configuration:"<<getCompleteStringRepresentation()<<std::endl;
            //std::cout<< check_and_cast<inet::physicallayer::IPrintableObject*>(getModuleByPath(".backgroundNoise"))->getCompleteStringRepresentation()<<std::endl;
            //std::cout<< check_and_cast<inet::physicallayer::IPrintableObject*>(getModuleByPath(".mediumLimitCache"))->getCompleteStringRepresentation()<<std::endl;
        }
    }

    OpalLoRaRadioMedium::~OpalLoRaRadioMedium()
    {
        for (auto r : receiversRadios) {
            delete r.second;
        }

    }

    void OpalLoRaRadioMedium::loadMeshesFromFiles() {
        //Our own simple mesh format
        std::string path=par("meshesPath");
        std::string meshesFileList = par("meshesFileList");
        std::string meshesNames;
        std::stringstream iss;
        std::vector<std::string> results;
        if (meshesFileList.empty()) {
            meshesNames = par("meshes").stdstringValue();
            iss.str(meshesNames);
            results.assign(std::istream_iterator<std::string>{iss},
                    std::istream_iterator<std::string>());
        } else {
            //Read meshes names from file with list
            std::ifstream infile(meshesFileList);

            std::string line;


            while (std::getline(infile, line)) {
                results.push_back(line);
            }
            infile.close();
        }
        //std::string meshesNames = par("meshes");
        EV_DEBUG<<"path="<<path<<"meshesNames="<<meshesNames<<endl;
        //std::istringstream iss(meshesNames);
        //std::vector<std::string> results(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());

        for (auto n : results) {
            std::ostringstream vf;
            std::ostringstream inf;
            std::ostringstream tf;
            std::ostringstream emf;


            EV_DEBUG<<"Loading mesh from "<<n<<endl;
            //Assume all files use the same suffixes
            vf<<path<<"/"<<n<<"-v.txt";
            inf<<path<<"/"<<n<<"-i.txt";
            tf<<path<<"/"<<n<<"-t.txt";
            emf<<path<<"/"<<n<<"-em.txt";
            std::vector<optix::float3> v=loadVerticesFromFile(vf.str().c_str());
            std::vector<int> ind=loadTrianglesFromFile(inf.str().c_str());
            optix::Matrix4x4 tm=loadTransformFromFile(tf.str().c_str());

            //emProp1.dielectricConstant = optix::make_float2(3.75f, -60.0f*defaultChannel.waveLength*0.038f);
            MaterialEMProperties emProp1 =loadEMFromFile(emf.str().c_str());
            addStaticMesh(static_cast<int>(v.size()), v.data(), static_cast<int>(ind.size()), ind.data(), tm, emProp1);

        }

    }
    MaterialEMProperties OpalLoRaRadioMedium::loadEMFromFile(const char* file) {
        std::ifstream infile(file);

        float a,b,c,d=0;
        //cha
        infile>>a;
        infile>>b;
        infile>>c;
        infile>>d;
        return ITUparametersToMaterial(a,b,c,d);



    }
    optix::Matrix4x4  OpalLoRaRadioMedium::loadTransformFromFile(const char* file) {
        std::ifstream infile(file);
        float x, y, z, w=0;
        //char c;
        optix::Matrix4x4 tm;
        std::string line;
        int row=0;

        while (std::getline(infile, line)) {
            if (row==4) {
                break;
            }
            //std::cout << line << std::endl;
            std::string delimiters = "\t";
            size_t current;
            size_t next = -1;
            int p = 0;
            do
            {
                current = next + 1;
                next = line.find_first_of(delimiters, current);
                if (p == 0) {
                    x = std::stof(line.substr(current, next - current));
                }
                if (p == 1) {
                    y = std::stof(line.substr(current, next - current));
                }
                if (p == 2) {
                    z = std::stof(line.substr(current, next - current));
                }
                if (p == 3) {
                    w = std::stof(line.substr(current, next - current));
                }

                //std::cout << line.substr(current, next - current) <<"\t"<< std::endl;
                p++;
            } while (next != std::string::npos);

            tm.setRow(row, optix::make_float4(x, y, z,w));
            row++;
        }
        std::cout << "Loaded matrix" << tm << "  from " << file << std::endl;
        infile.close();
        return tm;

    }
    std::vector<optix::float3>  OpalLoRaRadioMedium::loadVerticesFromFile(const char* file) {
        std::ifstream infile(file);
        float x, y, z;
        //char c;
        std::vector<optix::float3> vertices;
        std::string line;


        while (std::getline(infile, line)) {

            //std::cout << line << std::endl;
            std::string delimiters = "\t";
            size_t current;
            size_t next = -1;
            int p = 0;
            do
            {
                current = next + 1;
                next = line.find_first_of(delimiters, current);
                if (p == 0) {
                    x = std::stof(line.substr(current, next - current));
                }
                if (p == 1) {
                    y = std::stof(line.substr(current, next - current));
                }
                if (p == 2) {
                    z = std::stof(line.substr(current, next - current));
                }

                //std::cout << line.substr(current, next - current) <<"\t"<< std::endl;
                p++;
            } while (next != std::string::npos);

            vertices.push_back(optix::make_float3(x, y, z));
        }
        std::cout << "Loaded " << vertices.size() << " vertices from " << file << std::endl;
        infile.close();

        return vertices;
    }




    std::vector<int>  OpalLoRaRadioMedium::loadTrianglesFromFile(const char* file) {
        std::ifstream infile(file);
        int i;
        std::vector<int> triangles;

        while (infile>>i) {
            //std::cout << i << std::endl;
            triangles.push_back(i);
        }
        std::cout << "Loaded " << triangles.size() << "indices from " << file << std::endl;
        infile.close();
        return triangles;
    }


    optix::float3  OpalLoRaRadioMedium::getOpalCoordinates(Coord c) const {
        //WARNING:Unity uses a left-handed coordinate system with Y axis as up, whereas INET uses Z axis as up
        //if meshes have been exported from Unity, or coordinates are provided by Unity, we have to interchange axis, to keep everything consistent
        //Transformation matrices coming from Unity also have to be kept consistent
        //Otherwise, we can just keep the INET coordinates

        //At the moment, since I am using Unity meshes, I switch axis
        return optix::make_float3((float) c.x,(float) c.z, (float) c.y);
    }

    void OpalLoRaRadioMedium::addRadio(const IRadio *radio)
    {

        LoRaMedium::addRadio(radio);
        IMobility* mob= radio->getAntenna()->getMobility();
        if (mob) {
            Coord c=mob->getCurrentPosition();
            cModule *radioModule = const_cast<cModule *>(check_and_cast<const cModule *>(radio));
            int id =getContainingNode(radioModule)->par("id");
            double  rxRadius =getContainingNode(radioModule)->par("receptionRadius");
            OpalReceiverCallback* rc = new OpalReceiverCallback(this,id,radio);

            std::cout<<"Adding receiver "<<id<<" with Coord="<<c<<endl;
            std::cout<<"Receiver["<<id<<"]->radioId= "<<radio->getId()<<endl;
            receiversRadios.insert(std::pair<const IRadio*,OpalReceiverCallback*>(radio,rc));

            //Radios are both transmitter and receivers

            optix::float3 posrx=optix::make_float3((float) c.x,(float) c.y, (float) c.z);
            //TODO: set polarization according to antenna orientation

            addReceiver(id,posrx,optix::make_float3(0.0f,1.0f,0.0f),rxRadius,(rc->myCallback));


        } else {
            throw cRuntimeError("Radio does not have mobility coordinates");
        }

    }



    void OpalLoRaRadioMedium::removeRadio(const IRadio *radio) {



        LoRaMedium::removeRadio(radio);
        OpalReceiverCallback* rc;
        try {
            rc=receiversRadios.at(radio);
            //Check if there is an ongoing transmission for this radio
            std::cout<<simTime()<<":OpalLoRaRadioMedium::removeRadio. Removing "<<rc->opalReceiverId<<"(radioId="<<radio->getId()<<"). transmissions.size="<<transmissions.size()<<endl;
            int i=0;
            for (auto t : transmissions){
                // std::cout<<i<<":. Transmission stored for "<<t->getTransmitterId()<<" with id="<<t->getId()<<endl;

                if (t->getTransmitter()->getId()==radio->getId()){
                    //store a pending transmission
                    std::cout<<"Radio "<<rc->opalReceiverId<<"has ongoing transmissions."<<t->getId()<<endl;
                    pendingTransmissions.insert(std::pair<int,int>(t->getId(), rc->opalReceiverId));
                    break;
                }
                ++i;
            }
        } catch (std::out_of_range &e) {
            throw cRuntimeError("Radio is not registered with opal");
            return;
        }

        removeReceiver(rc->opalReceiverId);
        receiversRadios.erase(radio);
        //Delete the callback
        delete rc;

    }


    IRadioFrame *OpalLoRaRadioMedium::transmitPacket(const IRadio *radio, cPacket *packet) {
        auto radioFrame = createTransmitterRadioFrame(radio, packet);
        auto transmission = radioFrame->getTransmission();
        addTransmission(radio, transmission);
        if (recordCommunicationLog)
            communicationLog.writeTransmission(radio, radioFrame);

        //std::cout<<"OpalLoRaRadioMedium::transmitPacket(): transmission="<<transmission<<endl;
        sendToAffectedRadios(const_cast<IRadio *>(radio), radioFrame);
        communicationCache->setCachedFrame(transmission, radioFrame);


        transmitInOpal(radio,transmission);



        return radioFrame;
    }
    void OpalLoRaRadioMedium::sendToAffectedRadios(IRadio *radio, const IRadioFrame *frame)
    {
        //const Signal *signal = check_and_cast<const Signal *>(transmittedSignal);
        //std::cout << " OpalLoRaRadioMedium::sendToAffectedRadios(): Sending " << transmittedSignal << " with " << signal->getBitLength() << " bits in " << signal->getDuration() * 1E+6 << " us transmission duration"
        //<< " from " << radio << " on " << (IRadioMedium *)this << ". receivers.size=" <<receiversRadios.size()<< endl;
        //Send to all radios: opal will callback the ones receiving power
        //for (auto r : receiversRadios) {

         //   r.second->reset();
         //   sendToRadio(radio,r.first,transmittedSignal);

        //}
        const RadioFrame *radioFrame = check_and_cast<const RadioFrame *>(frame);
            EV_DEBUG << "Sending " << frame << " with " << radioFrame->getBitLength() << " bits in " << radioFrame->getDuration() * 1E+6 << " us transmission duration"
                     << " from " << radio << " on " << (IRadioMedium *)this << "." << endl;
            if (neighborCache && rangeFilter != RANGE_FILTER_ANYWHERE)
            {
                double range;
                if (rangeFilter == RANGE_FILTER_COMMUNICATION_RANGE)
                    range = mediumLimitCache->getMaxCommunicationRange(radio).get();
                else if (rangeFilter == RANGE_FILTER_INTERFERENCE_RANGE)
                    range = mediumLimitCache->getMaxInterferenceRange(radio).get();
                else
                    throw cRuntimeError("Unknown range filter %d", rangeFilter);
                if (std::isnan(range))
                {
                    EV_WARN << "We can't use the NeighborCache for radio " << radio->getId() << ": range is NaN" << endl;
                    sendToAllRadios(radio, frame);
                }
                else
                    neighborCache->sendToNeighbors(radio, frame, range);
            }
            else
                sendToAllRadios(radio, frame);

    }


    void OpalLoRaRadioMedium::transmitInOpal(const IRadio *radio, const ITransmission *transmission) {



        int id;
        try  {
            id=receiversRadios.at(radio)->opalReceiverId;
        } catch (std::out_of_range &e) {
            std::stringstream s;
            s<<"transmitInOpal():: radio "<<radio<<" is not registered with opal";
            throw cRuntimeError(s.str().c_str());
            return;
        }
        //Transmitter position
        Coord txpos=transmission->getStartPosition();
        //Transmitter power
        const IScalarSignal *scalarSignalAnalogModel = check_and_cast<const IScalarSignal *>(transmission->getAnalogModel());
        W transmissionPower = scalarSignalAnalogModel->getPower();
        optix::float3 postx=optix::make_float3((float) txpos.x,(float) txpos.y, (float) txpos.z);

        //TODO: set polarization according to antenna orientation
        // Coord pol= radio->getAntenna()->getMobility()->getCurrentPosition(); //Have to be changed to a unit vector indicating the antenna orientation
        //Perpendicular to the floor (Y axis up)
        optix::float3 polarization=optix::make_float3(0.0f,1.0f,0.0f);

        //std::cout<<simTime()<<":Opal radio id="<<id<<" transmitting. txid="<<transmission->getTransmitterId()<<"txPower="<<transmissionPower.get()<<endl;
        //std::cout<<simTime()<<":"<<transmission <<endl;

        transmit(id, (float) transmissionPower.get(),postx,polarization);


    }

    void OpalLoRaRadioMedium::sendToRadio(IRadio *trasmitter, const IRadio *receiver, const IRadioFrame *frame) {
        const Radio *transmitterRadio = check_and_cast<const Radio *>(trasmitter);
        const Radio *receiverRadio = check_and_cast<const Radio *>(receiver);
        const ITransmission *transmission = frame->getTransmission();
        if (receiverRadio != transmitterRadio && isPotentialReceiver(receiverRadio, transmission)) {
            const IArrival *arrival = getArrival(receiverRadio, transmission);
            simtime_t propagationTime = arrival->getStartPropagationTime();

            //  std::cout << " OpalLoRaRadioMedium::sendToRadio(): Sending " << transmittedSignal
            //<< " from " << (IRadio *)transmitterRadio << " at " << transmission->getStartPosition()
            // << " to " << (IRadio *)receiverRadio << " at " << arrival->getStartPosition()
            //<< " in " << propagationTime * 1E+6 << " us propagation time. arrivalTime at "<<arrival->getStartTime() << endl;
            auto radioFrame = static_cast<RadioFrame *>(createReceiverRadioFrame(transmission));


            //Set this transmission in callback
            // std::cout<<"OpalLoRaRadioMedium::sendToRadio(): transmission"<<transmission<<endl;
            OpalReceiverCallback* callback;
            try  {
                callback=receiversRadios.at(receiver);

            } catch (std::out_of_range &e) {
                std::stringstream s;
                s<<"sendToRadio():: radio "<<receiver<<" is not registered with opal";
                throw cRuntimeError(s.str().c_str());
                return;
            }


            callback->transmission=transmission;


            cGate *gate = receiverRadio->getRadioGate()->getPathStartGate();
            ASSERT(dynamic_cast<IRadio *>(getSimulation()->getContextModule()) != nullptr);
            const_cast<Radio *>(transmitterRadio)->sendDirect(radioFrame, propagationTime, transmission->getDuration(), gate);
            communicationCache->setCachedFrame(receiverRadio, transmission, radioFrame);
            radioFrameSendCount++;
        }

    }

    //Executed in the callbacks when power is received
    //Only receivers with actual power received should have been called here
    const W OpalLoRaRadioMedium::opalComputeReception(const IRadio *receiver, const ITransmission *transmission) const {

        //Get power from callback
        OpalReceiverCallback* ocb=nullptr;
        int txId;
        try  {
            ocb=receiversRadios.at(receiver);

        } catch (std::out_of_range &e) {
            std::stringstream s;
            s<<"computeReception():: radio "<<receiver<<" is not registered with opal";
            throw cRuntimeError(s.str().c_str());
            //return nullptr;
        }

        if (transmission->getTransmitter()==nullptr) {
            //Transmitter may have been removed
            std::stringstream s;
            ocb=receiversRadios.at(receiver);
            std::cout<<"computeReception():: transmitter is  null transmitterId="<<transmission->getTransmitter()->getId()<<"transmissionId"<<transmission->getId()<<endl;
            s<<"computeReception():: transmitter is  null. Receiver="<<ocb->opalReceiverId;
            try{
                txId=pendingTransmissions.at(transmission->getId());
            } catch (std::out_of_range &e) {
                std::stringstream s;
                s<<"computeReception():: pending transmission "<<transmission->getId()<<" not found";
                throw cRuntimeError(s.str().c_str());
            }

            //return nullptr;
        } else{

            try  {
                txId=receiversRadios.at(transmission->getTransmitter())->opalReceiverId;

            } catch (std::out_of_range &e) {
                std::stringstream s;
                if (transmission->getTransmitter()==nullptr) {
                    std::cout<<"computeReception():: out of range transmitter is  null";
                    s<<"computeReception():: receiver "<<ocb->opalReceiverId<<". transmitter is not registered with opal";
                } else {
                    s<<"computeReception():: receiver "<<ocb->opalReceiverId<<". transmitter "<<transmission->getTransmitter()<<" is not registered with opal";
                }
                throw cRuntimeError(s.str().c_str());
                // return nullptr;
            }
        }

        double p=0.0;
        if (ocb->hasReceivedPower) {
            if (ocb->lastTransmitterId==txId) {
                p=(double)ocb->lastPower;
                //std::cout<<"OpalLoRaRadioMedium::computeReception(): Power received="<< p << endl;
                ocb->reset();
            }else {
                throw cRuntimeError("%f: R[%d]: Received power from unknown transmitter %d, expected transmitter %d",simTime().dbl(),ocb->opalReceiverId, ocb->lastTransmitterId,txId);
            }
            if (ocb->opalReceiverId==txId) {
                throw cRuntimeError("%f: R[%d]: Received power from my own transmitter %d",simTime().dbl(),ocb->opalReceiverId,txId);

            }
        }
        W receptionPower(p);
        return receptionPower;
        //return new ScalarReception(receiver, transmission, receptionStartTime, receptionEndTime, receptionStartPosition, receptionEndPosition, receptionStartOrientation, receptionEndOrientation, narrowbandSignalAnalogModel->getCarrierFrequency(), narrowbandSignalAnalogModel->getBandwidth(), receptionPower);

    }

    void OpalLoRaRadioMedium::finishOpalContext()
    {

        elevationDelta=par("elevationDelta");
        azimuthDelta=par("azimuthDelta");
        if (elevationDelta<=0 || azimuthDelta <=0) {
            throw cRuntimeError("elevationDelta and azimuthDelta must be strictly positive");
        }
        bool useDecimalDegreeDelta=par("useDecimalDegreeDelta");
        if (useDecimalDegreeDelta) {
            if (elevationDelta>10 || azimuthDelta >10) {
                throw cRuntimeError("with decimal degrees elevationDelta and azimuthDelta must be between 1 and 10");

            }
            createRaySphere2DSubstep(elevationDelta, azimuthDelta);
        } else {
            createRaySphere2D(elevationDelta, azimuthDelta);
        }
        finishSceneContext();
        //setPrintEnabled(1024*1024*1024);



        WATCH_MAP(receiversRadios);
        EV_INFO << "Initialized " << printSceneReport() << endl;
        std::cout<<"Opal Initialized"<<endl;
    }

    void OpalLoRaRadioMedium::receiveSignal(cComponent* source, simsignal_t signal, cObject* value, cObject* details) {
        if (signal==mobilityStateChangedSignal) {
            MobilityBase* mob=check_and_cast<MobilityBase*>(value);
            //int id=mob->getParentModule()->par("id").intValue();
            int id=mob->getParentModule()->getId();
            for (std::map<const IRadio*, OpalReceiverCallback*>::iterator it=receiversRadios.begin(); it!=receiversRadios.end(); ++it) {
                if (it->second->opalReceiverId==id) {
                    Coord p=mob->getCurrentPosition();
                    optix::float3 pos=optix::make_float3((float)p.x,(float)p.y,(float)p.z);
                    //std::cout<<"OpalLoRaRadioMedium::receiveSignal() Updating position of "<<id<<"to "<<p<<pos<<endl;

                    updateReceiver(id,pos);


                    break;
                }
            }

            //Get the id


        } else {
            OpalLoRaRadioMedium::receiveSignal(source,signal,value,details);
        }

    }


}
//namespace
