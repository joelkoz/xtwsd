#include "jsonxt.h"

#include <string>

#include "xtutil.h"
#include "stdcapture.h"

using namespace std;
using namespace libxtide;
using json = nlohmann::json;


void tojson(libxtide::Coordinates coord, json& j) {
    j["lat"] = coord.lat();
    j["long"] = coord.lng();
}


void setPosition(libxtide::Coordinates coord, json& j) {
    json jpos;
    tojson(coord, jpos);
    j["position"] = jpos;
}


void tojson(TideEvent& event, json& j, Dstr* pTimeZone) {
    j["time"] = xtutil::toString(event.eventTime, *pTimeZone);
    j["name"] = event.longDescription();
    j["isMaxMin"] = event.isMaxMinEvent();
    if (event.isMaxMinEvent()) {
        j["level"] = event.eventLevel.val();
    }
}



void setEvents(TideEventsOrganizer& eventList, json& j, Dstr* pTimeZone) {

    json jevents = json::array();

    TideEventsIterator it = eventList.begin();
    while (it != eventList.end()) {
        TideEvent event = it->second;
        json je;
        tojson(event, je, pTimeZone);
        jevents += je;
        ++it;
    } // while

    j["events"] = jevents;
}


void tojson(StationRef* pRef, json& j) {
    string name = pRef->name.aschar();
    if (!xtutil::utf8_check_is_valid(name)) {
        name = xtutil::url_encode(name);
    }
    j["index"] = pRef->rootStationIndexIndex;
    j["id"] = xtutil::getStationId(pRef->rootStationIndexIndex);
    j["name"] = name;
    // not used anywhere: j["recordNum"] = pRef->recordNumber;
    // same as id: j["rootNdxNdx"] = pRef->rootStationIndexIndex;
    j["referenceStation"] = pRef->isReferenceStation;
    j["timezone"] = pRef->timezone.aschar();
    if (pRef->isCurrent) {
        j["type"] = "current";
    }
    else {
        j["type"] = "tide";
    }
    setPosition(pRef->coordinates, j);
}



void tojson(Station* pStat, StationRef* pRef, json& j) {

    j["index"] = pRef->rootStationIndexIndex;
    j["id"] = xtutil::getStationId(pRef->rootStationIndexIndex);
    string name = pStat->name.aschar();
    if (!xtutil::utf8_check_is_valid(name)) {
        name = xtutil::url_encode(name);
    }
    j["name"] = name;
    setPosition(pStat->coordinates, j);
    j["timezone"] = pStat->timezone.aschar();
    j["note"] = pStat->note.aschar();
    if (pStat->isCurrent) {
        j["type"] = "current";
    }
    else {
        j["type"] = "tide";
    }
    j["levelUnits"] = Units::shortName(pStat->predictUnits());
}

/**
 * Populates the member j.constituents with
 * the constituent values from the specified tide record
 */
void dumpHarmonicConstituents(TIDE_RECORD& rec, json& j) {

   DB_HEADER_PUBLIC db = get_tide_db_header();

   json clist = json::array();

   for (int c = 0; c < db.constituents; c++) {
        json jc;
        double amp = rec.amplitude[c];
        double epoch = rec.epoch[c];
        if (amp != 0.0 || epoch != 0.0) {
            jc["name"] = get_constituent(c);
            jc["amp"] = amp;
            jc["epoch"] = epoch;
            clist += jc;
        }
   }

   j["constituents"] = clist;
}


/**
 * Populates the json object j with harmonic data
 * from the tide record rec.
 */
void dumpHarmonicType1(TIDE_RECORD& rec, json& j) {
   j["datumOffset"] = rec.datum_offset;
   j["datum"] = get_datum(rec.datum);
   j["zoneOffset"] = rec.zone_offset;
   j["confidence"] = rec.confidence;

   // These values are always zero in every XTide record
   // j["expirationDate"] = rec.expiration_date;
   // j["monthsOnStation"] = rec.months_on_station;
   // j["lastDateOnStation"] = rec.last_date_on_station;

   dumpHarmonicConstituents(rec, j);
}



