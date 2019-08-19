#include <memory>
#include <string>

#include <nlohmann/json.hpp>
#include <served/served.hpp>

#include "_libxtide.h"
#include "nearstations.h"
#include "xtutil.h"
#include "jschema.h"
#include "jsonxt.h"

using namespace std;
using namespace libxtide;
using json = nlohmann::json;

/**
  * main.cpp
  * ------------------------------------
  * XTide Web Service Daemon
  * A RESTful service wrapper for XTide -
  * the tide and current prediction library
  * written by David Flater
  * -------------------------------------
  * 
  * This is the "main" module and contains all of the RESTful resource
  * definitions using the "served" framework.
  * 
  * @author Joel Kozikowski
  */

// 
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


#define OK 200
#define BAD_REQUEST 400
#define INTERNAL_SERVER_ERROR 500

/**
 * For responses that have an optional filter based on tide only stations or
 * current only stations...
 */
class StationTypeFilter {

    public:
        StationTypeFilter(string strFilter) {
            if (strFilter == "tide") {
                typeNum = 1;
            }
            else if (strFilter == "current") {
                typeNum = 2;
            }
            else {
                typeNum = 0;
            }
        }

        bool qualifies(StationRef* pRef) {
            switch (typeNum) {
                case 0:
                    return true;

                case 1:
                    return !pRef->isCurrent;

                case 2:
                    return pRef->isCurrent;
            } 

            return false;
        }

    private:
       int typeNum;
};



/**
 * Closes the specified session, returning the specified json
 * object as the value returned to the client.
 */
void returnjson(served::response& res, json& j, int statusCode = OK) {
    const string body = j.dump(-1, ' ', true);
    res.set_status(statusCode);
    res.set_body(body);
     res.set_header("Content-Type", "application/json");
}



void returnerror(served::response& res, const char* errorMsg, int statusCode = INTERNAL_SERVER_ERROR) {

     const string body = errorMsg;
     res.set_status(statusCode);
     res.set_body(body);
     res.set_header("Content-Type", "text/plain");
}



void returnbadstation(served::response& res, const string& stationId) {

     string body = "Invalid station Id: ";
     body += stationId;
     res.set_status(BAD_REQUEST);
     res.set_body(body);
     res.set_header("Content-Type", "text/plain");
}



int convert_to(std::string& val, int typeVal) {
    return stoi(val);
}


unsigned int convert_to(std::string& val, unsigned int typeVal) {
    return stoul(val);
}



double convert_to(std::string& val, double typeVal) {
    return stod(val);
}


string convert_to(std::string& val, string typeVal) {
    return val;
}


// const char* convert_to(std::string& val, const char* typeVal) {
//     return val.c_str();
// }


template <typename T>
T get_query_parameter(const served::request& req, const char* paramName, T defaultVal) {
    string val = req.query[paramName];
    if (val.empty()) {
        return defaultVal;
    }
    else {
        return convert_to(val, defaultVal);
    }
}


string get_query_parameter(const served::request& req, const char* paramName, string defaultVal="") {
    string val = req.query[paramName];
    if (val.empty()) {
        return defaultVal;
    }
    else {
        return val;
    }
}


bool has_query_parameter(const served::request& req, const char* paramName) {
    string val = req.query[paramName];
    return !val.empty();
}


template <typename T>
T get_path_parameter(const served::request& req, const char* paramName, T defaultVal) {
    string val = req.params[paramName];
    if (val.empty()) {
        return defaultVal;
    }
    else {
        return convert_to(val, defaultVal);
    }
}


string get_path_parameter(const served::request& req, const char* paramName, string defaultVal = "") {
    string val = req.params[paramName];
    if (val.empty()) {
        return defaultVal;
    }
    else {
        return val;
    }
}


/**
 * Handler for GET /locations
 */
