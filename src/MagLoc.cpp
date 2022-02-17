#include "MagLoc.h"

#include <Thruster_Config.h>    // for time values

void MagLoc::Init() {

// setup box rotation transforms
//  const float ang45 = asin(1) / 2;
  const float ang45 = M_PI / 4;
  Vector3Txf rotX, rotY, rotZ;
  for (int txf = 0; txf < numTxf; txf++) {    // number of box transforms (5)
    rotX.rotateX(ang45 * (txf & 1));
    rotY.rotateY(ang45 * (txf & 2));
    rotZ.rotateZ(ang45 * (txf & 4));
    mxBoxTxfs[txf] = rotX * (rotY * rotZ);

  // inverse of transform to view points
    rotX.rotateX(-ang45 * (txf & 1));
    rotY.rotateY(-ang45 * (txf & 2));
    rotZ.rotateZ(-ang45 * (txf & 4));
    mxBoxInvTxfs[txf] = rotZ * (rotY * rotX);
  }

}

void MagLoc::StoreLocation(Vector3f vtA, int idxBoxLoc) {
  StoreLocation1(vtA, idxBoxLoc);
  StoreLocation2(vtA, idxBoxLoc);
}

void MagLoc::StoreLocation1(Vector3f vtA, int idxBoxLoc) {
// Method 1 - get point rel centre of rotated boxes first
  if (idxBoxLoc >= numBoxLocs)
    return;
  Vector3f vtWBoxLoc = vtWBoxLocArr[idxBoxLoc];      // box centre in world coords
  Vector3f* vtLBoxSizes = vtLBoxSizesArr2[idxBoxLoc]; // array of box sizes for transforms in local coords
  Vector3f vtWRelBox = vtA - vtWBoxLoc;

  for (int txf = 0; txf < numTxf; txf++) {    // number of box transforms at location
    Vector3f vtLRelBox = mxBoxTxfs[txf] * vtWRelBox;
    // check if point is inside current box
    if (vtLRelBox.isInsideBox(vtLBoxSizes[txf])) {
    }
    vtLRelBox.stretchBox(vtLBoxSizes[txf]);
    float volMethod1 = 8 * vtLBoxSizes[txf].boxVolume();   // vtLBoxSizes is +/- sizes, so 8 segments
  }
} 

void MagLoc::StoreLocation2(Vector3f vtA, int idxBoxLoc) {
// Method 2 - rotate point to box coords first, then compare to box min and max limits

  if (idxBoxLoc >= numBoxLocs)
    return;
  Vector3f* vtLBoxMin = vtLBoxMinArr2[idxBoxLoc];     // array of box minimums transforms in local coords
  Vector3f* vtLBoxMax = vtLBoxMaxArr2[idxBoxLoc];     // array of box minimums transforms in local coords
  Vector3f vtLBoxCentre[numTxf];

  for (int txf = 0; txf < numTxf; txf++) {    // number of box transforms at location
    Vector3f vtLA = mxBoxTxfs[txf] * vtA;

    vtLBoxMin[txf].stretchMin(vtLA);
    vtLBoxMax[txf].stretchMax(vtLA);
    Vector3f vtLDiff = vtLBoxMax[txf] - vtLBoxMin[txf];
    vtLBoxCentre[txf] =  vtLBoxMin[txf] + (vtLDiff * 0.5);
    float volMethod2 = vtLDiff.volume();
  }
}

void MagLoc::TrackLocations(Vector3f vtA, int idxPath) {    // continually track locations for path# 'idxPath'
// store this location and time if move since last point >dx or time >dt

  if (idxPath != idxPathCurr) {
    // reset path

    idxPathCurr = idxPath;
  }

  Vector3f vtDiff = vtA - vtPosLastStored;
  float locDiffSq = vtDiff.magSq();
  int timeDiff = msTime - timeLastStored;
  if ((timeDiff > 100) || (locDiffSq > 4)) {
    // store vtA and time
    
  }
  

}


bool MagLoc::FindLocation() {
//  Vector3 vtFieldTx = magData.vtFieldTx;      // magnitude 0 to 100
  Vector3f vtA;

  // check if inside rotated box
  Vector3f vtCentre, vtFromCentre;
  Vector3f vtBoxBounds;
  Vector3f vtLBoxCoords, vtLBoxSize;
  Vector3Txf mxBoxTxf;
  
  for (int txf = 0; txf < numTxf; txf++) {    // number of box transforms at location
    mxBoxTxf = mxBoxTxfs[txf];
//    vtCentre = ;
    vtFromCentre = vtA - vtCentre;
    bool bInsideWorld = vtFromCentre.isInsideBox(vtBoxBounds);
    if (bInsideWorld) {
      vtLBoxCoords = mxBoxTxf * vtFromCentre;
      bool bInsideLoc = vtLBoxCoords.isInsideBox(vtLBoxSize);
  
    }
  }
  
  return 0;
}
