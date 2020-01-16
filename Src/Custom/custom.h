#ifndef CUSTOM_H
#define CUSTOM_H

#include "../Field/Scalar/scalar.h"
#include "../Matter/matter.h"
#include <vector>

////////////////////////
//                    //
//  Custom functions  //
//                    //
////////////////////////
namespace boil {
  /* droplet parameters */
  void droplet_parameters_2D(const real cang, const real area,
                             real & radius, real & zcent, real & chord);
  void droplet_parameters_3D(const real cang, const real volume,
                             real & radius, real & zcent, real & chord);

  /* scalar error */
  real l2_scalar_error(const Scalar & sca, const Scalar & scb);
  real li_scalar_error(const Scalar & sca, const Scalar & scb);

  /* setup circle */
  void setup_circle_xz(Scalar & c, const real radius, const real xcent, const real zcent);

  /* interpolate velocities */
  void interpolateXZ(const Vector & coarse, Vector & fine, 
                     const Scalar & cs, const Scalar & mdot,
                     const Matter & fluid);

  void interpolate_pressure_solve_2D(
                     real & x, real & y, real & z, real & q,
                     const real & F_x, const real & F_y,
                     const real & F_z, const real & F_q,
                     const real & c_a, const real & c_b,
                     const real & c_c, const real & c_d,
                     const real & Q_a, const real & Q_b,
                     const real & Q_c, const real & Q_d,
                     const real & Q_wb, const real & Q_wt,
                     const real & Q_eb, const real & Q_et,
                     const real & Q_bw, const real & Q_be,
                     const real & Q_tw, const real & Q_te);

  /* restrict scalar */
  void restrictXZ(const Scalar & fine, Scalar & coarse);
  void restrictXZ_area(const Scalar & fine, Scalar & coarse);

  /* restrict vector */
  void restrictXZ_vector_simple(const Vector & fine, Vector & coarse,
                                const Scalar & fs, const Scalar & cs);
}

#endif