void get_locations_handler(served::response& res, const served::request& req)
{
    StationTypeFilter filter(get_path_parameter(req, "stationType"));
    int filterRef = get_query_parameter<bool>(req, "referenceOnly", 0);
    
    json jLocs = json::array();

    StationIndex& stations = Global::stationIndex();

    for (int s = 0; s < stations.size(); s++) {
        StationRef*  pRef = stations[s];
        if (filter.qualifies(pRef)) {
            if (!filterRef || pRef->isReferenceStation) {
                json j = json({});
                tojson(pRef, j);
                jLocs += j;
            }
        }
    }

    returnjson(res, jLocs);
}


/**
 * Handler for GET /station
 */
void get_station_handler(served::response& res, const served::request& req)
{
    int stationIndex = xtutil::getStationIndex(get_path_parameter(req, "stationId"));
    if (!xtutil::stationIndexValid(stationIndex)) {
        returnbadstation(res, get_path_parameter(req, "stationId"));
        return;
    }

    StationIndex& stations = Global::stationIndex();
    Station* station = stations[stationIndex]->load();
    StationRef*  pRef = stations[stationIndex];

    json j;
    tojson(station, pRef, j);

    int localTime = get_query_parameter<int>(req, "local", 0);

    Timestamp startTime;
    Dstr timezone(UTC);
    if (localTime) {
        timezone = station->timezone;
    }
    if (has_query_parameter(req, "start")) {
        Dstr timestring = get_query_parameter<string>(req, "start", "").c_str();
        startTime = Timestamp(timestring, timezone);
    }
    else {
        startTime = Timestamp(std::time(nullptr));
    }

    int days = get_query_parameter<int>(req, "days", 1);

    Timestamp endTime = startTime + Interval(days * 60 * 60 * 24);
 

    Station::TideEventsFilter filter = Station::TideEventsFilter::maxMin;
    if (get_query_parameter(req, "detailed", 0) == 1) {
        filter = Station::TideEventsFilter::noFilter;
    }

    TideEventsOrganizer eventList;
    station->predictTideEvents(startTime, endTime, eventList, filter);
    setEvents(eventList, j, &timezone);

    returnjson(res, j);
}


/**
 * Handler for GET /graph
 */
void get_graph_handler(served::response& res, const served::request& req)
{
    int stationIndex = xtutil::getStationIndex(get_path_parameter(req, "stationId"));
    if (!xtutil::stationIndexValid(stationIndex)) {
        returnbadstation(res, get_path_parameter(req, "stationId"));
        return;
    }

    StationIndex& stations = Global::stationIndex();
    Station* station = stations[stationIndex]->load();
    StationRef*  pRef = stations[stationIndex];

    Timestamp startTime;
    Dstr timezone(UTC);
    if (has_query_parameter(req, "start")) {
        Dstr timestring = get_query_parameter(req, "start").c_str();
        startTime = Timestamp(timestring, timezone);
    }
    else {
        startTime = Timestamp(std::time(nullptr));
    }
 
    unsigned int width = get_query_parameter<unsigned int>(req, "width", 1200);
    unsigned int height = get_query_parameter<unsigned int>(req, "height", 400);
    SVGGraph svg (width, height);
    Dstr text_out;
    svg.drawTides(station, startTime);
    svg.print(text_out);

    const string body = text_out.aschar();

    res.set_status(BAD_REQUEST);
    res.set_body(body);
    res.set_header("Content-Type", "image/svg+xml");
}


/**
 * Handler for GET /nearest
 */
void get_nearest_handler(served::response& res, const served::request& req)
{
    StationTypeFilter filter(get_path_parameter(req, "stationType"));
    
    json jnear = json::array();

    StationIndex& stations = Global::stationIndex();

    double lat = get_query_parameter(req, "lat", 26.2567);
    double lng = get_query_parameter(req, "lng", -80.08);
    unsigned int count = get_query_parameter<unsigned int>(req, "count", 5);
    int filterRef = get_query_parameter<int>(req, "referenceOnly", 0);

    NearStations nearest(lat, lng, count);

    for (int s = 0; s < stations.size(); s++) {
        StationRef*  pRef = stations[s];
        if (filter.qualifies(pRef)) {
            if (!filterRef || pRef->isReferenceStation) {
                nearest.check(pRef);
            }
        }
    } // for


    for (int i = 0; i < nearest.getStationCount(); i++) {
        NearStations::Node* pNear = nearest[i];
        json j;
        j["distance"] = pNear->distance;
        tojson(pNear->pRef, j);
        jnear += j;
    }

    returnjson(res, jnear);
}



