#include "jschema.h"
#include "_libxtide.h"
#include <tcd.h>

using namespace libxtide;


json& addProperty(json& parent, const char* propertyName, const char* type,  const char* description = NULL) {

    json jprop;
    jprop["type"] = type;
    if (description != NULL) {
        jprop["description"] = description;
    }

    auto& props = parent["properties"];
    props[propertyName] = jprop;
    return props[propertyName];
}



json& addNumberProperty(json& parent, const char* propertyName, const char* type = "number", const char* description = NULL, 
                        const char* minVal = NULL, const char* maxVal = NULL) {

    json jprop;
    jprop["type"] = type;
    if (description != NULL) {
        jprop["description"] = description;
    }
    if (minVal != NULL) {
        jprop["minimum"] = minVal;
    }
    if (maxVal != NULL) {
        jprop["maximum"] = maxVal;
    }
    auto& props = parent["properties"];
    props[propertyName] = jprop;
    return props[propertyName];
}


json& addEnumProperty(json& parent,  const char* propertyName, 
                      NV_U_INT32 enumCount, std::function<NV_CHAR *(NV_INT32)> getEnumName,  
                      const char* description = NULL) {

    json jprop;
    jprop["type"] = "string";
    if (description != NULL) {
        jprop["description"] = description;
    }

    if (enumCount > 0) {
        json enumList = json::array();
        for (NV_U_INT32 e = 1; e < enumCount; e++) {
            enumList.push_back(getEnumName(e));
        } // for
        jprop["enum"] = enumList;
    }

    auto& props = parent["properties"];
    props[propertyName] = jprop;
    return props[propertyName];
}



json& addEnumProperty(json& parent,  const char* propertyName, 
                      std::vector<const char*> values,
                      const char* description = NULL) {

    json jprop;
    jprop["type"] = "string";
    if (description != NULL) {
        jprop["description"] = description;
    }
    jprop["enum"] = values;

    auto& props = parent["properties"];
    props[propertyName] = jprop;
    return props[propertyName];
}




json& addObjectProperty(json& parent,  const char* propertyName, const char* description = NULL) {

    json jprop;
    jprop["type"] = "object";
    if (description != NULL) {
        jprop["description"] = description;
    }
    jprop["properties"] = json::object();

    auto& props = parent["properties"];
    props[propertyName] = jprop;
    return props[propertyName];
}


// The following Text description values were taken from XTide's "tide editor" app, file tideDialogHelp.h
const char *datumText = "This is the description of which tidal datum is being used. "
  "In the U.S., it is usually 'Mean Lower Low Water'. "
  "For currents this field is irrelevant.";
const char *datum_offsetText = "For tides, this is the elevation of Mean Sea "
  "Level (MSL) relative to the specified datum in Level Units.  For "
  "currents it is an analogous constant used to calibrate the velocity of "
  "the predicted currents against zero.";
const char *confidenceText = "Confidence is a meaningless indicator of data "
  "quality ranging between 0 and 15 and normally initialized to 10.";
const char *zone_offsetText = "This is the standard time to which epochs are adjusted, a.k.a. the meridian, in hours and minutes east of UTC.  The format is +/-HHMM "
"(i.e. hours * 100 + min) with positive values being east of Greenwich and negative values west. "
"Do not use daylight savings time.";


