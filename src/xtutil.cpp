#include "xtutil.h"

#include <math.h>
#include <map>
#include <cmath> 
#include <cstdlib>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <ctime>
#include <iostream>

// Distance calculation found here: https://stackoverflow.com/questions/10198985/calculating-the-distance-between-2-latitudes-and-longitudes-that-are-saved-in-a
#define earthRadiusKm 6371.0


using namespace libxtide;
using namespace std;


// This function converts decimal degrees to radians
double xtutil::deg2rad(double deg) {
  return (deg * M_PI / 180);
}

//  This function converts radians to decimal degrees
double xtutil::rad2deg(double rad) {
  return (rad * 180 / M_PI);
}

/**
 * Returns the distance between two points on the Earth.
 * Direct translation from http://en.wikipedia.org/wiki/Haversine_formula
 * @param lat1d Latitude of the first point in degrees
 * @param lon1d Longitude of the first point in degrees
 * @param lat2d Latitude of the second point in degrees
 * @param lon2d Longitude of the second point in degrees
 * @return The distance between the two points in kilometers
 */
double xtutil::distanceEarth(const Coordinates& c1, const Coordinates& c2) {
  double lat1r, lon1r, lat2r, lon2r, u, v;
  lat1r = deg2rad(c1.lat());
  lon1r = deg2rad(c1.lng());
  lat2r = deg2rad(c2.lat());
  lon2r = deg2rad(c2.lng());
  u = sin((lat2r - lat1r)/2);
  v = sin((lon2r - lon1r)/2);
  return 2.0 * earthRadiusKm * asin(sqrt(u * u + cos(lat1r) * cos(lat2r) * v * v));
}


// found at http://www.zedwood.com/article/cpp-is-valid-utf8-string-function
bool xtutil::utf8_check_is_valid(const string& string)
{
    int c,i,ix,n,j;
    for (i=0, ix=string.length(); i < ix; i++)
    {
        c = (unsigned char) string[i];
        //if (c==0x09 || c==0x0a || c==0x0d || (0x20 <= c && c <= 0x7e) ) n = 0; // is_printable_ascii
        if (0x00 <= c && c <= 0x7f) n=0; // 0bbbbbbb
        else if ((c & 0xE0) == 0xC0) n=1; // 110bbbbb
        else if ( c==0xed && i<(ix-1) && ((unsigned char)string[i+1] & 0xa0)==0xa0) return false; //U+d800 to U+dfff
        else if ((c & 0xF0) == 0xE0) n=2; // 1110bbbb
        else if ((c & 0xF8) == 0xF0) n=3; // 11110bbb
        //else if (($c & 0xFC) == 0xF8) n=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
        //else if (($c & 0xFE) == 0xFC) n=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
        else return false;
        for (j=0; j<n && i<ix; j++) { // n bytes matching 10bbbbbb follow ?
            if ((++i == ix) || (( (unsigned char)string[i] & 0xC0) != 0x80))
                return false;
        }
    }
    return true;
}

// found at https://stackoverflow.com/questions/154536/encode-decode-urls-in-c
string xtutil::url_encode(const string &value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << uppercase;
        escaped << '%' << setw(2) << int((unsigned char) c);
        escaped << nouppercase;
    }

    return escaped.str();
}


string xtutil::toString(Timestamp& ts, const Dstr& timezone) {
   Dstr dstr;
   ts.print(dstr, timezone);
   return string(dstr.aschar());
}


static map<string, int>* pContextMap = NULL;;
static map<int, string>* pIndexMap = NULL;

map<string, int>* getContextMap() {
    if (pContextMap == NULL) {
        fflush(stderr);
        std::cerr << "Building context map...";
        fflush(stderr);

        TIDE_RECORD rec;

        pContextMap = new map<string, int>();
        pIndexMap = new map<int, string>();

        StationIndex& stations = Global::stationIndex();
        for (int s = 0; s < stations.size(); s++) {
            StationRef*  pRef = stations[s];
            if (open_tide_db(pRef->harmonicsFileName.aschar())) {
                if (read_tide_record(pRef->recordNumber, &rec) >= 0) {
                    string key = rec.station_id_context;
                    key += ":";
                    key += rec.station_id;
                    (*pContextMap)[key] = s;
                    (*pIndexMap)[s] = key;
                }
            }
        }

        std::cerr << "Done." << std::endl;
        fflush(stderr);

    }
    return pContextMap;
}


int xtutil::getStationIndex(const Dstr &harmonicsFileName, const uint32_t hFileRecordNumber) {

   StationIndex& stations = Global::stationIndex();

    for (int s = 0; s < stations.size(); s++) {
        StationRef*  pRef = stations[s];
        if (pRef->harmonicsFileName == harmonicsFileName &&
            pRef->recordNumber == hFileRecordNumber) {
              return s;
        }
    }

    return -1;
}


string* pEmptystr = new string();

const string& xtutil::getStationId(int stationNdx) {
    getContextMap();
    try {
        return pIndexMap->at(stationNdx);
    }
    catch (...) {
        return *pEmptystr;
    }
}


const std::string& xtutil::getStationId(const Dstr &harmonicsFileName, const uint32_t hFileRecordNumber) {
    int index = getStationIndex(harmonicsFileName, hFileRecordNumber);
    if (index != -1) {
        return getStationId(index);
    }
    else {
        return *pEmptystr;
    }
}



int xtutil::getStationIndex(const string& stationId) {

    std::size_t found = stationId.find(":");
    if (found != std::string::npos) {
        // This is in the format of context::stationId.
        map<string, int>* pMap = getContextMap();
        try {
            return pMap->at(stationId);
        }
        catch (...) {
            // Key not found - return -1
            return -1;
        }
    }
    else {
        // Is it a valid number?
        try {
            return stoi(stationId);
        }
        catch (...) {
            // Invalid conversion - return -1
            return -1;
        }
    }
}


bool xtutil::stationIndexValid(int internalId) {
    StationIndex& stations = Global::stationIndex();
    return internalId >= 0 && internalId < stations.size();
}
