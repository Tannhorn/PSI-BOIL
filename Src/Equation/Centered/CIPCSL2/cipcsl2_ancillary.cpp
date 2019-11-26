#include "cipcsl2.h"

/******************************************************************************/
void CIPCSL2::ancillary() {
/***************************************************************************//**
*  \brief Calculate ancillary vof parameters.
*******************************************************************************/

  boil::timer.start("cipcsl2 ancillary");

  /* wall values extrapolation */
  update_at_walls(phi);

  /*  calculate free surface position */
  heavi->cal_fs_interp(phi,fs);
  fs_bnd_nosubgrid(phi);

  /* normal vector */
  distfunc(phi,24); 
  gradphic(dist);

  /* calculate area density */
  heavi->calculate_adens();

  /* flag the interface */
  interfacial_flagging(phi);

  boil::timer.stop("cipcsl2 ancillary");

  return;
}

