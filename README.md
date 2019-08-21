# xtwsd 
Json Web Service for XTide
=======================

---
xtwsd stands for "XTide Web Service Daemon", and provides a json based RESTful API to David Flater's famous [XTide tide prediction software.](https://flaterco.com/xtide/index.html)

Features
-------

* Retrieve tide and current predictions in json and svg format 
* Search for stations closest to a specific lat/long 
* Add new reference and subordinate stations via a Json POST


Requirements
------------
xtwsd is written in C++ versoin 11 and is compiled with CMake and has the following dependencies:

| Project |  Description | Source code |
|---------|----------|-------------|
| XTide   | The primary workhorse of this project, XTide's libxtide library provides the actual prediction calculations and data. | https://flaterco.com/xtide/files.html |
| libtcd  | libtcd is a library that gives access to XTide's low level data (harmonics database) | https://flaterco.com/xtide/files.html |
| nlohmann::json | Excellent json library for C++ | https://github.com/nlohmann/json |
| served | RESTful framework for C++ | https://github.com/meltwater/served |
| boost | C++ library used by served | https://www.boost.org/ |
| openssl | SSL library needed by nos2xt utility | https://www.openssl.org/source/ |

Most of the above projects will be automatically downloaded and built as part of the checkout and build process.  The one exception
being the "Boost" library (which is a dependency of both served as well as the nos2xt utility described later). Boost version 1.70.0
or greater is required.


Building
----------
If you don't already have the Boost library installed in your development environment, download the latest code from https://www.boost.org/
and build the "system" library using the following commands:

```
cd /my/projects
tar zxvf boost_1_70_0.tar.gz
cd boost_1_70_0
./bootstrap.sh -with-libraries=system
./b2
```

By convention, Boost development requires the environment variables **BOOST_ROOT** and **BOOST_LIBRARYDIR** to be set.  Set those
to values appropriate for your system.  Here is an example, though your directories may be different:

```
export BOOST_LIBRARYDIR=/usr/local/boost_1_70_0/libs
export BOOST_ROOT=/usr/local/boost_1_70_0
```


Once Boost is built, you can download and build xtwsd with the following commands:

```
cd /my/projects
git clone --recursive https://github.com/joelkoz/xtwsd.git
mkdir xtwsd/build
cd xtwsd/build
```

On most Linux environments, you configure the build with the following:
```
cmake -Wno-dev ..
```

On Mac OS X, issue the following instead:
```
cmake -Wno-dev -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl ..
```


Once CMake has configured your build environment, you can build the system with a simple *make* command:
```
make
```


