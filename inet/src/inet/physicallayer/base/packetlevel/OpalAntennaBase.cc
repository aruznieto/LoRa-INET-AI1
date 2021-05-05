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

#include "inet/physicallayer/base/packetlevel/OpalAntennaBase.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

namespace inet {

namespace physicallayer {

OpalAntennaBase::OpalAntennaBase() :
    mobility(nullptr),
    numAntennas(-1)
{
}

void OpalAntennaBase::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        mobility = getModuleFromPar<IMobility>(par("mobilityModule"), getContainingNode(this));
        numAntennas = par("numAntennas");
        std::cout << "Creando ganancias" << endl;
        FILE *f;
        char *line, *n;

        f = fopen("gains/gainRX.txt", "r");

        int j;
        int i = 0;
        while(!feof(f)){
            j = 0;
            fscanf(f, "%s", line);
            n = strtok(line, "  ");
            while(n != NULL){
                gainRX[i][j] = pow(10,atof(n)/10);
                j++;
                n = strtok(NULL, "  ");
            }
            i++;
        }

        f = fopen("gains/gainTX.txt", "r");

        i = 0;
        while(!feof(f)){
            j = 0;
            fscanf(f, "%s", line);
            n = strtok(line, "  ");
            while(n != NULL){
                gainTX[i][j] = pow(10,atof(n)/10);
                j++;
                n = strtok(NULL, "  ");
            }
            i++;
        }

    }
}

std::ostream& OpalAntennaBase::printToStream(std::ostream& stream, int level) const
{
    return stream;
}

} // namespace physicallayer

} // namespace inet

