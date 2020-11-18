  /*------------+
  |  time loop  |
  +------------*/
  bool inertial = time.current_time()<boil::atto;
  Range<real> ml_range_z(0e-6,6e-6);
  auto cap_frac = [&](const real x, const real z,
                      const Range<real> & xr, const Range<real> zr,
                      const real val) {
    if(z>zr.last()||!xr.exists()) {
      return val;
    } else {
      return val*xr.fraction(x);
    }
  };
  auto cap_val  = [&](const real x, const real z, 
                      const Range<real> & xr, const Range<real> zr,
                      const real val0, const real val1) {
    if(z>zr.last()||!xr.exists()) {
      return val1;
    } else {
      return val0+(val1-val0)*xr.fraction(x);
    }
  };

  conc.front_minmax(Range<real>(0.  ,LX1),
                    Range<real>(-LX0,LX0),
                    Range<real>(0.  ,LZ1));
  
  for(time.start(); time.end(); time.increase()) {

#ifdef USE_BOTTOM_DIRICHLET
  #include "update_tpr_bnd.cpp"
#endif

    /* test temperature */
    real tsol_max(-boil::unreal), tsol_min(boil::unreal);
    real tliq_max(-boil::unreal), tliq_min(boil::unreal);
    real tvap_max(-boil::unreal), tvap_min(boil::unreal);
    for_vijk(tpr,i,j,k) {
      real tval = tpr[i][j][k];
      if(tpr.domain()->ibody().on(i,j,k)) {
        if(cht.topo->above_interface(i,j,k)) {
          if(tval>tliq_max)
            tliq_max = tval;
          if(tval<tliq_min)
            tliq_min = tval;
        } else {
          if(tval>tvap_max)
            tvap_max = tval;
          if(tval<tvap_min)
            tvap_min = tval;
        }
      } else {
        if(tval>tsol_max)
          tsol_max = tval;
        if(tval<tsol_min)
          tsol_min = tval;
      }
    }
    boil::cart.max_real(&tsol_max);
    boil::cart.max_real(&tliq_max);
    boil::cart.max_real(&tvap_max);
    boil::cart.min_real(&tsol_min);
    boil::cart.min_real(&tliq_min);
    boil::cart.min_real(&tvap_min);
    boil::oout<<"tprextrema= "<<time.current_time()<<" "
              <<tsol_min<<" "<<tsol_max<<" "
              <<tliq_min<<" "<<tliq_max<<" "
              <<tvap_min<<" "<<tvap_max<<" "
              <<boil::endl;
    if(tliq_min<-0.1||tvap_min<-0.1||tsol_min<-0.1) {
      boil::oout<<"temperature instability. exiting."<<boil::endl;
      iint++;
      boil::plot->plot(uvw,c,tpr,mdot,mflx,press,
                       "uvw-c-tpr-mdot-mflx-press",
                       iint);
      exit(0);
    }

    /*-------------------+
    |  reset body force  |
    +-------------------*/
    for_m(m)
      for_avmijk(xyz,m,i,j,k)
        xyz[m][i][j][k]=0.0;

    /* gravity force */
    Comp m = Comp::w();
    for_vmijk(xyz,m,i,j,k) {
      real phil=std::max(0.0,std::min(1.0,c[i][j][k]));
      real phiv=1.0-phil;
      real deltmp=tpr[i][j][k]-tsat0;
      real rhomix = phil*boil::rho(liquid.rho()->value(),
                                   liquid.beta()->value(),deltmp)
                  + phiv*boil::rho(vapor.rho()->value(),
                                   vapor.beta()->value(),deltmp);
      if(xyz.domain()->ibody().on(m,i,j,k))
        xyz[m][i][j][k] += -gravity * xyz.dV(m,i,j,k) * rhomix;
    }

    /* surface tension */
    conc.tension(&xyz, mixed,conc.color());
    conc.output_cangle_2d(Comp::i(),Comp::k(),Sign::neg());

    /* boundary temperature */
    cht.new_time_step();

    /*---------------+
    |  phase change  |
    +---------------*/
    pc.update();

    real massflow_heat = pc.get_smdot();
    real massflux_heat = massflow_heat/conc.topo->get_totarea();
    real massflux_inert = rhov*sqrt(boil::pi/7.*rhov*latent*deltat_nucl
                                    /rhol/tsat0_K);

    /* transition zone */
    real xmaxbub = conc.topo->get_xmaxft();
    real xmaxml = output_row(ml_range_z.last(),c,true);
    real xtrml = 1.5*xmaxml - 0.5*xmaxbub;
    boil::oout<<"MLranges= "<<time.current_time()<<" "<<xtrml
                            <<" "<<xmaxml<<" "<<xmaxbub<<boil::endl;
    Range<real> ml_range_x(xtrml,xmaxml);

    real massflow_cap(0.0), massflow_ml(0.0);
    real are_cap(0.0), are_ml(0.0);
    for_vijk(c,i,j,k) {
      if(conc.topo->interface(i,j,k)) {
        real a = conc.topo->get_area(i,j,k);
        real a_cap = cap_frac(c.xc(i),
                              c.zc(k),
                              ml_range_x, ml_range_z,
                              a);
        real a_ml = a-a_cap;  

        massflow_cap += mflx[i][j][k]*a_cap;
        massflow_ml += mflx[i][j][k]*a_ml;
        are_cap += a_cap;
        are_ml += a_ml;
      }
    }
    boil::cart.sum_real(&massflow_cap);
    boil::cart.sum_real(&massflow_ml);
    boil::cart.sum_real(&are_cap);
    boil::cart.sum_real(&are_ml);
    
    real massflux_cap(0.0), massflux_ml(0.0);
    if(are_cap>0.0)
      massflux_cap = massflow_cap/are_cap;
    if(are_ml>0.0)
      massflux_ml = massflow_ml/are_ml;

#if 0
    for_vijk(c,i,j,k) {
      if(conc.topo->interface(i,j,k)) {
        mflx[i][j][k] = cap_val(c.xc(i),
                                c.zc(k),
                                ml_range_x, ml_range_z,
                                mflx[i][j][k],
                                massflux_cap);
      }
    }
    mflx.bnd_update();
    mflx.exchange();
    pc.finalize();
#elif 0
    mflx = massflux_heat;
    mflx.bnd_update();
    mflx.exchange();
    pc.finalize();
#endif

    boil::oout<<"mflux= "<<time.current_time()<<" "
                         <<massflux_heat<<" "<<massflux_inert<<" "
                         <<massflux_inert/massflux_heat<<" | "
                         <<massflow_heat<<" "<<massflow_cap<<" "<<massflow_ml<<" | "
                         <<massflux_cap<<" "<<massflux_ml
                         <<boil::endl;

    ns.vol_phase_change(&f);

    /*--------------------------+
    |  solve momentum equation  |
    +--------------------------*/
    /* essential for moving front */
    ns.discretize();
    pr.discretize();
    pr.coarsen();

    /* momentum */
    ns.new_time_step(&f);

    ns.grad(press);
    ns.solve(ResRat(1e-14));

    p = 0.0;
    if(multigrid.cycle(multigrid_cycle0,
                       multigrid_cycle1,
                       multigrid_rt,
                       multigrid_rr,
                       multigrid_mi,
                       multigrid_mstale))
      OMS(converged);

    p.exchange();
    ns.project(p);
    press += p;

    /* shift pressure */
    real pmin=1.0e+300;
    for_vijk(press,i,j,k) {
      if(press.domain()->ibody().on(i,j,k)) {
        if(pmin>press[i][j][k])
          pmin=press[i][j][k];
      }
    }
    boil::cart.min_real(&pmin);

    for_vijk(press,i,j,k) {
      if(press.domain()->ibody().on(i,j,k)) {
        press[i][j][k] -= pmin;
      } else {
        press[i][j][k] = 0.0;
      }
    }

    press.bnd_update();
    press.exchange_all();

    /*---------------------------+
    |  solve transport equation  |
    +---------------------------*/
    conc.new_time_step();
    conc.advance_with_extrapolation(false,ResTol(1e-6),uvw,f,
                                    &liquid,&uvw_1,&vapor,&uvw_2);

    for_avk(c,k) {
      if(c.zc(k)>=(zmax-c.dzc(k))) {
        for_avij(c,i,j) {
          c[i][j][k]= 1.0;
        }
      }
    }

    c.bnd_update();
    c.exchange_all();
    conc.ancillary();
    conc.totalvol();

    /*------------------------+
    |  solve energy equation  |
    +------------------------*/
    enthFD.discretize();
    enthFD.new_time_step();
    enthFD.solve(ResRat(1e-16),"enthFD");

    /*-------------+
    |  dt control  |
    +-------------*/
    /* minimum color function */
    conc.color_minmax();

    /* front */
    conc.front_minmax(Range<real>(0.  ,LX1),
                      Range<real>(-LX0,LX0),
                      Range<real>(0.  ,LZ1));

    time.control_dt(ns.cfl_max(),cfl_limit,dt);

    /*---------------------+
    |  stopping criterion  |
    +---------------------*/
    if(   conc.topo->get_xmaxft()>LX0-dxmin
       || conc.topo->get_zmaxft()>LZ0-dxmin) {
      boil::save_backup(time.current_step(), 1, time,
                        load_scalars, load_scalar_names,
                        load_vectors, load_vector_names);
      boil::rm_backup(ts,
                      load_scalars, load_scalar_names,
                      load_vectors, load_vector_names);

      iint++;
      boil::plot->plot(uvw,c,tpr,mdot,mflx,press,
                       "uvw-c-tpr-mdot-mflx-press",
                       iint);

      /* cell-center velocities */
      Scalar u(d), v(d), w(d);
      boil::cell_center_velocities(uvw,u,v,w);
      boil::save_backup(time.current_step(), 1, time,
                        {&u,&v,&w}, {"u","v","w"});

      break;
    }

    /*--------------+
    |  output data  |
    +--------------*/
    bool otpcond = time.current_time() / t_per_plot >= real(iint);
    if(otpcond) {
      iint++;
      boil::plot->plot(uvw,c,tpr,mdot,mflx,press,
                       "uvw-c-tpr-mdot-mflx-press",
                       iint);

      std::fstream output;
      std::stringstream ssp;
      ssp <<"profile-"<<iint<<".txt";
      output.open(ssp.str(), std::ios::out);
      boil::output_profile_xz(conc.color(),output,Range<int>(NZsol/2+1,NZtot/2),
                              Range<int>(-1,-2),LX1);
      boil::cart.barrier();
      output.close();

      real ml_thickness_max = 10e-6;
      std::stringstream ssm;
      ssm <<"microlayer-"<<iint<<".txt";
      output.open(ssm.str(), std::ios::out);
      boil::output_profile_zx(conc.color(),output,
                              Range<int>(1,NXtot/2),
                              Range<int>(NZsol/2+1,
                                         NZsol/2+ceil(ml_thickness_max/2./DX0)
                                        )
                             );
      boil::cart.barrier();
      output.close();

      std::stringstream ssb;
      ssb <<"bndtpr-"<<iint<<".txt";
      output.open(ssb.str(), std::ios::out);
      if(NZsol>0) {
        boil::output_wall_heat_transfer_xz(cht,output,NXtot/2);
      } else {
        boil::output_wall_heat_transfer_xz(tpr,*(conc.topo),pc,
                                           output,NXtot/2);
      }
      boil::cart.barrier();
      output.close();
    }

    /*--------------+
    |  backup data  |
    +--------------*/
    if(time.current_step() % n_per_backup == 0) {
      boil::save_backup(time.current_step(), 0, time,
                        load_scalars, load_scalar_names,
                        load_vectors, load_vector_names);
    }

    if(boil::timer.current_min() > (wmin-30.0)) {
      boil::save_backup(time.current_step(), 1, time,
                        load_scalars, load_scalar_names,
                        load_vectors, load_vector_names);
      boil::rm_backup(ts,
                      load_scalars, load_scalar_names,
                      load_vectors, load_vector_names);

      boil::set_irun(0);

      break;
    }
  }
