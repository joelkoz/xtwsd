#ifndef _xtutil_h_
#define _xtutil_h_

#include <string>

#include "_libxtide.h"

/**
  * xtutil.h
  * -------------------------
  * XTide Utilities
  * Contains helper functions used in the xtwsd XTide Web Service daemon.
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


#define UTC ":UTC"

namespace xtutil {

/**
 * Is the specified string a valid utf8 string?
 */
extern bool utf8_check_is_valid(const std::string& string);


/**
 * Returns a URL encoded version of the specified value
 */
extern std::string url_encode(const std::string &value);



/**
 * Converts the specified xtide Timestamp object to a string
 */
extern std::string toString(libxtide::Timestamp& ts, const Dstr& timezone);


/**
 * Calculate the distance, in KM, between two points.
 */
extern double distanceEarth(const libxtide::Coordinates& c1, const libxtide::Coordinates& c2);


/**
 * Convert the specified degress to radians
 */
extern double deg2rad(double deg);


/**
 * Convert the specified radians to degrees
 */
extern double rad2deg(double rad);


/**
 * Returns the internal id (index of station index) for the specified harmonics file/record number
 * combination.  -1 is returned if nothing was found.
 */
extern int getStationIndex(const Dstr &harmonicsFileName, const uint32_t hFileRecordNumber);


/**
 * Returns the station id for the specified harmonics file/record number
 * combination.  An empty string is returned if nothing was found.
 */
extern const std::string& getStationId(const Dstr &harmonicsFileName, const uint32_t hFileRecordNumber);


/**
 * Returns the internal id of the specified station Id. If no Id is found,
 * -1 is returned.
 */
extern int getStationIndex(const std::string& stationId);


/**
 * Returns the station Id for the specified station index.
 * An empty string is returned if no match is found.
 */
extern const std::string& getStationId(int stationNdx);


/**
 * Returns TRUE if the specified internal Id is a valid station index (i.e. the
 * index is within range)
 */
extern bool stationIndexValid(int stationNdx);



/**
 * Invalidates the context map so it will be re-read. This
 * should be called whenever a new record is added to the database.
 */
extern void invalidateContextMap();


}

#endif