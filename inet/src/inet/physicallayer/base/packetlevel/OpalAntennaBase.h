//
// Copyright (C) 2013 OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_OPALANTENNABASE_H
#define __INET_OPALANTENNABASE_H

#include "AntennaBase.h"

namespace inet {

namespace physicallayer {

class INET_API OpalAntennaBase : public AntennaBase
{
  protected:

  protected:
    virtual void initialize(int stage);

  public:

    std::vector<std::vector<double>> gainsAntenna;

};

} // namespace physicallayer

} // namespace inet

#endif // ifndef __INET_ANTENNABASE_H