void getJsonSchema(json& schema) {

    StationIndex& stations = Global::stationIndex();
    StationRef*  pRef = stations[0];
    if (open_tide_db(pRef->harmonicsFileName.aschar())) {
        auto db = get_tide_db_header();

        schema["$schema"] = "http://json-schema.org/draft-07/schema#";
        schema["type"] = "object";
        schema["title"] = "XTide Station Definition";
        schema["description"] = "Values needed to define an xtide tidal or current station";
        schema["properties"] = json::object();
        addProperty(schema, "name", "string", "Tide station name");


        addProperty(schema, "index", "number", "The XTide index number. Leave blank or zero for new entries. Use only recent values returned by the server for updates");
        addProperty(schema, "comments", "string", "Any special comments about this station. Leave blank if none.");
        addProperty(schema, "notes", "string", "Any special notes about this station. Leave blank if none.");
        addProperty(schema, "referenceStation", "boolean", "TRUE if this is a reference station, false if a subordinate station." );

        addEnumProperty(schema, "type", { "tide", "current" }, "What type of information does this station supply?");
        addEnumProperty(schema, "country", db.countries, get_country);
        
        addEnumProperty(schema, "timezone", db.tzfiles, get_tzfile);
        addEnumProperty(schema, "levelUnits", db.level_unit_types, get_level_units);

        auto& flow = addObjectProperty(schema, "flow", "Data for current stations only. See https://flaterco.com/xtide/harmonics.html");
        addNumberProperty(flow, "ebbDirection", "integer", "This is the direction of the maximum ebb current.  Enter a number between 0 and 359, or 361 if unknown.", "0", "361");
        addNumberProperty(flow, "floodDirection", "integer", "This is the direction of the maximum flood current.  Enter a number between 0 and 359, or 361 if unknown.", "0", "361");
        addEnumProperty(flow, "units", db.dir_unit_types, get_dir_units);

        auto& position = addObjectProperty(schema, "position", "Location of station in 'Signed degrees' format");
        addNumberProperty(position, "lat", "number", "Geographic lattitude. Use negative numbers for South, positive for North", "-90", "90");
        addNumberProperty(position, "long", "number", "Geographic longitude. Use negative numbers for West, positive for East", "-180", "180");
        position["required"] = { "lat", "long"};

        auto& source = addObjectProperty(schema, "source", "Source of the data for this station definition");
        addProperty(source, "context", "string", "Name of government agency, company, etc. that supplied the data");
        addProperty(source, "name", "string", "Name of how the data was obtained from the context");
        addProperty(source, "stationId", "string", "How does this data source identify this particular station");
        source["required"] = { "context", "stationId" };

        auto& harmonics = addObjectProperty(schema, "harmonics", "Data required for reference tide and current stations");
        addNumberProperty(harmonics, "confidence", "integer", confidenceText);
        addEnumProperty(harmonics, "datum", db.datum_types, get_datum, datumText);
        addNumberProperty(harmonics, "datumOffset", "number", datum_offsetText);
        addNumberProperty(harmonics, "zoneOffset", "integer", zone_offsetText);
        auto& constit = addProperty(harmonics, "constituents", "array", "Harmonic constants that define this reference station");
        auto constitItem = json::object();
        constitItem["type"] = "object";
        addEnumProperty(constitItem, "name", db.constituents, get_constituent, "The harmonic constant being defined. See https://flaterco.com/xtide/harmonics.html");
        addNumberProperty(constitItem, "amp", "number", "The amplitutde in units defined in levelUnits");
        addNumberProperty(constitItem, "epoch", "number", "The epoch, sometimes defined as 'phase'. Should be relative to GMT unless zoneOffset is non-zero");
        constitItem["required"] = { "name", "amp", "epoch" };
        constit["items"] = constitItem;


        auto& offsets = addObjectProperty(schema, "offsets", "Data required for subordinate tide and current stations");
        addNumberProperty(offsets, "ebbBegins", "integer", "For current stations only - the time corrector for the beginning of ebb tide for this station, a.k.a. 'minimum before ebb.'  Use offset hours * 100 + minutes. 2560 represents NULL/unknown");
        addNumberProperty(offsets, "floodBegins", "integer", "For current stations only - the time corrector for the beginning of flood tide for this station, a.k.a. 'minimum before flood.'  Use offset hours * 100 + minutes. 2560 represents NULL/unknown");
        addNumberProperty(offsets, "minLevelAdd", "number", "Offset to add to the low tide value from the reference station");
        addNumberProperty(offsets, "maxLevelAdd", "number", "Offset to add to the high tide value from the reference station");
        addNumberProperty(offsets, "minTimeAdd", "integer", "Offset to add to the time of low tide at the reference station (HHMM)");
        addNumberProperty(offsets, "maxTimeAdd", "integer", "Offset to add to the time of high tide at the reference station (HHMM)");
        addNumberProperty(offsets, "minLevelMultiply", "number", "Multiplier to scale the low tide level from the reference station");
        addNumberProperty(offsets, "maxLevelMultiply", "number", "Multiplier to scale the high tide level from the reference station");
        addNumberProperty(offsets, "referenceStationId", "integer", "The value of the 'id' field for the reference station this station is subordinate to.");


        schema["allOf"] = json::array();

        json combo1;
        combo1["if"]["properties"]["type"]["const"] = "tide";
        combo1["if"]["properties"]["referenceStation"]["const"] = true;
        combo1["then"]["required"] = {  "name", "type", "position", "timezone", "levelUnits", "harmonics", "source" };
        schema["allOf"] += combo1;


        json combo2;
        combo2["if"]["properties"]["type"]["const"] = "tide";
        combo2["if"]["properties"]["referenceStation"]["const"] = false;
        combo2["then"]["required"] = {  "name", "type", "position", "timezone", "levelUnits", "offsets", "source" };
        schema["allOf"] += combo2;


        json combo3;
        combo3["if"]["properties"]["type"]["const"] = "current";
        combo3["if"]["properties"]["referenceStation"]["const"] = true;
        combo3["then"]["required"] = {  "name", "type", "position", "timezone", "levelUnits", "harmonics", "flow", "source" };
        schema["allOf"] += combo3;


        json combo4;
        combo4["if"]["properties"]["type"]["const"] = "current";
        combo4["if"]["properties"]["referenceStation"]["const"] = false;
        combo4["then"]["required"] = {  "name", "type", "position", "timezone", "levelUnits", "offsets", "flow", "source" };
        schema["allOf"] += combo4;


        schema["additionalProperties"] = false;

        close_tide_db();
    }

}