/**
 * Populates the json object j with tidal offset data
 * from the tide record rec.
 */
void dumpHarmonicType2(StationRef* pRef, TIDE_RECORD& rec, json& j) {

    // For definitions of what these values mean, see https://flaterco.com/xtide/harmonics.html
    j["minTimeAdd"] = rec.min_time_add;
    j["minLevelAdd"] = rec.min_level_add;
    j["minLevelMultiply"] = rec.min_level_multiply;
    j["maxTimeAdd"] = rec.max_time_add;
    j["maxLevelAdd"] = rec.max_level_add;
    j["maxLevelMultiply"] = rec.max_level_multiply;

    if (pRef->isCurrent) {
        j["floodBegins"] = rec.flood_begins;
        j["ebbBegins"] = rec.ebb_begins;    
    }
}


void getStationHarmonicsAsJson(int stationIndex, json& j) {

    StationIndex& stations = Global::stationIndex();
    StationRef*  pRef = stations[stationIndex];
    if (open_tide_db(pRef->harmonicsFileName.aschar())) {
        TIDE_RECORD rec;
        if (read_tide_record(pRef->recordNumber, &rec) != -1) {

            // Uncomment below to display tide data on stderr...
            // dump_tide_record(&rec);

            tojson(pRef, j);

            j["notes"] = rec.notes;
            j["comments"] = rec.comments;
            j["country"] = get_country(rec.country);

            // flow is a structure that exists only for non-tidal current stations
            if (pRef->isCurrent) {
                json flow;
                flow["units"] = get_dir_units(rec.direction_units);
                flow["ebbDirection"] = rec.min_direction;
                flow["floodDirection"] = rec.max_direction;
                j["flow"] = flow;
            }

            j["levelUnits"] = get_level_units(rec.level_units);

            json source;
            source["name"] = rec.source;
            source["context"] = rec.station_id_context;
            source["stationId"] = rec.station_id;
            j["source"] = source;

            if (pRef->isReferenceStation) {
                json ref;
                dumpHarmonicType1(rec, ref);
                j["harmonics"] = ref;
            }
            else {
                json sub;
                dumpHarmonicType2(pRef, rec, sub);
                sub["referenceStationId"] = xtutil::getStationId(pRef->harmonicsFileName, rec.header.reference_station);
                j["offsets"] = sub;
            }
        }

        // // For exploring the database, uncomment below and modify.
        // for (int s = 0; s < stations.size(); s++) {
        //     StationRef*  pRef = stations[s];
        //     if (read_tide_record(pRef->recordNumber, &rec) != -1 &&
        //         pRef->isCurrent && !pRef->isReferenceStation &&
        //         (rec.ebb_begins != 2560 || rec.flood_begins != 2560) ) {
        //         printf("Flow direction found for %d\n", s);
        //     }
        // } // for

        close_tide_db();
    }

}



NV_INT32 getEnumProperty(json& j, const char* propertyName, std::function<NV_INT32(const NV_CHAR *name)> getEnumVal) {
    if (j.count(propertyName)) {
        string strVal = j[propertyName].get<string>();
        return getEnumVal(strVal.c_str());
    }
    else {
        return -1;
    }
}


int updateStationIndex(TIDE_RECORD& rec) {

    int id = Global::stationIndex().size();



    return id;

}


string fmtString(const char* fmt, int num) {
    char* s = new char[strlen(fmt)+20];
    sprintf(s, fmt, num);
    string str(s);
    delete[] s;
    return str;
}


#include "jutil.hpp"

