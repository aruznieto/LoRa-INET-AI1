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

#ifndef LORAPHY_HATAOKUMURA_H_
#define LORAPHY_HATAOKUMURA_H_

#include "inet/physicallayer/pathloss/FreeSpacePathLoss.h"

namespace inet {

namespace physicallayer {

/**
 * This class implements the LoRaHataOkumura.
 */
class INET_API HataOkumura : public FreeSpacePathLoss
{
  protected:
    double K1;
    double K2;
    bool small;
    int suburban;
    int GW;
    int Node;

  protected:
    virtual void initialize(int stage) override;

  public:
    HataOkumura();
    virtual std::ostream& printToStream(std::ostream& stream, int level) const override;
    virtual double computePathLoss(mps propagationSpeed, Hz frequency, m distance) const override;
};

} // namespace physicallayer

} // namespace inet

#endif /* LORAPHY_LORAHATAOKUMURA_H_ */
