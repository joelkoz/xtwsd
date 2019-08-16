#include <memory>
#include <string>

#include <nlohmann/json.hpp>
#include "restbed.h"

#include "_libxtide.h"
#include "nearstations.h"
#include "xtutil.h"
#include "jschema.h"
#include "jsonxt.h"

using namespace std;
using namespace restbed;
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
  * definitions using the "restbed" framework.
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
void returnjson(const shared_ptr< Session > session, json& j, int statusCode = OK) {
    const string body = j.dump(-1, ' ', true);
    session->close( statusCode, body, { { "Content-Length", ::to_string( body.size( ) ) },
                                { "Content-Type", "application/json"  } } );
}



void returnerror(const shared_ptr< Session > session, const char* errorMsg, int statusCode = INTERNAL_SERVER_ERROR) {

    const string body = errorMsg;
    session->close( statusCode, body, { { "Content-Length", ::to_string( body.size( ) ) },
                                        { "Content-Type", "text/plain" } } );
}


void returnbadstation(const shared_ptr< Session > session) {

    const auto& request = session->get_request( );
    string stationId = request->get_path_parameter("stationId", "");

    string body = "Invalid station Id: ";
    body += stationId;

    session->close( BAD_REQUEST, body, { { "Content-Length", ::to_string( body.size( ) ) },
                                         { "Content-Type", "text/plain" } } );
}



/**
 * Handler for GET /locations
 */
void get_locations_handler( const shared_ptr< Session > session )
{
    const auto& request = session->get_request();

    StationTypeFilter filter(request->get_path_parameter("stationType", ""));
    int filterRef = request->get_query_parameter<bool>("referenceOnly", 0);
    
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

    returnjson(session, jLocs);
}


/**
 * Handler for GET /station
 */
void get_station_handler( const shared_ptr< Session > session )
{
    const auto& request = session->get_request( );
    
    int stationIndex = xtutil::getStationIndex(request->get_path_parameter("stationId", ""));
    if (!xtutil::stationIndexValid(stationIndex)) {
        returnbadstation(session);
        return;
    }

    StationIndex& stations = Global::stationIndex();
    Station* station = stations[stationIndex]->load();
    StationRef*  pRef = stations[stationIndex];

    json j;
    tojson(station, pRef, j);

    int localTime = request->get_query_parameter<int>("local", 0);

    Timestamp startTime;
    Dstr timezone(UTC);
    if (localTime) {
        timezone = station->timezone;
    }
    if (request->has_query_parameter("start")) {
        Dstr timestring = request->get_query_parameter("start").c_str();
        startTime = Timestamp(timestring, timezone);
    }
    else {
        startTime = Timestamp(std::time(nullptr));
    }

    int days = request->get_query_parameter<int>("days", 1);

    Timestamp endTime = startTime + Interval(days * 60 * 60 * 24);
 

    Station::TideEventsFilter filter = Station::TideEventsFilter::maxMin;
    if (request->get_query_parameter("detailed", 0) == 1) {
        filter = Station::TideEventsFilter::noFilter;
    }

    TideEventsOrganizer eventList;
    station->predictTideEvents(startTime, endTime, eventList, filter);
    setEvents(eventList, j, &timezone);

    returnjson(session, j);
}


/**
 * Handler for GET /graph
 */
void get_graph_handler( const shared_ptr< Session > session )
{
    const auto& request = session->get_request( );
    
    int stationIndex = xtutil::getStationIndex(request->get_path_parameter("stationId", ""));
    if (!xtutil::stationIndexValid(stationIndex)) {
        returnbadstation(session);
        return;
    }

    StationIndex& stations = Global::stationIndex();
    Station* station = stations[stationIndex]->load();
    StationRef*  pRef = stations[stationIndex];

    Timestamp startTime;
    Dstr timezone(UTC);
    if (request->has_query_parameter("start")) {
        Dstr timestring = request->get_query_parameter("start").c_str();
        startTime = Timestamp(timestring, timezone);
    }
    else {
        startTime = Timestamp(std::time(nullptr));
    }
 
    unsigned int width = request->get_query_parameter<unsigned int>("width", 1200);
    unsigned int height = request->get_query_parameter<unsigned int>("height", 400);
    SVGGraph svg (width, height);
    Dstr text_out;
    svg.drawTides(station, startTime);
    svg.print(text_out);

    const string body = text_out.aschar();
    session->close( OK, body, { { "Content-Length", ::to_string( body.size( ) ) },
                                { "Content-Type", "image/svg+xml" } } );

}


