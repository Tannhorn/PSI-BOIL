#include "cipcsl2.h"

/******************************************************************************/
void CIPCSL2::color_minmax() {
/***************************************************************************//**
*  \brief Detect maximu and minimum of free surface position.
*           results: xminft,xmaxft,yminft,ymaxft,zminft,zmaxft
*******************************************************************************/
  real cmin = +boil::exa;
  real cmax = -boil::exa;

  int i_cmin, j_cmin, k_cmin, i_cmax, j_cmax, k_cmax;
  i_cmin = j_cmin = k_cmin = i_cmax = j_cmax = k_cmax = -1;

#ifdef IB
  for_ijk(i,j,k){
    if ( dom->ibody().on(i,j,k) ) {
      if( cmin > clr[i][j][k] ) {
        cmin = clr[i][j][k];
        i_cmin = i;
        j_cmin = j;
        k_cmin = k; 
      }
      if( cmax < clr[i][j][k] ) {
        cmax = clr[i][j][k];
        i_cmax = i;
        j_cmax = j;
        k_cmax = k; 
      }
    }
  }
#else
  for_ijk(i,j,k){
    if( cmin > clr[i][j][k] ) {
      cmin = clr[i][j][k];
      i_cmin = i;
      j_cmin = j;
      k_cmin = k; 
    }
    if( cmax < clr[i][j][k] ) {
      cmax = clr[i][j][k];
      i_cmax = i;
      j_cmax = j;
      k_cmax = k; 
    }
  }
#endif

  real cminLocal = cmin;
  real cmaxLocal = cmax;

  boil::cart.min_real(&cmin);
  boil::cart.max_real(&cmax);

  set_minval(cmin);
  set_maxval(cmax);

  boil::oout<<"color_minmax: "<<time->current_time()<<" "
            <<cmin<<" "<<cmax<<"\n";

  if (cmin<-1.0 || cmax>2.0) {
    boil::oout<<"cipcsl2_color_minmax: Stop calculation because of";
    boil::oout<<" too small or too large color function.\n";
    if (cminLocal<-1.0 || cmaxLocal>2.0) { 
      boil::aout<<"proc= "<<boil::cart.iam()
        <<" min= "<<cminLocal<<" max= "<<cmaxLocal
        <<" i_min= "<<i_cmin<<" j_min= "<<j_cmin<<" k_min= "<<k_cmin
        <<" i_max= "<<i_cmax<<" j_max= "<<j_cmax<<" k_min= "<<k_cmax<<"\n";
    }
    boil::plot->plot((*u),clr,kappa, "uvw-clr-kappa", time->current_step());
    exit(0);
  }

}
