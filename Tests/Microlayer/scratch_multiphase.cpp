      boil::print_line("######################");
      boil::print_line("#                    #");
      boil::print_line("# START FROM SCRATCH #");
      boil::print_line("#                    #");
      boil::print_line("######################");

      /* color */
      real zcent, chord, V0;
      const real xcent = 0.0;
      boil::droplet_parameters_3D(180.-cangle,V0,const_cast<const real&>(radius),zcent,chord);

      boil::setup_circle_xz(conc_fine.color(),radius,xcent,zcent);
      for_avijk(conc_fine.color(),i,j,k)
        conc_fine.color()[i][j][k] = 1. - conc_fine.color()[i][j][k];

      conc_fine.color().bnd_update();
      conc_fine.color().exchange_all();
 
      conc_fine.color_to_vf();
      conc_fine.init();
      
      c.restrict_volume_XZ();
      conc_coarse.init();

      /* temperature */
      for_coarsefine(l) {
        tpr[l] = tout;

        for_vijk(tpr[l],i,j,k) {
          if(tpr[l].domain()->ibody().off(i,j,k)) {
            tpr[l][i][j][k] = twall;
          } else if(c[l][i][j][k]<0.5) { /* almost good */
            tpr[l][i][j][k] = tsat0;
          } else if(tpr[l].zc(k)<=ztconst) {
            tpr[l][i][j][k] = twall + (tout-twall)/ztconst * tpr[l].zc(k);
          }
        }
        tpr[l].bnd_update();
        tpr[l].exchange_all();
      }

      if(case_flag==1) {
        boil::plot->plot(uvw.coarse,c.coarse,tpr.coarse,mdot.coarse,mflx.coarse,
                         "uvw-c-tpr-mdot-mflx",
                         0);
      } else {
        boil::plot->plot(uvw.fine,conc_fine.color(),tpr.fine,mdot.fine,mflx.fine,
                         "uvwf-clrf-tprf-mdot-mflx",
                         0);

        boil::plot->plot(uvw.coarse,c.coarse,press,
                         "uvwc-cc-press",
                         0);
      }
