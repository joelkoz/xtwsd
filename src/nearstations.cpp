#include "nearstations.h"
#include "xtutil.h"

using namespace libxtide;


NearStations::NearStations(double lat, double lng, unsigned int maxStations) :
    mine(lat, lng),
    maxStations{maxStations} {

    nodes = new NearStations::Node[maxStations];
    stationCount = 0;
}


NearStations::~NearStations() {
   delete [] nodes;
}


void NearStations::insert(int posNdx) {
   for (int i = stationCount-1; i > posNdx; i--) {
       nodes[i] = nodes[i-1];
   }
}


void NearStations::check(StationRef* pRef) {

    double dist = xtutil::distanceEarth(mine, pRef->coordinates);

    int posNdx = 0;
    while (posNdx < stationCount && 
           dist >= nodes[posNdx].distance) {
        posNdx++;
    } // while

    if (posNdx < stationCount || stationCount < maxStations) {
        // We need to add this ref to the list

        if (stationCount < maxStations) {
            stationCount++;
        }

        insert(posNdx);
        nodes[posNdx].distance = dist;
        nodes[posNdx].pRef = pRef;

    }

}


NearStations::Node* NearStations::operator[](int pos) {

  if (pos < 0 || pos >= stationCount) {
      return NULL;
  }
  
  return &(nodes[pos]);
  
}