Running
-----------
Be sure you have also downloaded a harmonics data file from the [FlaterCo web site](https://flaterco.com/xtide/files.html). Per the XTide
specifications, you must then specify the path where your harmonics file is stored in either the environment variable *HFILE_PATH*, or the file */etc/xtide.conf*.  Finally, run xtwsd with the command:  *xtwsd &lt;port&gt;*

Example:

```
xtwsd 8080
```


nos2xt utility
---------------
The United States National Oceanic and Atmospheric Administration (NOAA) provides public harmonics data via a web service interface for stations it maintains in the USA and parts of the Carribean.  The stations from the USA are part of the harmonics database provided on the FlaterCo web site, but other stations in the Carribean are not.  The utility nos2xt, (source code in src/util) can make calls to the
public web service maintained by NOAA and make web service calls
to your local xtwsd server to add those additional stations.  That code also serves as a good example on how to
use the xtwsd services for adding new data to the server using data you may find in other locations.

As an example, the following command can be run from the "build" directory to add missing Carribean tide stations:

```
./nos2xt -f ../src/util/caribbean.txt -p 8080
```

Type ```nos2xt``` with no parameters to see the options available to you.

KNOWN BUG:
There seems to be an issue with doing "updates" to existing tide prediction data records. The crash occurs somewhere inside the libtcd library of XTide. The data seems to be saved, the the xtwsd server crashes. Restarting it seems to work.  Adding NEW tide
prediction records seems to work fine.  For this reason, the default behavior of nos2xt is to skip tide stations that
are already in the harmonics database.  You can override this behavior with the "-u" option.  


Station Index vs. Station Id
-----------------------------
The XTide project (and in turn, the libxtide libraries) references stations using a number called an "index."  This index
is an index into an internal cache of stations. The biggest problem with using this index to uniquely identify a station is
that is can change when new station data is added to the database. The tcd library keeps stations alphabetized and that
sorting affects the index number.  For this reason, whenever possible, *xtwsd* uses a synthesized id that is in the format ```<source.context>:<source.stationId>```. Each tide and current station record has a "source context" that identifies where
the prediction data came from.  For example "NOS" is the United States' National Oceanic Service, which is a department within
the National Oceanic and Atmosopheric Administration (NOAA). The NOS uses its own system to identify its stations.  For example, 
the station id 8723178 is the station at Government Cut, Miami Florida.  The "Station id" used by *xtwsd* thus would be
```NOS:8723178```.  Using this as an identifier ensures the station is uniquely identified between database updates. 

In the resource paths below, where you see {*stationId*}, you can in fact use either the station Id or the station index. Just
be aware that if you are using xtwsd in an environment where the station data may be added to or updated, the station index
for any given station may change following such updates.


Resource paths
----------------

The following resource paths are available from xtwsd:



### GET /locations&lt;/[tide|current]&gt;&lt;?referenceOnly=[0|1]&gt;

Retrieves a list of every tide or current station in the database. Requesting "/locations" by itself returns every tide and current
station in the database. By specifying the *referenceOnly* parameter, you can limit
the returned list to reference stations only.  Because this list can be very long, you may be more interestd in the */nearest* path described below.

Example:
```
http://127.0.0.1:8080/locations/tide?referenceOnly=1
```


### GET /nearest&lt;/[tide|current]&gt;?lat=*n*&amp;lng=*n*&lt;&amp;count=*n*&gt;&lt;&amp;referenceOnly=[0|1]&gt;

Retrieves a list of the tide or current stations that are closted to the specified latitude and longitude parameters. The parameters
*lat* and *lng* should be specified in *decimal degrees* format (e.g. 28.1234). The five closest stations will be returned unless
the *count* parameter is used to specify a different number. By specifying the *referenceOnly* parameter, you can limit
the returned list to reference stations only.  

Example:
```
http://127.0.0.1:8080/nearest/tide?lat=26.2567&lng=-80.08&count=10&referenceOnly=1
```



### GET /location/{*stationId*}&lt;?start=YYYY-MM-DD HH:MM ZZZ&gt;&lt;&amp;days=*n*&gt;&lt;&amp;local=[1|0]&gt;&lt;&amp;detailed=[1|0]&gt;

Retrieves the tide or current predictions for the specified station. If specified, *start* is the date the predictions will start. If not specified, today's date is used.  If specified, *days* is the number of days beyond *start* to predict (default if not specified is 1). *local* determines if the times returned should be in the same time zone the station is (*local=1*) or GMT (*local=0* or not specified).
If *detailed=1*, predictions will include events such as sunrise, and moonrise, provided they occur within the specified time range. *detailed=0* (or not specified) returns only high and low tide events.

Example
```
http://127.0.0.1:8080/location/NOS:8722862?start=2019-08-15 12:00 am EDT&local=1&detailed=1
```

### GET /graph/{*stationId*}&lt;?start=YYYY-MM-DD HH:MM ZZZ&gt;&lt;&amp;width=*n*&gt;&lt;&amp;height=*n*&gt;

Returns an SVG graph of the tide or current predictions for the specified station, starting at the specified start date. The optional *width* and *height* can be used to specify the size (in pixels) of the returned SVG image.

Example
```
http://127.0.0.1:8080/graph/NOS:8722862?start=2019-08-15 12:00 am EDT&width=1200&height=600
```


### GET /harmonics/{*stationId*}

Retrieves the harmonic constituents for the prediction data for the specified tide or current station. If the station is a subordinate station, the harmonic offsets will be returned instead.

Example
```
http://127.0.0.1:8080/harmonics/NOS:8722862
```


### GET /harmonics/schema

Returns a Json Schema that specifies the required and optional data required to add or update new prediction data. This schema
also covers the data returned by *GET /harmonics{*stationId*}*.

Example
```
http://127.0.0.1:8080/harmonics/schema
```


### POST /harmonics

Allows new prediction data to be added to the database. The Json object sent with the POST request should match the Json Schema
returned by *GET /harmonics/schema*.  If you are updating an existing record, be sure the *index* property is populated. For
adding new records, *index* should be omitted, or set to "-1".  The response to the POST will be a Json object with the following
structure:

```
{
    "statusCode": nn // The HTTP status code (200 = OK, 400 = bad request, 500 = internal error, etc.)
    "index": nnnn // The index of the record that was updated or added
    "message": "Details of any failures"
}
```

Example
```
$ curl -vX POST http://127.0.0.1:8080/harmonics -d @stationdata.json \
--header "Content-Type: application/json"
```

contents of stationdata.json:
```
{
    "comments": "",
    "country": "Bahamas",
    "harmonics": {
        "confidence": 10,
        "constituents": [
            {   
                "name": "M2",
                "amp": 1.329,
                "epoch": 10.6 
            },
            ... see tests/ directory for complete data ...
        ],
        "datum": "Mean Lower Low Water",
        "datumOffset": 1.47,
        "zoneOffset": 0
    },
    "levelUnits": "feet",
    "name": "Settlement Point Bahamas",
    "notes": "",
    "position": {
        "lat": 26.710,
        "long": -78.99666667
    },
    "referenceStation": true,
    "source": {
        "context": "NOS",
        "name": "tidesandcurrents.noaa.gov",
        "stationId": "9710441"
    },
    "timezone": ":America/New_York",
    "type": "tide"
}
```



### GET /tcd

Retrieves basic information about the harmonics data library currently in use.

Example
```
http://127.0.0.1:8080/tcd
```
