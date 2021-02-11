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

#ifndef __INET4_OPALRADIOMEDIUM_H_
#define __INET4_OPALRADIOMEDIUM_H_

//#include <omnetpp.h>
#include "inet/physicallayer/common/packetlevel/RadioMedium.h"
#include "LoRaMedium.h"
#include "inet/physicallayer/analogmodel/packetlevel/ScalarReception.h"
#include "inet/physicallayer/contract/packetlevel/IPrintableObject.h"
#include "Opal.h"
#include <unordered_set>
using namespace omnetpp;
using namespace opal;
namespace inet::physicallayer {

    /**
     * TODO - Generated class
     */
    class OpalLoRaRadioMedium; //forward declaration

    //Class that will be notified by Opal when some transmission is received
    class OpalReceiverCallback : public IPrintableObject {
    public:
        OpalReceiverCallback(OpalLoRaRadioMedium* medium ,int id, const IRadio* radio);

        OpalLoRaRadioMedium* myMedium;
        bool hasReceivedPower;
        int opalReceiverId;
        float lastPower;
        int lastTransmitterId;
        const ITransmission *transmission;
        //Callback methods
        void operator()(float p, int id);
        void getPower(float p, int id);
        void createReception(float p, int id);
        void reset();
        const IRadio* radio;
        virtual std::ostream& printToStream(std::ostream& stream, int level) const {
            stream << "opalReceiverId=" << opalReceiverId << ",hasReceivedPower=" << hasReceivedPower<<",lastPower="<<lastPower<<",lastTransmissionId="<<lastTransmitterId; // no endl!
            return stream;
        }
        std::function<void(float,int)> myCallback;


    };

    //Uses OpalSceneManager because we only consider single transmitter (and multiple receivers). Computation of multi transmitter in
    //parallel requires further changes and it is complex in the INET context (if possible at all) since it requires delayed
    //computation of received power. But then one cannot know if the radio should change to reception mode...

    class INET_API OpalLoRaRadioMedium : public LoRaMedium, public OpalSceneManager
    {



    public:
        //Subscribe to position changes for the receivers and transmitters
        static simsignal_t mobilityStateChangedSignal;


        //Parameters to be passed to Opal
        double  carrierFrequency;
        float receptionRadius;
        int azimuthDelta;
        int elevationDelta;
        int maxNumberOfReflections;
        bool loadFromFiles;

        std::map<const IRadio*, OpalReceiverCallback*> receiversRadios;

        //Pending transmissions
        std::map<int, int> pendingTransmissions; //transmissionId, opalId;

        //Location of Optix Programs: change to your source directory (can be read as a parameter)
        //Leave  empty to use default directory: optix SDK opal sample directory
        const char* cudaDir = "";

        virtual void initialize(int stage);
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }



        void transmitInOpal(const IRadio *radio, const ITransmission *transmission);


        //Utility methods to load simple scenarios
        void loadMeshesFromFiles();
        std::vector<optix::float3> loadVerticesFromFile(const char* file);
        std::vector<int>  loadTrianglesFromFile(const char* file);
        optix::Matrix4x4 loadTransformFromFile(const char* file);
        MaterialEMProperties loadEMFromFile(const char* file);

        //Transmission methods
        virtual void sendToAffectedRadios(IRadio *transmitter, const IRadioFrame *frame);

    public:
        OpalLoRaRadioMedium();
        virtual ~OpalLoRaRadioMedium();

        virtual const W opalComputeReception(const IRadio *receiver, const ITransmission *transmission) const;

        //Translate Opal to OMNET coordinates
        optix::float3 getOpalCoordinates(Coord c) const;
        //Add  a new transceiver
        virtual void addRadio(const IRadio *radio) override;
        //Remove a transceiver
        virtual void removeRadio(const IRadio *radio) override;
        //Transmit packet with Opal
        virtual IRadioFrame *transmitPacket(const IRadio *transmitter, cPacket *macFrame);
        //Send to all radios
        virtual void sendToRadio(IRadio *trasmitter, const IRadio *receiver, const IRadioFrame *frame);
        //Finish context and build scene
        void finishOpalContext();
        //Receive mobility signals to change receivers positions in Opal
        virtual void receiveSignal(cComponent *source, simsignal_t signal, cObject *value, cObject *details) override;

    };

} //namespace

#endif