bool setStationHarmonicsFromJson(json& j, json& status) {

    StationIndex& stations = Global::stationIndex();

    StationRef*  pRef = NULL;
    int recordNum = -1;
    int stationIndex = -1;

    if (j.count("index") && j["index"].is_number()) {
        // An index was specified - this is possibly an update?
        stationIndex = j["index"].get<int>();
    }

    if (j.count("id")) {
        stationIndex = xtutil::getStationIndex(j["id"].get<string>());
    }

    if (xtutil::stationIndexValid(stationIndex)) {
        pRef = stations[stationIndex];
        recordNum = pRef->recordNumber;
    }

    if (recordNum == -1) {
        // Add new records to the first database file listed
        pRef = stations[0];
    }

    if (open_tide_db(pRef->harmonicsFileName.aschar())) {

        auto db = get_tide_db_header();

        TIDE_RECORD rec;

        string stationId;

        // Initialize rec with old data, or blank for new records...
        if (recordNum >= 0) {
            if (read_tide_record(recordNum, &rec) == -1) {
                status["statusCode"] = 500;
                status["message"] = fmtString("Could not read tide record %d", stationIndex);
                return false;
            }
        }
        else {
            memset(&rec, 0, sizeof(rec));
        }

        setString(rec.header.name , sizeof(rec.header.name), j, "name");
        rec.header.record_type = getBool(j, "referenceStation") ? TIDE_RECORD_TYPE::REFERENCE_STATION : TIDE_RECORD_TYPE::SUBORDINATE_STATION;
        rec.header.tzfile = getEnumProperty(j, "timezone", find_tzfile);
        rec.country = getEnumProperty(j, "country", find_country);
        rec.level_units = getEnumProperty(j, "levelUnits", find_level_units);
        setString(rec.comments , sizeof(rec.comments), j, "comments");
        setString(rec.notes , sizeof(rec.notes), j, "notes");


        if (j.count("source")) {
            json& s = j["source"];
            setString(rec.source, sizeof(rec.source), s, "name");
            setString(rec.station_id_context, sizeof(rec.station_id_context), s, "context");
            setString(rec.station_id, sizeof(rec.station_id), s, "stationId");
            stationId = rec.station_id_context;
            stationId += ":";
            stationId += rec.station_id;
        }

        if (j.count("position")) {
            json& pos = j["position"];
            rec.header.latitude = getNum(pos, "lat");
            rec.header.longitude = getNum(pos, "long");
        }

        string stationType = getStr(j, "type");

        if (stationType == "current") {
            if (j.count("flow")) {
                json& flow = j["flow"];

                rec.direction_units = getEnumProperty(flow, "units", find_dir_units);
                rec.min_direction = getInt(flow, "ebbDirection");
                rec.max_direction = getInt(flow, "floodDirection");
            }
        }
        else {
            // Use the "NULL" values for tide stations
            rec.min_direction = 361;
            rec.max_direction = 361;
        }

        if (rec.header.record_type == TIDE_RECORD_TYPE::REFERENCE_STATION) {
            rec.header.reference_station = -1;
            // A reference station requires harmonics definitions
            if (j.count("harmonics")) {
                json& harm = j["harmonics"];
                rec.confidence = getInt(harm, "confidence");
                rec.datum = getEnumProperty(harm, "datum", find_datum);
                rec.datum_offset = getNum(harm, "datumOffset");
                rec.zone_offset = getInt(harm, "zoneOffset");
                if (harm.count("constituents")) {
                    json& csts = harm["constituents"];
                    for (auto& cst : csts) {
                        int i = getEnumProperty(cst, "name", find_constituent);
                        if (i >= 0) {
                            rec.amplitude[i] = getNum(cst, "amp");
                            rec.epoch[i] = getNum(cst, "epoch");
                        }
                    } // for
                }
                else {
                    status["statusCode"] = 400;
                    status["message"] = "Reference stations must contain harmonics.constituents data";
                    return false;
                }
            }
            else {
                status["statusCode"] = 400;
                status["message"] = "Reference stations must contain harmonics data";
                return false;
            }
        }
        else {
            // Subordinate stations requires "offsets"
            if (j.count("offsets")) {
                json& off = j["offsets"];

                string refStationId = getStr(off, "referenceStationId");
                int refStationNdx = xtutil::getStationIndex(refStationId);
                rec.header.reference_station = refStationNdx;
                rec.min_time_add = getInt(off, "minTimeAdd");
                rec.min_level_add = getNum(off, "minLevelAdd");
                rec.min_level_multiply = getNum(off, "minLevelMultiply");

                rec.max_time_add = getInt(off, "maxTimeAdd");
                rec.max_level_add = getNum(off, "maxLevelAdd");
                rec.max_level_multiply = getNum(off, "maxLevelMultiply");

                if (stationType == "current") {
                    rec.flood_begins = getInt(off, "floodBegins");
                    rec.ebb_begins = getInt(off, "ebbBegins");
                }
                else {
                    // Use the "NULL" value for tide stations...
                    rec.flood_begins = NULLSLACKOFFSET;
                    rec.ebb_begins = NULLSLACKOFFSET;
                }
            }
            else {
                status["statusCode"] = 400;
                status["message"] = "Subordinate stations must contain offsets data";
                return false;
            }
        }

        if (stationId.empty()) {
            status["statusCode"] = 400;
            status["message"] = "The properties 'source.context' and 'source.stationId' must be set";
            return false;
        }

        int preExistingIndex = xtutil::getStationIndex(stationId);

        StdCapture captureOutput;
        captureOutput.beginCapture();
        if (recordNum >= 0) {
            // Update an existing record...
            if (stationIndex != preExistingIndex) {
                status["statusCode"] = 400;
                string msg = stationId;
                msg += " has a pre-existing index of ";
                msg += to_string(preExistingIndex);
                msg += " which does not match the specified index ";
                msg += to_string(stationIndex);
                status["message"] = msg;
                return false;
            }

            if (update_tide_record(recordNum, &rec, &db)) {
                status["statusCode"] = 200;
                status["index"] = stationIndex;
                close_tide_db();

                return true;
            }
            else {
                status["statusCode"] = 500;
                string msg = fmtString("Could not update database record number %d:\n", recordNum);
                msg += captureOutput.getCapture();
                status["message"] = msg;
                if (captureOutput.getCapture().find("error") != string::npos) {
                    status["statusCode"] = 400;
                }
                return false;
            }
        }
        else {
            // Add a new record...
            if (preExistingIndex != -1) {
                status["statusCode"] = 400;
                string msg = "Station id ";
                msg += stationId;
                msg += " already exists in database (index  ";
                msg += to_string(preExistingIndex);
                msg += "). Updates require 'index' property to be specifid";
                status["message"] = msg;
                return false;
            }

            if (add_tide_record(&rec, &db)) {
                // Update libxtide's interal C++ structure for the newly added record...
                StationRef *sr = new StationRef (pRef->harmonicsFileName,
                                                db.number_of_records - 1,
                                                rec.header.name,
                                                ((rec.header.latitude != 0.0 || rec.header.longitude != 0.0) ?
                                                Coordinates(rec.header.latitude, rec.header.longitude) : Coordinates()),
                                                (char *)get_tzfile(rec.header.tzfile),
                                                (rec.header.record_type == REFERENCE_STATION),
                                                stationType == "current");

                Global::stationIndex().push_back(sr);
                Global::stationIndex().sort();
                Global::stationIndex().setRootStationIndexIndices();
                sr->rootStationIndexIndex = xtutil::getStationIndex(sr->harmonicsFileName, sr->recordNumber);

                status["statusCode"] = 200;
                status["index"] = sr->rootStationIndexIndex;

                xtutil::invalidateContextMap();

                close_tide_db();
                return true;
            }
            else {
                status["statusCode"] = 500;
                string msg = "Could not add new database record:\n";
                msg += captureOutput.getCapture();
                status["message"] = msg;
                if (captureOutput.getCapture().find("error") != string::npos) {
                    status["statusCode"] = 400;
                }
                return false;
            }
        }

        close_tide_db();

        return true;
    }
    else {
        status["statusCode"] = 500;
        status["message"] = "Could not open database";
        return false;
    }

}
