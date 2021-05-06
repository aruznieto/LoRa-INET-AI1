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

#include "OpalAntennaBase.h"
#include "inet/common/ModuleAccess.h"
#include <functional>
#include <fstream>
#include <string>

namespace inet {

namespace physicallayer {

void OpalAntennaBase::initialize(int stage)
{

    AntennaBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        std::string path = par("gainPath");
        if(path == "") throw "You must set a path to the gain of antennas";
        std::ifstream infile(path);
        std::string line;
        std::string delimiter = "  ";
        size_t pos = 0;
        std::string token;
        double firstword = true;
        while (std::getline(infile, line)) {
            firstword = true;
            pos = 0;
            std::vector<double> v;
            while ((pos = line.find(delimiter)) != std::string::npos) {
                token = line.substr(0, pos);
                if(!firstword) {
                    double gain = pow(10,atof(token.c_str())/10);
                    v.push_back(gain);
                }
                firstword = false;
                line.erase(0, pos + delimiter.length());
            }
            gainsAntenna.push_back(v);
        }

        infile.close();

        //std::cout << "Matriz: Filas = " << gains.size() << " Columnas = " << gains[0].size() << endl;
        //std::cout << "Matriz TX: Filas = " << gainsTX.size() << " Columnas = " << gainsTX[0].size() << endl;
        //std::cout << gainsAntenna[5][65] << endl;

    }
}

} // namespace physicallayer

} // namespace inet

