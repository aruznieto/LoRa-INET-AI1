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

#include "OpalLoRaAnalogModel.h"

namespace inet::physicallayer {
    Define_Module(OpalLoRaAnalogModel);

    void OpalLoRaAnalogModel::initialize(int stage)
        {
            if (stage== INITSTAGE_LAST ) {
                analogModel = check_and_cast<OpalLoRaRadioMedium *>(getParentModule());
            }
        }
    W OpalLoRaAnalogModel::computeReceptionPower(const IRadio *receiver, const ITransmission *transmission, const IArrival *arrival) const {
        const IAntenna *receiverAntenna = receiver->getAntenna();
        const IAntenna *transmitterAntenna = transmission->getTransmitter()->getAntenna();

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

        const EulerAngles transmissionAntennaDirection = EulerAngles(0,acos(phi),0);
        const EulerAngles receptionAntennaDirection = EulerAngles(0,acos(theta),0);

        double transmitterAntennaGain = transmitterAntenna->computeGain(transmissionAntennaDirection);
        double receiverAntennaGain = receiverAntenna->computeGain(receptionAntennaDirection);

        W power = analogModel->opalComputeReception(receiver, transmission);

        return power * std::min(1.0, transmitterAntennaGain * receiverAntennaGain);
    }
}
