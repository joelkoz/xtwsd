/**
  * nos2xt.cpp
  * ------------------------------------
  * National Oceanic Service data to XTide Web Service
  * A RESTful client for xtwsd that converts
  * data from http://tidesandcurrents.noaa.gov
  * and posts to a local xtwsd XTide web service.
  * -------------------------------------
  * 
  * 
  * For a list of station Ids that you may be interested in
  * using this utility with, see
  * // https://tidesandcurrents.noaa.gov/tide_predictions.html?gid=1750#listing
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


#include "httpclient.h"

#include <string>
#include <fstream>

#include "../jutil.hpp"

using json = nlohmann::json;


using namespace std;



const char* NOS_HOST = "tidesandcurrents.noaa.gov";


#include <iostream>


// This is a TOTAL hack, but it saves an uncessary lookup.  Set
// "lastRefCountry" to the "country" code of the reference station.
// This needs to be done either when "stationExists()" is called and
// it finds one, or when a reference station is added to the database
// for the first time.
string lastRefCountry = "";

// If a country has been specified to be used, use that instead of
// what is found from the NOS web site or from the reference
// station.
string specifiedCountry = "";


// Convert the station data in jstat into an XTide time zone
string getTzName(json& jstat) {

    string zoneName = getStr(jstat, "timezone");
    if (zoneName == "EST") {
        return ":America/New_York";
    }
    else {
        string tzname = ":Etc/GMT";
        int tzcorrection = getInt(jstat, "timezonecorr");
        if (tzcorrection > 0) {
            tzname += "+";
        }
        tzname += std::to_string(tzcorrection);
        return tzname;
    }
}


double getDatumVal(json& jdatums, const char* name) {

    for (auto& jdatum : jdatums) {
        if (getStr(jdatum, "name") == name) {
            return getNum(jdatum, "value");
        }
    } // for

    // We should not get here, but if we do...
    return 0.0;
}


// Returns a time offset, assumed to be in "minutes",
// in XTide time offset format (which is HHMM).
int getXTimeOffset(json& j, const char* name) {

    int mins = getInt(j, name);

    if (mins != 0) {
        int sign;
        if (mins < 0) {
            sign = -1;
            mins = -mins;
        }
        else {
            sign = 1;
        }

        int xhrs = mins / 60;
        int xmins = mins % 60;

        return sign * (xhrs * 100 + xmins);
    }
    else {
        return 0;
    }

}


// Return TRUE if the specified stationId already exists in the XTide
// database
bool stationExists(const string& stationId, const char* port, bool silent=false) {

    HttpClient http("127.0.0.1", "http", port);

    if (!silent) {
        printf("Checking for existence of reference station %s - ", stationId.c_str());
    }

    string path = "/harmonics/";
    path += stationId;

    std::shared_ptr<HttpClient::Response> pResp = http.get(path);

    bool exists = pResp->ok();

    if (!silent) {
        printf("%s\n", (exists ? "already exists" : "not in database"));
    }

    if (exists) {
        json jref = pResp->asJson();
        lastRefCountry = getStr(jref, "country");
    }

    return exists;
}



// Post json harmonics data to the XTide server..
int postXTide(json& jxt, const char* port) {

    printf("Posting new data to XTide Web Service...\n");

    string country = getStr(jxt, "country");
    if (country != "USA") {
        // For non-USA stations, append the country to the name if it is
        // not there already
        string oldName = getStr(jxt, "name");
        if (oldName.find(country) == std::string::npos) {
            string newName = oldName + ", " + country;
            jxt["name"] = newName;
        }
    }

    printf("%s\n", jxt.dump(2).c_str());

    HttpClient http("127.0.0.1", "http", port);

    std::shared_ptr<HttpClient::Response> pResp = http.post("/harmonics", jxt);
    if (pResp->ok()) {
        json result = pResp->asJson();
        printf("New station added at index %d\n", getInt(result, "index"));

        if (getBool(jxt, "referenceStation")) {
            // We just added a new reference station - save the Country...
            lastRefCountry = getStr(jxt, "country");
        }

        return EXIT_SUCCESS;
    }
    else {
        printf("Unexpected result adding station: result code %d, body: %s\n", pResp->result_int(), pResp->getBody().c_str());
        return EXIT_FAILURE;
    }
}



// Convert NOS data to XTide
int convertStation(const string stationId, const char* port, bool skipExisting) {


    if (skipExisting) {
        string fullId = "NOS:";
        fullId += stationId;
        if (stationExists(fullId, port, true)) {
            printf("Station Id %s already exists in XTide - skipping.", stationId.c_str());
            return EXIT_SUCCESS;
        }
    }

    HttpClient https(NOS_HOST, "https");

    printf("Looking up station id %s...\n", stationId.c_str());
    string base = "/mdapi/v1.0/webapi/stations/";
    base += stationId;

    string path = base;
    path += ".json";
    std::shared_ptr<HttpClient::Response> pResp = https.get(path);
    if (pResp->ok()) {
        json jresult = pResp->asJson();
        json jstat = jresult["stations"][0];
        json jxt;

        string xtsid = "NOS:";
        xtsid += stationId;
        jxt["id"] = xtsid;

        jxt["name"] = getStr(jstat, "name");
        jxt["levelUnits"] = "feet";
        if (!getBool(jstat, "tidal")) {
            jxt["type"] = "current";
        }
        else {
            jxt["type"] = "tide";
        }

        json jsource;
        jsource["context"] = "NOS";
        jsource["name"] = "CO-OPS Metadata API";
        jsource["stationId"] = stationId;
        jxt["source"] = jsource;
        jxt["comments"] = "nos2xt";

        json pos;
        pos["lat"] = jstat["lat"];
        pos["long"] = jstat["lng"];
        jxt["position"] = pos;



        if (!specifiedCountry.empty()) {
            jxt["country"] = specifiedCountry;
        }
        else {
            string state = getStr(jstat,"state");
            if (state.length() == 2) {
                jxt["country"] = "USA";
                jxt["name"] = getStr(jstat, "name") + ", " + getStr(jstat, "state");
            }
            else {
                jxt["country"] = state;
            }
        }

        jxt["timezone"] = getTzName(jstat);

        if (jstat.count("reference_id") == 0) {
            // This is a reference station...
            jxt["referenceStation"] = true;
            printf("Retrieving harmonic constants for reference station %s...\n", getStr(jstat, "name").c_str());
            path = base;
            path += "/harcon.json";
            pResp = https.get(path);
            if (pResp->ok()) {
                printf("Harmonic constants received.\n");
                json jharm = pResp->asJson();
                jxt["levelUnits"] = getStr(jharm, "units");
                json jxtharm;
                jxtharm["confidence"] = 10;
                jxtharm["datum"] = "Mean Lower Low Water";
                jxtharm["zoneOffset"] = 0;
                auto jxtconst = json::array();
                json& jconst = jharm["HarmonicConstituents"];
                for (auto& jc : jconst) {
                    double amp = getNum(jc, "amplitude");
                    double epoch = getNum(jc, "phase_GMT");
                    if (amp != 0.0 || epoch != 0.0) {
                        json jxtc;
                        jxtc["name"] = jc["name"];
                        jxtc["amp"] = amp;
                        jxtc["epoch"] = epoch;
                        jxtconst += jxtc;
                    }
                }
                jxtharm["constituents"] = jxtconst;

                printf("Retrieving datum values for reference station...\n");
                path = base;
                path += "/datums.json";
                pResp = https.get(path);
                if (pResp->ok()) {
                    json jresp = pResp->asJson();
                    json jdatums = jresp["datums"];
                    double mlw = getDatumVal(jdatums, "MLW");
                    double mllw = getDatumVal(jdatums, "MLLW");
                    jxtharm["datumOffset"] = mlw - mllw;
                    jxt["harmonics"] = jxtharm;

                    return postXTide(jxt, port);
                }
                else {
                    printf("Unexpected result - datum query returned code %d\n", pResp->result_int());
                    return EXIT_FAILURE;
                }
            }
            else {
                printf("Unexpected result - harmonic query returned code %d\n", pResp->result_int());
                return EXIT_FAILURE;
            }
        }
        else {
            // This is a subordinate station..
            jxt["referenceStation"] = false;
            printf("Retrieving data for subordiante station %s...\n", getStr(jstat, "name").c_str());
            path = base;
            path += "/tidepredoffsets.json";
            pResp = https.get(path);
            if (pResp->ok()) {
                printf("Offset data received.\n");
                json joffs = pResp->asJson();
                json jxtoffs;
                jxtoffs["maxLevelMultiply"] = joffs["heightOffsetHighTide"];
                jxtoffs["minLevelMultiply"] = joffs["heightOffsetLowTide"];
                jxtoffs["maxTimeAdd"] = getXTimeOffset(joffs, "timeOffsetHighTide");
                jxtoffs["minTimeAdd"] = getXTimeOffset(joffs, "timeOffsetLowTide");

                string refStationId = joffs["refStationId"].get<string>();
                string fullRefStationId = "NOS:";
                fullRefStationId += refStationId;
                jxtoffs["referenceStationId"] = fullRefStationId;

                jxt["offsets"] = jxtoffs;

                if (!stationExists(fullRefStationId, port)) {
                    printf("Adding reference station data to database...\n");
                    if (convertStation(refStationId, port, false) == EXIT_FAILURE) {
                        printf("Adding reference station failed. Aborting.\n");
                        return EXIT_FAILURE;
                    }
                }

                if (!specifiedCountry.empty()) {
                    jxt["country"] = specifiedCountry;
                }
                else {
                    // Make sure country matches that of primary station...
                    jxt["country"] = lastRefCountry;
                }

                return postXTide(jxt, port);

            }
            else {
                printf("Unexpected result - harmonic query returned code %d\n", pResp->result_int());
                return EXIT_FAILURE;
            }
        }
    }
    else {
        printf("Unexpected result - Station query returned code %d\n", pResp->result_int());
        return EXIT_FAILURE;
    }



}


void printUsage() {
    printf("Usage: nos2xt [stationId|-f stationListFileName] <-p xtwsd_server_port> <-c countryNameToUse> <-u>\n");
    printf("   -f nnnnn Process a list of NOS station Ids from the text file nnnn\n");
    printf("   -p pppp Port number of xtwsd web service daemon (default 8080)\n");
    printf("   -c nnnn Force the country name to be nnnn\n");
    printf("   -u Update station Id(s) even if it already exists in database (default NO)\n");
    printf("      ^Warning! '-u' option may crash sever due to possible bug in libtcd\n");
    printf("   example:s\n");
    printf("      nos2xt 9710441 -p 8080\n\n");
    printf("      nos2xt -f my_station_list.txt -p 8080\n\n");
}


int main(const int argc, const char** argv)
{

    printf("nos2xt v0.2\n");

    string stationId;
    string fName;
    const char* port = "8080";
    char invalidArg = 0;
    bool skipExisting = true;

    int a = 1;
    while (a < argc) {
        if (argv[a][0] == '-') {
            switch (argv[a][1]) {
                case 'p':
                    if (a+1 < argc) {
                        port = argv[a+1];
                    }
                    a += 2;
                    break;
                case 'f':
                    if (a+1 < argc) {
                        fName = argv[a+1];
                    }
                    a += 2;
                    break;

                case 'c':
                    if (a+1 < argc) {
                        specifiedCountry = argv[a+1];
                    }
                    a += 2;
                    break;

                case 'u':
                    skipExisting = false;
                    a++;
                    break;

                default:
                    invalidArg = argv[a][1];
                    a++;
            }
        }
        else {
            stationId = argv[a];
            a++;
        }
    }


    if (invalidArg != 0) {
        printf("Invalid argument: %c\n", invalidArg);
        printUsage();
        return EXIT_FAILURE;
    }
    else if (fName.length() > 0) {
        printf("Processing input file %s\n", fName.c_str());

        std::ifstream infile(fName);
        int lineNum = 0;
        while (std::getline(infile, stationId)) {
            lineNum++;
            if (stationId.substr(0,1) != "#" && stationId.substr(0,1) != "@") {
                // Not a comment - assume its a station Id...
                int result = convertStation(stationId, port, skipExisting);
                if (result == EXIT_SUCCESS) {
                    printf("\n\n");
                }
                else {
                    printf("Could not process line %d, station Id %s. Stopping.\n", lineNum, stationId.c_str());
                    return EXIT_FAILURE;
                }
            }
            else {
                if (stationId.substr(0,1) == "@") {
                    // This is a specified country
                    specifiedCountry = stationId.substr(1);
                    printf("Using country name '%s'\n", specifiedCountry.c_str());
                }
                else {
                    printf("%s\n", stationId.c_str());
                }
            }
        } // while

        printf("\nDone. %d lines processed.\n", lineNum);
    }
    else if (stationId.length() > 0) {
       return convertStation(stationId, port, skipExisting);
    }
    else {
        printUsage();
        return EXIT_FAILURE;
    }

}
