#ifndef _nearstations_h_
#define _nearstations_h_

#include "_libxtide.h"

/**
  * nearstations.h
  * -------------------------
  * A container class that holds a list of stations that are nearest to a specific point.
  * -------------------------
  * @author Joel Kozikowski
  */

//  (C) 2019 Joel Kozikowski
//
//  This file is part of xtwsd.
//
//  xtwsd is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  xtwsd is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Foobar.  If not, see <https://www.gnu.org/licenses/>.



/**
 * A container class that holds a list of stations that are nearest to a specific point.
 */
class NearStations {

    public:
        struct Node {
            double distance;
            libxtide::StationRef* pRef;
        };

        NearStations(double lat, double lng, unsigned int maxStations);

        ~NearStations();

        /**
         * Checks pRef to see if it is one of the nearest stations discovered.
         * If so, it is added to the list.
         */
        void check(libxtide::StationRef* pRef);


        /**
         * Returns the station at the specified position. If
         * no such position exists, NULL is returned.
         */
        Node* operator[](int pos);

        int getStationCount() { return stationCount; }

    private:
        libxtide::Coordinates mine;
        unsigned int maxStations;
        Node* nodes;
        unsigned int stationCount;

        void insert(int posNdx);

};

#endif