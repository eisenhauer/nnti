/*****************************************************************************/
/* Copyright 1999, Sandia Corporation. The United States Government retains  */
/* a nonexclusive license in this software as prescribed in AL 88-1 and AL   */
/* 91-7. Export of this program may require a license from the United States */
/* Government.                                                               */
/*****************************************************************************/

/*****************************************************************************/
/* Sample driver for AZTEC/ML package. The software is tested by reading in  */
/* a matrix stored in a file called .data, using a zero initial guess        */
/* and a random right hand side, and then solving the system of equations    */
/* using AZTECs gmres solver and ML preconditioner                           */
/*                                                                           */
/* NOTE: the file .data must exist on all processors (though it need only    */
/* contain the number of rows in the matrix on all but the first processor)  */
/*                                                                           */
/* Author:       Dawn Chamberlain, Div 9222, Sandia National Labs            */
/* date:         10/21/99                                                    */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "az_aztec.h"
#include "ml_include.h"
#include <math.h>


extern int AZ_using_fortran;
int    parasails_factorized = 0;
int    parasails_sym        = 0;
double parasails_thresh     = 0.0;
int    parasails_nlevels    = 1;
double parasails_filter     = 0.;
double parasails_loadbal    = 0.;




int main(int argc, char *argv[])
{
	int num_PDE_eqns=1, N_levels=10, nsmooth=1;

	int    leng, level, N_grid_pts, coarsest_level;

  /* See Aztec User's Guide for more information on the */
  /* variables that follow.                             */

  int    proc_config[AZ_PROC_SIZE], options[AZ_OPTIONS_SIZE];
  double params[AZ_PARAMS_SIZE], status[AZ_STATUS_SIZE];

  /* data structure for matrix corresponding to the fine grid */

  int    *data_org = NULL, *update = NULL, *external = NULL;
  int    *update_index = NULL, *extern_index = NULL;
  int    *cpntr = NULL, *bindx = NULL, N_update, iii;
  double *val = NULL, *xxx, *rhs, solve_time, setup_time, start_time;
  AZ_MATRIX *Amat;
  AZ_PRECOND *Pmat = NULL;
  ML *ml;
  FILE *fp;
  int ch,i, j, Nrigid, *garbage, nblocks, *blocks;
  struct AZ_SCALING *scaling;
  ML_Aggregate *ag;
double *mode, *rigid;
char filename[80];
double alpha;
int    one = 1;



#ifdef ML_MPI
  MPI_Init(&argc,&argv);

  /* get number of processors and the name of this processor */

  AZ_set_proc_config(proc_config, MPI_COMM_WORLD);
#else
  AZ_set_proc_config(proc_config, AZ_NOT_MPI);
#endif

  /* read in the number of matrix equations */
  leng = 0;
  if (proc_config[AZ_node] == 0) {
#    ifdef binary
	fp=fopen(".data","rb");
#    else
	fp=fopen(".data","r");
#    endif
     if (fp==NULL) {
        printf("couldn't open file .data\n");
        exit(1);
     }
#    ifdef binary
        fread(&leng, sizeof(int), 1, fp);
#    else
        fscanf(fp,"%d",&leng);
#    endif
     fclose(fp);
  }
  leng = AZ_gsum_int(leng, proc_config);

  N_grid_pts=leng/num_PDE_eqns;



  /* initialize the list of global indices. NOTE: the list of global */
  /* indices must be in ascending order so that subsequent calls to  */
  /* AZ_find_index() will function properly. */
	
  AZ_read_update(&N_update, &update, proc_config, N_grid_pts, num_PDE_eqns,
                 AZ_linear);
	
	
  AZ_read_msr_matrix(update, &val, &bindx, N_update, proc_config);

  AZ_transform(proc_config, &external, bindx, val,  update, &update_index,
	       &extern_index, &data_org, N_update, 0, 0, 0, &cpntr,
	       AZ_MSR_MATRIX);
	
  Amat = AZ_matrix_create( leng );
  AZ_set_MSR(Amat, bindx, val, data_org, 0, NULL, AZ_LOCAL);

  Amat->matrix_type  = data_org[AZ_matrix_type];
	
  data_org[AZ_N_rows]  = data_org[AZ_N_internal] + data_org[AZ_N_border];
			
  start_time = AZ_second();

  options[AZ_scaling] = AZ_none;
  ML_Create(&ml, N_levels);
			
			
  /* set up discretization matrix and matrix vector function */
	
  AZ_ML_Set_Amat(ml, N_levels-1, N_update, N_update, Amat, proc_config);
	
  ML_Aggregate_Create( &ag );
ML_Aggregate_Set_CoarsenScheme_MIS(ag);
ML_Aggregate_Set_Threshold(ag, 0.0);
ML_Aggregate_Set_MaxCoarseSize( ag, 300);


  /* read in the rigid body modes */

   Nrigid = 0;
   if (proc_config[AZ_node] == 0) {
      sprintf(filename,"rigid_body_mode%d",Nrigid+1);
      while( (fp = fopen(filename,"r")) != NULL) {
          fclose(fp);
          Nrigid++;
          sprintf(filename,"rigid_body_mode%d",Nrigid+1);
      }
    }
    Nrigid = AZ_gsum_int(Nrigid,proc_config);

    if (Nrigid != 0) {
       rigid = (double *) ML_allocate( sizeof(double)*Nrigid*(N_update+1) );
       if (rigid == NULL) {
          printf("Error: Not enough space for rigid body modes\n");
       }
    }

    rhs=(double *)malloc(leng*sizeof(double));
    AZ_random_vector(rhs, data_org, proc_config);

    for (i = 0; i < Nrigid; i++) {
           sprintf(filename,"rigid_body_mode%d",i+1);
           AZ_input_msr_matrix(filename, update, &mode, &garbage, N_update, proc_config);

           /*
            *  Rescale matrix/rigid body modes and checking 
            *
             AZ_sym_rescale_sl(mode, Amat->data_org, options, proc_config, scaling);
             Amat->matvec(mode, rigid, Amat, proc_config);
             for (j = 0; j < N_update; j++) printf("this is %d %e\n",j,rigid[j]);
           */

           for (j = 0; j < i; j++) {
              alpha = -AZ_gdot(N_update, mode, &(rigid[j*N_update]), proc_config)/
                       AZ_gdot(N_update, &(rigid[j*N_update]), &(rigid[j*N_update]), 
                               proc_config);
              daxpy_(&N_update, &alpha,  &(rigid[j*N_update]),  &one, mode, &one);
           }
   
           /* rhs orthogonalization */

           alpha = -AZ_gdot(N_update, mode, rhs, proc_config)/
                    AZ_gdot(N_update, mode, mode, proc_config);
           daxpy_(&N_update, &alpha,  mode,  &one, rhs, &one);

           for (j = 0; j < N_update; j++) rigid[i*N_update+j] = mode[j];
           free(mode);
           free(garbage);
  }

  for (j = 0; j < Nrigid; j++) {
     alpha = -AZ_gdot(N_update, rhs, &(rigid[j*N_update]), proc_config)/
              AZ_gdot(N_update, &(rigid[j*N_update]), &(rigid[j*N_update]), proc_config);
     daxpy_(&N_update, &alpha,  &(rigid[j*N_update]),  &one, rhs, &one);
  }

  if (Nrigid != 0) {
     ML_Aggregate_Set_NullSpace(ag, num_PDE_eqns, Nrigid, rigid, N_update);
  }





   coarsest_level = ML_Gen_MGHierarchy_UsingAggregation(ml, N_levels-1, ML_DECREASING, 
                                                        ag);
   coarsest_level = N_levels - coarsest_level;
   if ( proc_config[AZ_node] == 0 )
	printf("Coarse level = %d \n", coarsest_level);
	
   /* set up smoothers */
	
   for (level = N_levels-1; level > coarsest_level; level--) {
		
      ML_Gen_Smoother_ParaSails(ml , level, ML_PRESMOOTHER, nsmooth, parasails_sym, 
                                parasails_thresh, parasails_nlevels, parasails_filter);
/*
      ML_Gen_Smoother_SymGaussSeidel(ml , level, ML_PRESMOOTHER, nsmooth,1.);
      ML_Gen_Smoother_SymGaussSeidel(ml , level, ML_POSTSMOOTHER, nsmooth,1.);
*/

      /*   
      nblocks = ML_Aggregate_Get_AggrCount( ag, level );
      ML_Aggregate_Get_AggrMap( ag, level, &blocks);
      ML_Gen_Smoother_VBlockGaussSeidel(ml , level, ML_PRESMOOTHER, nsmooth,
                                        nblocks, blocks);
      ML_Gen_Smoother_VBlockGaussSeidel( ml , level, ML_POSTSMOOTHER, nsmooth,
                                                 nblocks, blocks);

      ML_Gen_Smoother_BlockGaussSeidel(ml , level, ML_PRESMOOTHER, nsmooth, 0.67, 
                                       num_PDE_eqns);
      ML_Gen_Smoother_BlockGaussSeidel(ml , level, ML_POSTSMOOTHER, nsmooth, 0.67, 
                                       num_PDE_eqns); 
        Note: num_PDE_eqns needs to be set to 6 after first time through
      */
   }
	
   ML_Gen_CoarseSolverSuperLU( ml, coarsest_level);
		
	
   ML_Gen_Solver(ml, ML_MGV, N_levels-1, coarsest_level); 
   AZ_defaults(options, params);
	
   options[AZ_solver]   = AZ_cg;
   options[AZ_scaling]  = AZ_none;
   options[AZ_precond]  = AZ_user_precond;
   options[AZ_conv]     = AZ_r0;
   options[AZ_output]   = 1;
   options[AZ_max_iter] = 1500;
   options[AZ_poly_ord] = 5;
   options[AZ_kspace]   = 130;
   params[AZ_tol]       = 1.0e-8;
	
   AZ_set_ML_preconditioner(&Pmat, Amat, ml, options); 
   setup_time = AZ_second() - start_time;
	
   xxx = (double *) malloc( leng*sizeof(double));
   rhs=(double *)malloc(leng*sizeof(double));

   for (iii = 0; iii < leng; iii++) xxx[iii] = 0.0; 
	
   /* Set rhs */
 
   fp = fopen("AZ_capture_rhs.dat","r");
   if (fp == NULL) {
      if (proc_config[AZ_node] == 0) printf("taking random vector for rhs\n");
      AZ_random_vector(rhs, data_org, proc_config);
      AZ_reorder_vec(rhs, data_org, update_index, NULL);
   }
   else {
      ch = getc(fp);
      if (ch == 'S') {
         while ( (ch = getc(fp)) != '\n') ;
      }
      else ungetc(ch,fp);
      for (i = 0; i < data_org[AZ_N_internal]+data_org[AZ_N_border]; i++) 
         fscanf(fp,"%lf",&(rhs[i]));
      fclose(fp);
   }

   /* Set x */

   fp = fopen("AZ_capture_init_guess.dat","r");
   if (fp != NULL) {
      ch = getc(fp);
      if (ch == 'S') {
         while ( (ch = getc(fp)) != '\n') ;
      }
      else ungetc(ch,fp);
      for (i = 0; i < data_org[AZ_N_internal]+data_org[AZ_N_border]; i++)
         fscanf(fp,"%lf",&(xxx[i]));
      fclose(fp);
      options[AZ_conv] = AZ_expected_values;
   }

   /* if Dirichlet BC ... put the answer in */

   for (i = 0; i < data_org[AZ_N_internal]+data_org[AZ_N_border]; i++) {
      if ( (val[i] > .99999999) && (val[i] < 1.0000001))
         xxx[i] = rhs[i];      
   }

   fp = fopen("AZ_no_multilevel.dat","r");
   scaling = AZ_scaling_create();
   start_time = AZ_second();
   if (fp != NULL) {
      fclose(fp);
      options[AZ_precond] = AZ_none;
      options[AZ_scaling] = AZ_sym_diag;
      options[AZ_ignore_scaling] = AZ_TRUE;

      options[AZ_keep_info] = 1;
      AZ_iterate(xxx, rhs, options, params, status, proc_config, Amat, NULL, scaling); 

/*
      options[AZ_pre_calc] = AZ_reuse;
      options[AZ_conv] = AZ_expected_values;
      if (proc_config[AZ_node] == 0) 
              printf("\n-------- Second solve with improved convergence test -----\n");
      AZ_iterate(xxx, rhs, options, params, status, proc_config, Amat, NULL, scaling); 
      if (proc_config[AZ_node] == 0) 
              printf("\n-------- Third solve with improved convergence test -----\n");
      AZ_iterate(xxx, rhs, options, params, status, proc_config, Amat, NULL, scaling); 
*/
   }
   else {
      options[AZ_keep_info] = 1;
      AZ_iterate(xxx, rhs, options, params, status, proc_config, Amat, Pmat, scaling); 
      options[AZ_pre_calc] = AZ_reuse;
      options[AZ_conv] = AZ_expected_values;
/*
      if (proc_config[AZ_node] == 0) 
              printf("\n-------- Second solve with improved convergence test -----\n");
      AZ_iterate(xxx, rhs, options, params, status, proc_config, Amat, Pmat, scaling); 
      if (proc_config[AZ_node] == 0) 
              printf("\n-------- Third solve with improved convergence test -----\n");
      AZ_iterate(xxx, rhs, options, params, status, proc_config, Amat, Pmat, scaling); 
*/
   }
   solve_time = AZ_second() - start_time;

   if (proc_config[AZ_node] == 0) 
      printf("Solve time = %e, MG Setup time = %e\n", solve_time, setup_time);

   ML_Aggregate_Destroy(&ag);
   ML_Destroy(&ml);
   AZ_free((void *) Amat->data_org);
   AZ_free((void *) Amat->val);
   AZ_free((void *) Amat->bindx);
   AZ_free((void *) update);
   AZ_free((void *) external);
   AZ_free((void *) extern_index);
   AZ_free((void *) update_index);
   AZ_scaling_destroy(&scaling);
   if (Amat  != NULL) AZ_matrix_destroy(&Amat);
   if (Pmat  != NULL) AZ_precond_destroy(&Pmat);
   free(xxx);
   free(rhs);


#ifdef ML_MPI
  MPI_Finalize();
#endif
	
  return 0;
	
}


