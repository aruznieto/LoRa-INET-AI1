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

#include "inet/LoRaPhy/LoRaAnalogModel.h"
#include "inet/physicallayer/contract/packetlevel/IRadioMedium.h"
#include "inet/physicallayer/analogmodel/packetlevel/ScalarAnalogModel.h"
#include "inet/physicallayer/analogmodel/packetlevel/ScalarReception.h"
#include "LoRaReception.h"
#include "LoRaTransmission.h"
#include "LoRaReceiver.h"
#include "inet/LoRa/LoRaRadio.h"

namespace inet {

namespace physicallayer {

Define_Module(LoRaAnalogModel);

std::ostream& LoRaAnalogModel::printToStream(std::ostream& stream, int level) const
{
    return stream << "LoRaAnalogModel";
}

const W LoRaAnalogModel::getBackgroundNoisePower(const LoRaBandListening *listening) const {
    //const LoRaBandListening *loRaListening = check_and_cast<const LoRaBandListening *>(listening);
    //Sensitivity values from Semtech SX1272/73 datasheet, table 10, Rev 3.1, March 2017
    W noisePower = W(math::dBm2mW(-126.5) / 1000);
    if(listening->getLoRaSF() == 6)
    {
        if(listening->getLoRaBW() == Hz(125000)) noisePower = W(math::dBm2mW(-121) / 1000);
        if(listening->getLoRaBW() == Hz(250000)) noisePower = W(math::dBm2mW(-118) / 1000);
        if(listening->getLoRaBW() == Hz(500000)) noisePower = W(math::dBm2mW(-111) / 1000);
    }

    if (listening->getLoRaSF() == 7)
    {
        if(listening->getLoRaBW() == Hz(125000)) noisePower = W(math::dBm2mW(-124) / 1000);
        if(listening->getLoRaBW() == Hz(250000)) noisePower = W(math::dBm2mW(-122) / 1000);
        if(listening->getLoRaBW() == Hz(500000)) noisePower = W(math::dBm2mW(-116) / 1000);
    }

    if(listening->getLoRaSF() == 8)
    {
        if(listening->getLoRaBW() == Hz(125000)) noisePower = W(math::dBm2mW(-127) / 1000);
        if(listening->getLoRaBW() == Hz(250000)) noisePower = W(math::dBm2mW(-125) / 1000);
        if(listening->getLoRaBW() == Hz(500000)) noisePower = W(math::dBm2mW(-119) / 1000);
    }
    if(listening->getLoRaSF() == 9)
    {
        if(listening->getLoRaBW() == Hz(125000)) noisePower = W(math::dBm2mW(-130) / 1000);
        if(listening->getLoRaBW() == Hz(250000)) noisePower = W(math::dBm2mW(-128) / 1000);
        if(listening->getLoRaBW() == Hz(500000)) noisePower = W(math::dBm2mW(-122) / 1000);
    }
    if(listening->getLoRaSF() == 10)
    {
        if(listening->getLoRaBW() == Hz(125000)) noisePower = W(math::dBm2mW(-133) / 1000);
        if(listening->getLoRaBW() == Hz(250000)) noisePower = W(math::dBm2mW(-130) / 1000);
        if(listening->getLoRaBW() == Hz(500000)) noisePower = W(math::dBm2mW(-125) / 1000);
    }
    if(listening->getLoRaSF() == 11)
    {
        if(listening->getLoRaBW() == Hz(125000)) noisePower = W(math::dBm2mW(-135) / 1000);
        if(listening->getLoRaBW() == Hz(250000)) noisePower = W(math::dBm2mW(-132) / 1000);
        if(listening->getLoRaBW() == Hz(500000)) noisePower = W(math::dBm2mW(-128) / 1000);
    }
    if(listening->getLoRaSF() == 12)
    {
        if(listening->getLoRaBW() == Hz(125000)) noisePower = W(math::dBm2mW(-137) / 1000);
        if(listening->getLoRaBW() == Hz(250000)) noisePower = W(math::dBm2mW(-135) / 1000);
        if(listening->getLoRaBW() == Hz(500000)) noisePower = W(math::dBm2mW(-129) / 1000);
    }
    return noisePower;
}

W LoRaAnalogModel::computeReceptionPower(const IRadio *receiverRadio, const ITransmission *transmission, const IArrival *arrival) const
{
    const IRadioMedium *radioMedium = receiverRadio->getMedium();
    const IRadio *transmitterRadio = transmission->getTransmitter();
    const IAntenna *receiverAntenna = receiverRadio->getAntenna();
    const IAntenna *transmitterAntenna = transmitterRadio->getAntenna();
    const INarrowbandSignal *narrowbandSignalAnalogModel = check_and_cast<const INarrowbandSignal *>(transmission->getAnalogModel());
    const IScalarSignal *scalarSignalAnalogModel = check_and_cast<const IScalarSignal *>(transmission->getAnalogModel());
    const Coord receptionStartPosition = arrival->getStartPosition();
    const Coord receptionEndPosition = arrival->getEndPosition();
    const EulerAngles transmissionDirection = computeTransmissionDirection(transmission, arrival);
    EulerAngles transmissionAntennaDirection = transmission->getStartOrientation() - transmissionDirection;
    EulerAngles receptionAntennaDirection = transmissionDirection - arrival->getStartOrientation();

    const Coord tSP = transmission->getStartPosition();
    const Coord aSP = arrival->getStartPosition();

    const Coord a = Coord(0,1,0); //Antenna Orientation

    const Coord dir = tSP - aSP;

    const double mA = sqrt(pow(a.x, 2) + pow(a.y, 2) + pow(a.z, 2));
    //const double mtSP = sqrt(pow(tSP.x, 2) + pow(tSP.y, 2) + pow(tSP.z, 2));
    //const double maSP =  sqrt(pow(aSP.x, 2) + pow(aSP.y, 2) + pow(aSP.z, 2));
    const double mdir = sqrt(pow(dir.x, 2) + pow(dir.y, 2) + pow(dir.z, 2));

    const Coord aNorm = a / mA;
    //const Coord tSPNorm = tSP / mtSP;
    //const Coord aSPNorm = aSP / maSP;
    const Coord dirNorm = dir / mdir;

    const float theta = dirNorm * aNorm;
    const float phi = -(dirNorm * aNorm);

    transmissionAntennaDirection = EulerAngles(0,acos(phi) - 1.57079632679,0);
    receptionAntennaDirection = EulerAngles(0,acos(theta),0);

    double transmitterAntennaGain = transmitterAntenna->computeGain(transmissionAntennaDirection);
    double receiverAntennaGain = receiverAntenna->computeGain(receptionAntennaDirection);

    //const EulerAngles TXsO = transmission->getStartOrientation();
    //const EulerAngles RXsO = arrival->getStartOrientation();


    std::cout << "TX COORDS: X = " << tSP.x << " | Y = " << tSP.y << " | Z = " << tSP.z << endl;
    std::cout << "RX COORDS: X = " << aSP.x << " | Y = " << aSP.y << " | Z = " << aSP.z << endl;
    //std::cout << "DIR COORDS: X = " << dir.x << " | Y = " << dir.y << " | Z = " << dir.z << endl;
    //std::cout << "THETA = " << acos(theta) << endl;
    //std::cout << "PHI = " << acos(phi) << endl;
    //std::cout << "TX COORDS NORM: X = " << tSPNorm.x << " | Y = " << tSPNorm.y << " | Z = " << tSPNorm.z << endl;
    //std::cout << "RX COORDS NORM: X = " << aSPNorm.x << " | Y = " << aSPNorm.y << " | Z = " << aSPNorm.z << endl;
    //std::cout << "DIR COORDS NORM: X = " << dirNorm.x << " | Y = " << dirNorm.y << " | Z = " << dirNorm.z << endl;
    //std::cout << "START TX OR: Alpha = " << TXsO.alpha << " | Beta = " << TXsO.beta <<  " | Gamma = " << TXsO.gamma << endl;
    //std::cout << "START RX OR: Alpha = " << RXsO.alpha << " | Beta = " << RXsO.beta <<  " | Gamma = " << RXsO.gamma << endl;
    //std::cout << "TX AD: Alpha = " << transmissionAntennaDirection.alpha << " | Beta = " << transmissionAntennaDirection.beta << endl;
    //std::cout << "RX AD: Alpha = " << receptionAntennaDirection.alpha << " | Beta = " << receptionAntennaDirection.beta << endl;

    double pathLoss = radioMedium->getPathLoss()->computePathLoss(transmission, arrival);
    double obstacleLoss = radioMedium->getObstacleLoss() ? radioMedium->getObstacleLoss()->computeObstacleLoss(narrowbandSignalAnalogModel->getCarrierFrequency(), transmission->getStartPosition(), receptionStartPosition) : 1;
    W transmissionPower = scalarSignalAnalogModel->getPower();
    return transmissionPower * std::min(1.0, transmitterAntennaGain * receiverAntennaGain * pathLoss * obstacleLoss);
}

const IReception *LoRaAnalogModel::computeReception(const IRadio *receiverRadio, const ITransmission *transmission, const IArrival *arrival) const
{
    const LoRaTransmission *loRaTransmission = check_and_cast<const LoRaTransmission *>(transmission);
    const simtime_t receptionStartTime = arrival->getStartTime();
    const simtime_t receptionEndTime = arrival->getEndTime();
    const EulerAngles receptionStartOrientation = arrival->getStartOrientation();
    const EulerAngles receptionEndOrientation = arrival->getEndOrientation();
    const Coord receptionStartPosition = arrival->getStartPosition();
    const Coord receptionEndPosition = arrival->getEndPosition();
    W receivedPower = computeReceptionPower(receiverRadio, transmission, arrival);
    Hz LoRaCF = loRaTransmission->getLoRaCF();
    int LoRaSF = loRaTransmission->getLoRaSF();
    Hz LoRaBW = loRaTransmission->getLoRaBW();
    int LoRaCR = loRaTransmission->getLoRaCR();
    return new LoRaReception(receiverRadio, transmission, receptionStartTime, receptionEndTime, receptionStartPosition, receptionEndPosition, receptionStartOrientation, receptionEndOrientation, LoRaCF, LoRaBW, receivedPower, LoRaSF, LoRaCR);
}

const INoise *LoRaAnalogModel::computeNoise(const IListening *listening, const IInterference *interference) const
{
    const LoRaBandListening *bandListening = check_and_cast<const LoRaBandListening *>(listening);
    Hz commonCarrierFrequency = bandListening->getLoRaCF();
    Hz commonBandwidth = bandListening->getLoRaBW();
    simtime_t noiseStartTime = SimTime::getMaxTime();
    simtime_t noiseEndTime = 0;
    std::map<simtime_t, W> *powerChanges = new std::map<simtime_t, W>();
    const std::vector<const IReception *> *interferingReceptions = interference->getInterferingReceptions();
    for (auto reception : *interferingReceptions) {
        const ISignalAnalogModel *signalAnalogModel = reception->getAnalogModel();
        const INarrowbandSignal *narrowbandSignalAnalogModel = check_and_cast<const INarrowbandSignal *>(signalAnalogModel);
        const LoRaReception *loRaReception = check_and_cast<const LoRaReception *>(signalAnalogModel);
        Hz signalCarrierFrequency = loRaReception->getLoRaCF();
        Hz signalBandwidth = loRaReception->getLoRaBW();
        if((commonCarrierFrequency == signalCarrierFrequency && commonBandwidth == signalBandwidth))
        {
            const IScalarSignal *scalarSignalAnalogModel = check_and_cast<const IScalarSignal *>(signalAnalogModel);
            W power = scalarSignalAnalogModel->getPower();
            simtime_t startTime = reception->getStartTime();
            simtime_t endTime = reception->getEndTime();
            if (startTime < noiseStartTime)
                noiseStartTime = startTime;
            if (endTime > noiseEndTime)
                noiseEndTime = endTime;
            std::map<simtime_t, W>::iterator itStartTime = powerChanges->find(startTime);
            if (itStartTime != powerChanges->end())
                itStartTime->second += power;
            else
                powerChanges->insert(std::pair<simtime_t, W>(startTime, power));
            std::map<simtime_t, W>::iterator itEndTime = powerChanges->find(endTime);
            if (itEndTime != powerChanges->end())
                itEndTime->second -= power;
            else
                powerChanges->insert(std::pair<simtime_t, W>(endTime, -power));
        }
        else if (areOverlappingBands(commonCarrierFrequency, commonBandwidth, narrowbandSignalAnalogModel->getCarrierFrequency(), narrowbandSignalAnalogModel->getBandwidth()))
            throw cRuntimeError("Overlapping bands are not supported");
    }

    simtime_t startTime = listening->getStartTime();
    simtime_t endTime = listening->getEndTime();
    std::map<simtime_t, W> *backgroundNoisePowerChanges = new std::map<simtime_t, W>();
    const W noisePower = getBackgroundNoisePower(bandListening);
    backgroundNoisePowerChanges->insert(std::pair<simtime_t, W>(startTime, noisePower));
    backgroundNoisePowerChanges->insert(std::pair<simtime_t, W>(endTime, -noisePower));

    for (const auto & backgroundNoisePowerChange : *backgroundNoisePowerChanges) {
        std::map<simtime_t, W>::iterator jt = powerChanges->find(backgroundNoisePowerChange.first);
        if (jt != powerChanges->end())
            jt->second += backgroundNoisePowerChange.second;
        else
            powerChanges->insert(std::pair<simtime_t, W>(backgroundNoisePowerChange.first, backgroundNoisePowerChange.second));
    }

    EV_TRACE << "Noise power begin " << endl;
    W noise = W(0);
    for (std::map<simtime_t, W>::const_iterator it = powerChanges->begin(); it != powerChanges->end(); it++) {
        noise += it->second;
        EV_TRACE << "Noise at " << it->first << " = " << noise << endl;
    }
    EV_TRACE << "Noise power end" << endl;
    return new ScalarNoise(noiseStartTime, noiseEndTime, commonCarrierFrequency, commonBandwidth, powerChanges);
}

const ISNIR *LoRaAnalogModel::computeSNIR(const IReception *reception, const INoise *noise) const
{
    return new ScalarSNIR(reception, noise);
}

} // namespace physicallayer

} // namespace inet

