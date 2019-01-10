#include "vof.h"

/******************************************************************************/
real VOF::fext_cut(const int i, const int j, const int k, const real mdotc) {
/***************************************************************************//**
*  \brief Cut off fext
*******************************************************************************/
  real dt=time->dt();
  real clrc =phi [i][j][k];
  clrc = std::min(1.0,std::max(0.0,clrc));

  /* more evaporation than liquid content */
  if(mdotc<-clrc/dt)
    return -clrc/dt;
  else if (mdotc>(1.0-clrc)/dt)
    return (1.0-clrc)/dt;

  return mdotc;
}
