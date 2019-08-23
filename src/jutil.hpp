#ifndef _jutil_hpp
#define _jutil_hpp

//
// Some utility functions for nlohmann/json to allow for "safe" access to properties
// without throwing exceptions. 
// Joel Kozikowski


#include "json_fifo.h"

void setString(char* str, int strSize, json& j, const char* propertyName) {
    if (j.count(propertyName)) {
        strncpy(str, j[propertyName].get<std::string>().c_str(), strSize);
        // Guarantee str is zero terminated...
        str[strSize-1] = 0;
    }
    else {
        str[0] = 0;
    }
}


int getInt(json& j, const char* propertyName) {

    if (j.count(propertyName) && j[propertyName].is_number()) {
        return j[propertyName].get<int>();
    }
    else {
        return 0;
    }

}


double getNum(json& j, const char* propertyName) {

    if (j.count(propertyName) &&  j[propertyName].is_number()) {
        return j[propertyName].get<double>();
    }
    else {
        return 0.0;
    }

}


bool getBool(json& j, const char* propertyName) {

    if (j.count(propertyName) &&  j[propertyName].is_boolean()) {
        return j[propertyName].get<bool>();
    }
    else {
        return false;
    }
}


std::string getStr(json& j, const char* propertyName) {

    if (j.count(propertyName)) {
        return j[propertyName].get<std::string>();
    }
    else {
        return "";
    }
}



#endif