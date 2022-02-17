#ifndef _MagLoc_h_
#define _MagLoc_h_

#include <Vector3Txf.h>

/*
For each location, new points added are logged in xx box rotations, so the one with least volume can be used.
As points are added to box, box centre and size are adjusted to contain all added points.

*/

class MagLoc {

public:
  void Init();
  bool FindLocation();
  void StoreLocation(Vector3f vtA, int idxBoxLoc);     // Store location on 'return' received over serial
  void StoreLocation1(Vector3f vtA, int idxBoxLoc);
  void StoreLocation2(Vector3f vtA, int idxBoxLoc);

  void TrackLocations(Vector3f vtA, int idxPath);    // continually track locations for path# 'idxPath'

protected:
  static const int numBoxLocs = 20;    // number of box locations
  static const int numTxf = 5;        // number of box rotations at each location
  Vector3Txf mxBoxTxfs[numTxf];
  Vector3Txf mxBoxInvTxfs[numTxf];

  int idxPathCurr;
  int timeLastStored;
  Vector3f vtPosLastStored;

// for method 1
  Vector3f vtWBoxLocArr[numBoxLocs];
  Vector3f vtLBoxSizesArr2[numBoxLocs][numTxf];

// for method 2
  Vector3f vtLBoxMinArr2[numBoxLocs][numTxf];
  Vector3f vtLBoxMaxArr2[numBoxLocs][numTxf];
  Vector3f vtLBoxCentreArr2[numBoxLocs][numTxf];
  float fBoxVolArr2[numBoxLocs][numTxf];

};



#endif  // _MagLoc_h_