void get_schema_handler(served::response& res, const served::request& req)
{
    json schema;
    getJsonSchema(schema);

    if (!schema.empty()) {
        returnjson(res, schema);
    }
    else {
        returnerror(res, "Could not open database");
    }

}


/**
 * Handler for GET /harmonics
 */
void get_harmonics_handler(served::response& res, const served::request& req)
{
    if (get_path_parameter(req, "stationId") == "schema") {
        return get_schema_handler(res, req);
    }
   
    int stationIndex = xtutil::getStationIndex(get_path_parameter(req, "stationId"));
    if (!xtutil::stationIndexValid(stationIndex)) {
        returnbadstation(res, get_path_parameter(req, "stationId"));
        return;
    }

    json j;

    getStationHarmonicsAsJson(stationIndex, j);

    if (!j.empty()) {
        returnjson(res, j);
    }
    else {
        returnerror(res, "Could not open database");
    }

}



/**
 * Handler for POST /harmonics
 */
void post_harmonics_handler(served::response& res, const served::request& req)
{
    if (req.header("Content-Type") != "application/json") {
        returnerror(res, "Request must be of type application/json", BAD_REQUEST);
        return;
    }

    try {
        json j = json::parse(req.body());
        json status;

        setStationHarmonicsFromJson(j, status);

        returnjson(res, status, status["statusCode"].get<int>());
    }
    catch (nlohmann::detail::parse_error& err) {
        string msg = "Error parsing input string: ";
        msg += err.what();
        returnerror(res, msg.c_str(), BAD_REQUEST);
    }
    catch (const std::exception& err) {
        returnerror(res, err.what());
    }

}




/**
 * Handler for GET /tcd
 */
void get_tcd_handler(served::response& res, const served::request& req)
{
    json j;
    StationIndex& stations = Global::stationIndex();
    StationRef*  pRef = stations[0];
    if (open_tide_db(pRef->harmonicsFileName.aschar())) {
        auto db = get_tide_db_header();

        json version;
        version["major_rev"] = db.major_rev;
        version["minor_rev"] = db.minor_rev;
        version["libtcd"] = db.version;
        j["version"] = version;
        j["start_year"] = db.start_year;
        j["end_year"] = db.start_year + db.number_of_years;
        j["number_of_records"] = db.number_of_records;

        close_tide_db();
    }
    returnjson(res, j);
}



int main(const int argc, const char** argv)
{

    printf("xtwsd v0.2\n");

    const char* port = "8080";
    if (argc >= 2) {
        port = argv[1];
    }

	// Create a multiplexer for handling requests
	served::multiplexer mux;

    mux.handle("/locations/{stationType}").get(get_locations_handler);
    mux.handle("/locations").get(get_locations_handler);
    mux.handle("/location/{stationId}").get(get_station_handler);
    mux.handle("/graph/{stationId}").get(get_graph_handler);
    mux.handle("/nearest/{stationType}").get(get_nearest_handler);
    mux.handle("/nearest").get(get_nearest_handler);
    mux.handle("/harmonics/{stationId}").get(get_harmonics_handler);
    mux.handle("/harmonics").post(post_harmonics_handler);
    mux.handle("/tcd").get(get_tcd_handler);
    
    printf("Starting web service on port %s\n", port);
    
	served::net::server server("0.0.0.0", port, mux);
	server.run(10);

    return EXIT_SUCCESS;
}