/**
 * Handler for GET /nearest
 */
void get_nearest_handler( const shared_ptr< Session > session )
{
    const auto& request = session->get_request( );
    StationTypeFilter filter(request->get_path_parameter("stationType", ""));
    
    json jnear = json::array();

    StationIndex& stations = Global::stationIndex();

    double lat = request->get_query_parameter("lat", 26.2567);
    double lng = request->get_query_parameter("lng", -80.08);
    unsigned int count = request->get_query_parameter<unsigned int>("count", 5);
    int filterRef = request->get_query_parameter<bool>("referenceOnly", 0);

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

    returnjson(session, jnear);
}



void get_schema_handler( const shared_ptr< Session > session )
{
    const auto& request = session->get_request( );

    json schema;
    getJsonSchema(schema);

    if (!schema.empty()) {
        returnjson(session, schema);
    }
    else {
        returnerror(session, "Could not open database");
    }

}


/**
 * Handler for GET /harmonics
 */
void get_harmonics_handler( const shared_ptr< Session > session )
{
    const auto& request = session->get_request( );

    if (request->get_path_parameter("stationId") == "schema") {
        return get_schema_handler(session);
    }
   
    int stationIndex = xtutil::getStationIndex(request->get_path_parameter("stationId", ""));
    if (!xtutil::stationIndexValid(stationIndex)) {
        returnbadstation(session);
        return;
    }

    json j;

    getStationHarmonicsAsJson(stationIndex, j);

    if (!j.empty()) {
        returnjson(session, j);
    }
    else {
        returnerror(session, "Could not open database");
    }

}



/**
 * Handler for PUT /harmonics
 */
void put_harmonics_handler( const shared_ptr< Session > session )
{

    try {
        const auto& request = session->get_request( );

        string body;
        request->get_body(body);

        json j = json::parse(body);
        json status;

        setStationHarmonicsFromJson(j, status);

        returnjson(session, status, status["statusCode"].get<int>());
    }
    catch (nlohmann::detail::parse_error& err) {
        string msg = "Error parsing input string: ";
        msg += err.what();
        returnerror(session, msg.c_str(), BAD_REQUEST);
    }
    catch (const std::exception& err) {
        returnerror(session, err.what());
    }
}




/**
 * Handler for GET /tcd
 */
void get_tcd_handler( const shared_ptr< Session > session )
{
    const auto& request = session->get_request();

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
    returnjson(session, j);
}



int main( const int, const char** )
{

    printf("xtwsd v1.0\n");
    Service service;

    auto resource = make_shared< Resource >( );
    resource->set_paths( { "/locations/{stationType: .*}", "/locations" } );
    resource->set_method_handler( "GET", get_locations_handler );
    service.publish( resource );
    

    resource = make_shared< Resource >( );
    resource->set_path( "/location/{stationId: .*}" );
    resource->set_method_handler( "GET", get_station_handler );
    service.publish( resource );

    resource = make_shared< Resource >( );
    resource->set_path( "/graph/{stationId: .*}" );
    resource->set_method_handler( "GET", get_graph_handler );
    service.publish( resource );


    resource = make_shared< Resource >( );
    resource->set_paths( { "/nearest/{stationType: .*}", "/nearest" } );
    resource->set_method_handler( "GET", get_nearest_handler );
    service.publish( resource );


    resource = make_shared< Resource >( );
    resource->set_path( "/harmonics/{stationId: .*}" );
    resource->set_method_handler( "GET", get_harmonics_handler );
    service.publish( resource );


    resource = make_shared< Resource >( );
    resource->set_path( "/harmonics" );
    resource->set_method_handler( "PUT", put_harmonics_handler );
    service.publish( resource );


    resource = make_shared< Resource >( );
    resource->set_path( "/tcd" );
    resource->set_method_handler( "GET", get_tcd_handler );
    service.publish( resource );


    auto settings = make_shared< restbed::Settings >( );
    settings->set_port( 8081 );
    settings->set_default_header( "Connection", "close" );
    
    printf("Starting web service...\n");
    service.start( settings );
    
    return EXIT_SUCCESS;
}