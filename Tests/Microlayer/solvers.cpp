  /*-------------------+
  |  time-integration  |
  +-------------------*/
  real dt = surftens_dt_coef*pow(vapor.coarse.rho()->value()*pow(dxmin,3.0)
                / (2.0*boil::pi*mixed.coarse.sigma()->value()),0.5);
  if(case_flag==0) {
    dt = 10.*dxmin;
  }
  boil::oout<<"dxmin= "<<dxmin<<" "<<boil::cart.iam()<<" "<<dt<<"\n";
  Times time(ndt, dt);
  time.set_coef_dec(0.75);
  time.set_dt(dt*initdtcoef);

  /*-----------------+
  |  define solvers  |
  +-----------------*/
  Krylov * solver_coarse = new CG(d.coarse(), Prec::ic2());
  Krylov * solver_fine   = new CG(d.fine()  , Prec::ic2());

  /*-------------------+
  |  define equations  |
  +-------------------*/
  /* momentum equation */
  Momentum ns( uvw.coarse, xyz, time, solver_coarse, &mixed.coarse);
  ns.convection_set(TimeScheme::forward_euler());
  ns.diffusion_set(TimeScheme::backward_euler());

  /* pressure solver */
  Pressure pr(p, f.coarse, uvw.coarse, time, solver_coarse, &mixed.coarse);
  AC multigrid( &pr );
  multigrid.stop_if_diverging(multigrid_stop_if_diverging);
  multigrid.min_cycles(multigrid_min_cycles);
  multigrid.max_cycles(multigrid_max_cycles);

  /* color function */
  VOFaxisym conc_coarse(c.coarse, g.coarse, kappa.coarse, uvw_1, time, solver_coarse);

  conc_coarse.set_curv_method(curv_method);
  conc_coarse.set_topo_method(topo_method);
  conc_coarse.set_use_interp(use_fs_interp);
  conc_coarse.set_pressure_extrapolation_parameters(store_pressure_extrap,niter_pressure_extrap);

  if(subgrid_method) {
    conc_coarse.set_subgrid_method(SubgridMethod::SLICliquid());
  } else {
    conc_coarse.set_subgrid_method(SubgridMethod::None());
  }

  conc_coarse.set_cangle(cangle);
  conc_coarse.set_wall_curv_method(wall_curv_method,Sign::neg(),cangle);
  
  /* vof on the fine grid */
  VOFaxisym conc_fine(c.fine, g.fine, kappa.fine, uvw.fine, time, solver_fine);
  conc_fine.set_topo_method(topo_method);
  conc_fine.set_use_interp(use_fs_interp);

  /* supplementary classes for heat transfer */
  TIF tsat(tsat0);
  HTWallModel * htwm = NULL;
  if(NZheat>0||NZsol==0) {
    htwm = new HTWallModel(HTWallModel::Resistance(resist));
  } else {
    htwm = new HTWallModel(HTWallModel::Full(resist,qflux));
  }

  /* is there conjugate heat transfer? */
  Matter * solid_coarse_ptr = NULL;
  Matter * solid_fine_ptr = NULL;
  if(NZsol>0) {
    solid_coarse_ptr = &solid.coarse;
    solid_fine_ptr = &solid.fine;
  }

  /* enthalpy equation */
  EnthalpyFDaxisym enthFD_coarse(tpr.coarse, q.coarse, uvw.coarse, uvw_1, uvw.coarse,
                                 time, solver_coarse, &mixed.coarse,
                                 conc_coarse.topo, tsat, solid_coarse_ptr, htwm);

  enthFD_coarse.convection_set(TimeScheme::forward_euler());
  enthFD_coarse.diffusion_set(TimeScheme::backward_euler());

  /* enthalpy on the fine grid */
  EnthalpyFDaxisym enthFD_fine(tpr.fine, q.fine, uvw.fine, time, solver_fine, &mixed.fine,
                               conc_fine.topo, tsat, solid_fine_ptr, htwm);

  enthFD_fine.convection_set(TimeScheme::forward_euler());
  enthFD_fine.diffusion_set(TimeScheme::backward_euler());

  /* phase change */
  PhaseChange4 pc_coarse(mdot.coarse, mflx.coarse, tpr.coarse, q.coarse,
                         c.coarse, g.coarse, f.coarse, uvw.coarse, conc_coarse.topo,
                         tsat, time, &mixed.coarse, solid_coarse_ptr,
                         &enthFD_coarse.heat_transfer_wall_model());
  pc_coarse.set_second_order_accuracy(use_second_order_accuracy);
  pc_coarse.set_discard_points_near_interface(discard_points_near_interface);
  pc_coarse.set_unconditional_extrapolation(use_unconditional_extrapolation);

  /* phase change on the fine grid*/
  PhaseChange4 pc_fine(mdot.fine, mflx.fine, tpr.fine, q.fine,
                       c.fine, g.fine , f.fine , uvw.fine, conc_fine.topo,
                       tsat, time, &mixed.fine, solid_fine_ptr,
                       &enthFD_fine.heat_transfer_wall_model());
  pc_fine.set_second_order_accuracy(use_second_order_accuracy);
  pc_fine.set_discard_points_near_interface(discard_points_near_interface);
  pc_fine.set_unconditional_extrapolation(use_unconditional_extrapolation);
