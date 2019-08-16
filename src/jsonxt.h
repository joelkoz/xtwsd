#ifndef _jsonxt_H_
#define _jsonxt_H_

#include <memory>
#include <nlohmann/json.hpp>

#include "_libxtide.h"


/**
  * jsonxt.h
  * -------------------------
  * Functions for converting between XTide data structures and json (and vice versa).
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
 * Sets the lat and long properies of j to
 * the specified coordinates.
 */
extern void tojson(libxtide::Coordinates coord, nlohmann::json& j);



/**
 * Populates the json member j.position with the
 * specified coordinates.
 */
extern void setPosition(libxtide::Coordinates coord, nlohmann::json& j);



/**
 * Populates the json object j with data from
 * the station reference pRef.
 */
extern void tojson(libxtide::StationRef* pRef, nlohmann::json& j);


/**
 * Populates the json object j with data from
 * the station pStat.
 */
extern void tojson(libxtide::Station* pStat, libxtide::StationRef* pRef, nlohmann::json& j);



/**
 * Populates the specified json object with
 * event data from TideEvent
 */
extern void tojson(libxtide::TideEvent& event, nlohmann::json& j);



/**
 * Populates the json member j.events with the
 * tide events specified in eventList;
 */

extern void setEvents(libxtide::TideEventsOrganizer& eventList, nlohmann::json& j, Dstr* pTimeZone);




/**
 * Returns a populated json object with the tide or current prediction harmonics
 * data for the specified station Id.
 */
extern void getStationHarmonicsAsJson(int stationId, nlohmann::json& j);



/**
 * Attempts to add or update the XTide harmonics database using the tide station
 * definition stored in j.  TRUE is returned if the write was successful.  status
 * will be populated with a return code and a message string.
 */
extern bool setStationHarmonicsFromJson(nlohmann::json& j, nlohmann::json& status);

#endif
