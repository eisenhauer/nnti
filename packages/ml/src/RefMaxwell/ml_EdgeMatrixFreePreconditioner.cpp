#include "ml_config.h"
#include "ml_EdgeMatrixFreePreconditioner.h"
#include "ml_MultiLevelPreconditioner.h"
#include "ml_epetra.h"
#include "ml_epetra_utils.h"
#include "ml_mat_formats.h"
#if defined(HAVE_ML_EPETRA) && defined(HAVE_ML_TEUCHOS)

//hax
#include "EpetraExt_RowMatrixOut.h"
//mucho hax
void cms_residual_check(const char * tag, const Epetra_Operator * op,const Epetra_MultiVector& rhs, const Epetra_MultiVector& lhs);
double cms_compute_residual(const Epetra_Operator * op,const Epetra_MultiVector& rhs, const Epetra_MultiVector& lhs);
extern void Epetra_CrsMatrix_Print(const Epetra_CrsMatrix& A, ostream& os);
extern void MVOUT (const Epetra_MultiVector & A, ostream & os);
extern void IVOUT(const Epetra_IntVector & A, ostream & os);

// ================================================ ====== ==== ==== == =
/* This function does a "view" getrow in an ML_Operator.  This is intended to be
used in a ML_CSR_Matrix to Epetra_CrsMatrix (view) translator.  Inlined for
speed. */
inline void CSR_getrow_view(ML_Operator *M, int row, int *ncols,int **cols, double**vals){
  struct ML_CSR_MSRdata* M_= (struct ML_CSR_MSRdata*)ML_Get_MyGetrowData(M);
  *ncols=M_->rowptr[row+1]-M_->rowptr[row];
  *cols=&M_->columns[M_->rowptr[row]];
  *vals=&M_->values[M_->rowptr[row]];  
}/*end CSR_getrow_view*/


int CSR_getrow_ones(ML_Operator *data, int N_requested_rows, int requested_rows[],
   int allocated_space, int columns[], double values[], int row_lengths[])
{
   register int    *bindx, j;
   int     *rowptr,  row, itemp;
   register double *val;
   struct ML_CSR_MSRdata *input_matrix;
   ML_Operator *mat_in;

   row            = *requested_rows;
   mat_in = (ML_Operator *) data;
   input_matrix = (struct ML_CSR_MSRdata *) ML_Get_MyGetrowData(mat_in);
   rowptr = input_matrix->rowptr;
   itemp = rowptr[row];
   *row_lengths = rowptr[row+1] - itemp;


   if (*row_lengths > allocated_space) {
    ML_avoid_unused_param( (void *) &N_requested_rows);
    return(0);
  }

   bindx  = &(input_matrix->columns[itemp]);
   for (j = 0 ; j < *row_lengths; j++) {
      *columns++ = *bindx++;
   }
   val    = &(input_matrix->values[itemp]);
   for (j = 0 ; j < *row_lengths; j++) {
      *values++  = 1;
   }
   return(1);
}


// ================================================ ====== ==== ==== == = 
ML_Epetra::EdgeMatrixFreePreconditioner::EdgeMatrixFreePreconditioner(const Epetra_Operator_With_MatMat & Operator, const Epetra_Vector& Diagonal, const Epetra_CrsMatrix & D0_Matrix,const Epetra_CrsMatrix & TMT_Matrix, const Teuchos::ParameterList &List,const bool ComputePrec):
  ML_Preconditioner(),Operator_(&Operator),D0_Matrix_(&D0_Matrix),TMT_Matrix_(&TMT_Matrix),Prolongator(0),InvDiagonal_(0),CoarseMatrix(0),CoarsePC(0)
{
  /* Set the Epetra Goodies */
  Comm_ = &(Operator_->Comm());
  printf("[%d] EMFP: Constructor Commencing\n",Comm_->MyPID());

  EdgeDomainMap_ = &(Operator_->OperatorDomainMap());
  EdgeRangeMap_ = &(Operator_->OperatorRangeMap());
  NodeDomainMap_ = &(TMT_Matrix_->OperatorDomainMap());
  NodeRangeMap_ = &(TMT_Matrix_->OperatorRangeMap());
  
  List_=List;
  Label_=strdup("ML edge matrix-free preconditioner");
  InvDiagonal_ = new Epetra_Vector(Diagonal);  
  if(ComputePrec) ML_CHK_ERRV(ComputePreconditioner());
}/*end constructor*/

// ================================================ ====== ==== ==== == =   
ML_Epetra::EdgeMatrixFreePreconditioner::~EdgeMatrixFreePreconditioner(){
  DestroyPreconditioner();
}/*end destructor*/


// ================================================ ====== ==== ==== == = 
// Computes the preconditioner
int ML_Epetra::EdgeMatrixFreePreconditioner::ComputePreconditioner(const bool CheckFiltering)
{
  /* ML Communicator */
  ML_Comm_Create(&ml_comm_);

  /* Parameter List Options */
  int OutputLevel = List_.get("output", 10);
  num_cycles  = List_.get("cycle applications",1);
  ML_Set_PrintLevel(OutputLevel);

  /* Sanity Checking*/
  int OperatorDomainPoints =  OperatorDomainMap().NumGlobalPoints();
  int OperatorRangePoints =  OperatorRangeMap().NumGlobalPoints();
  if (OperatorDomainPoints != OperatorRangePoints)
    ML_CHK_ERR(-1); // only square matrices

  /* Invert non-zeros on the diagonal */
  for (int i = 0; i < InvDiagonal_->MyLength(); ++i)
    if ((*InvDiagonal_)[i] != 0.0)
      (*InvDiagonal_)[i] = 1.0 / (*InvDiagonal_)[i];

  double nrm;
  InvDiagonal_->Norm2(&nrm);
  printf("Inverse Diagonal Norm = %6.4e\n",nrm);
  
  /* Do the eigenvalue estimation for Chebyshev */
  printf("EMFP: Doing Smoother Setup\n");
  ML_CHK_ERR(SetupSmoother());

  /* Build the Nullspace */
  printf("EMFP: Building Nullspace\n");
  Epetra_MultiVector *nullspace=BuildNullspace();
  if(!nullspace) ML_CHK_ERR(-1);
  
  /* Build the prolongator */
  printf("EMFP: Building Prolongator\n");
  ML_CHK_ERR(BuildProlongator(*nullspace));
  
  /* Form the coarse matrix */
  printf("EMFP: Building Coarse Matrix\n");
  ML_CHK_ERR(FormCoarseMatrix());

  /* Setup Preconditioner on Coarse Matrix */
  printf("EMFP: Building Coarse Precond\n");
  CoarsePC = new MultiLevelPreconditioner(*CoarseMatrix,List_);
  if(!CoarsePC) ML_CHK_ERR(-2);


  /* DEBUG: Output matrices */
  ofstream of1("prolongator.dat");
  Epetra_CrsMatrix_Print(*Prolongator,of1);
  ofstream of2("coarsemat.dat");
  Epetra_CrsMatrix_Print(*CoarseMatrix,of2);

  
  
  /* Clean Up */
  delete nullspace;
  return 0;
}/*end ComputePreconditioner*/


// ================================================ ====== ==== ==== == = 
// Setup the Smoother
int ML_Epetra::EdgeMatrixFreePreconditioner::SetupSmoother()
{
  /* Variables */
  double lambda_min = 0.0;
  double lambda_max = 0.0;
  Teuchos::ParameterList IFPACKList;  

  /* Parameter-list Options */
  int PolynomialDegree = List_.get("smoother: degree", 3);
  int MaximumIterations = List_.get("eigen-analysis: max iters", 10);
  string EigenType_ = List_.get("eigen-analysis: type", "cg");
  double boost = List_.get("eigen-analysis: boost for lambda max", 1.0);
  double omega_ = List_.get("smoother: damping", 1.0);


  ofstream ofs("inv_diagonal.dat");
  MVOUT(*InvDiagonal_,ofs);
  
  /* Do the eigenvalue estimation*/
  if (EigenType_ == "power-method"){   
    ML_CHK_ERR(Ifpack_Chebyshev::PowerMethod(*Operator_,*InvDiagonal_,MaximumIterations,lambda_max));
    lambda_min=lambda_max/30.0;
  }/*end if*/
  else if(EigenType_ == "cg"){    
    ML_CHK_ERR(Ifpack_Chebyshev::CG(*Operator_,*InvDiagonal_,MaximumIterations,lambda_min,lambda_max));
  }/*end else if*/
  else
    ML_CHK_ERR(-1); // not recognized

  /* Setup the Smoother's List*/
  IFPACKList.set("chebyshev: min eigenvalue", lambda_min);
  IFPACKList.set("chebyshev: max eigenvalue", boost * lambda_max);
  IFPACKList.set("chebyshev: operator inv diagonal", InvDiagonal_);
  IFPACKList.set("chebyshev: degree", PolynomialDegree);
  printf("Chebyshev Smoother: lmin/lmax %6.4e/%6.4e\n",lambda_min,lambda_max);//DEBUG
  
  /* Build the Smoother */
  Smoother_ = new Ifpack_Chebyshev(Operator_);
  if (Smoother_ == 0) ML_CHK_ERR(-1); // memory error?
  IFPACKList.set("chebyshev: zero starting solution", true);
  ML_CHK_ERR(Smoother_->SetParameters(IFPACKList));
  ML_CHK_ERR(Smoother_->Initialize());
  ML_CHK_ERR(Smoother_->Compute());

  return 0;
}/*end SetupSmoother */



// ================================================ ====== ==== ==== == = 
// Build the edge nullspace
Epetra_MultiVector * ML_Epetra::EdgeMatrixFreePreconditioner::BuildNullspace()
{
  /* Pull the coordinates from Teuchos */
  double * xcoord=List_.get("x-coordinates",(double*)0);
  double * ycoord=List_.get("y-coordinates",(double*)0);
  double * zcoord=List_.get("z-coordinates",(double*)0);
  dim=(xcoord!=0) + (ycoord!=0) + (zcoord!=0);
  
  /* Sanity Checks */
  if(dim == 0 || (!xcoord && (ycoord || zcoord) || (xcoord && !ycoord && zcoord))){
    cerr<<"Error: Coordinates not defined.  This is *necessary* for the EdgeMatrixFreePreconditioner.\n";
    return 0;
  }/*end if*/
  printf("[%d] BuildNullspace: Pulling %d vectors\n",Comm_->MyPID(),dim);
  
  /* Build the MultiVector */
  double ** d_coords=new double* [3];
  d_coords[0]=xcoord; d_coords[1]=ycoord; d_coords[2]=zcoord;
  Epetra_MultiVector e_coords(View,*NodeDomainMap_,d_coords,dim);

  /* Build the Nullspace */
  Epetra_MultiVector * nullspace=new Epetra_MultiVector(*EdgeDomainMap_,dim,false);  
  D0_Matrix_->Multiply(false,e_coords,*nullspace);  

  /* Cleanup */
  delete [] d_coords ;
  return nullspace;
}/*end BuildNullspace*/




// ================================================ ====== ==== ==== == = 
//! Build the edge-to-vector-node prolongator described in Bochev, Hu, Siefert and Tuminaro (2006).
int ML_Epetra::EdgeMatrixFreePreconditioner::BuildProlongator(const Epetra_MultiVector & nullspace)
{
  // NTS: Sadly, there's no easy way to do aggregation w/o coarsening in ML.  So
  // we must do a tad bit of extra work here.  The code here was largely horked
  // from ml_MatrixFreePreconditioner.cpp

  /* Wrap TMT_Matrix in a ML_Operator */
  ML_Operator* TMT_ML = ML_Operator_Create(ml_comm_);
  ML_Operator_WrapEpetraCrsMatrix((Epetra_CrsMatrix*)TMT_Matrix_,TMT_ML);

  /* Pull Teuchos Options */
  string CoarsenType = List_.get("aggregation: type", "Uncoupled");
  double Threshold   = List_.get("aggregation: threshold", 0.0);  
  int    NodesPerAggr = List_.get("aggregation: nodes per aggregate", 
                                  ML_Aggregate_Get_OptimalNumberOfNodesPerAggregate());

  /* Setup the Aggregation */
  printf("[%d] EMFP: Building aggregates\n",Comm_->MyPID());
  ML_Aggregate_Struct * MLAggr;
  ML_Aggregate_Create(&MLAggr);
  ML_Aggregate_Set_MaxLevels(MLAggr, 2);
  ML_Aggregate_Set_StartLevel(MLAggr, 0);
  ML_Aggregate_Set_Threshold(MLAggr, Threshold);
  MLAggr->cur_level = 0;
  ML_Aggregate_Set_Reuse(MLAggr);
  MLAggr->keep_agg_information = 1;  
  ML_Operator *P = ML_Operator_Create(ml_comm_);

  /* Process Teuchos Options */
  if (CoarsenType == "Uncoupled")  MLAggr->coarsen_scheme = ML_AGGR_UNCOUPLED;
  else if (CoarsenType == "METIS"){
    MLAggr->coarsen_scheme = ML_AGGR_METIS;
    ML_Aggregate_Set_NodesPerAggr(0, MLAggr, 0, NodesPerAggr);
  }/*end if*/
  else {ML_CHK_ERR(-1);}

  /* Aggregate Nodes */
  int NumAggregates = ML_Aggregate_Coarsen(MLAggr, TMT_ML, &P, ml_comm_);
  if (NumAggregates == 0){
    cerr << "Found 0 aggregates, perhaps the problem is too small." << endl;
    ML_CHK_ERR(-2);
  }/*end if*/
  else printf("[%d] EMFP: %d aggregates created %d invec_leng=%d\n",Comm_->MyPID(),NumAggregates,P->invec_leng);


  /* DEBUG: Dump aggregates */
  Epetra_IntVector AGG(View,*NodeDomainMap_,MLAggr->aggr_info[0]);
  ofstream of0("agg.dat");
  IVOUT(AGG,of0);
  


  
  /* Create wrapper to do abs(T) */
  // NTS: Assume D0 has already been reindexed by now.
  printf("[%d] EMFP: abs(T) prewrap\n",Comm_->MyPID());
  ML_Operator* AbsD0_ML = ML_Operator_Create(ml_comm_);
  ML_Operator_WrapEpetraCrsMatrix((Epetra_CrsMatrix*)D0_Matrix_,AbsD0_ML);
  ML_Operator_Set_Getrow(AbsD0_ML,AbsD0_ML->outvec_leng, CSR_getrow_ones);

  /* Form abs(T) * P_n */
  printf("[%d] EMFP: Building abs(T) * P_n\n",Comm_->MyPID());
  ML_Operator* AbsD0P = ML_Operator_Create(ml_comm_);
  ML_2matmult(AbsD0_ML,P,AbsD0P, ML_CSR_MATRIX);

  /* Wrap P_n into Epetra-land */
  printf("[%d] EMFP: Wrapping to PSparse\n",Comm_->MyPID());
  Epetra_CrsMatrix *Psparse;
  Epetra_CrsMatrix_Wrap_ML_Operator(AbsD0P,*Comm_,*EdgeRangeMap_,&Psparse);

  /* DEBUG: output*/
  ofstream of1("psparse.dat");
  Epetra_CrsMatrix_Print(*Psparse,of1);
  ofstream of2("nullspace.dat");
  MVOUT(nullspace,of2);

  /* Build the DomainMap of the new operator*/
  const Epetra_Map & FineColMap = Psparse->ColMap();
  CoarseMap_=new Epetra_Map(-1,NumAggregates*dim,0,*Comm_);

  
  /* Allocate the Prolongator */
  printf("[%d] EMFP: Building Prolongator\n",Comm_->MyPID());
  Prolongator=new Epetra_CrsMatrix(Copy,*EdgeRangeMap_,0);
  int ne1, *idx1, idx2[dim*AbsD0P->max_nz_per_row];
  double *vals1, vals2[dim*AbsD0P->max_nz_per_row];

  for(int i=0;i<Prolongator->NumMyRows();i++){
    Psparse->ExtractMyRowView(i,ne1,vals1,idx1);
    for(int j=0;j<ne1;j++)
      for(int k=0;k<dim;k++) {
        idx2[j*dim+k]=FineColMap.GID(idx1[j])*dim+k;
        //FIX: This works only because there's an implicit linear mapping which
        //we're exploiting.
        if(idx2[j*dim+k]==-1) printf("[%d] ERROR: idx1[j]=%d / idx1[j]*dim+k=%d does not have a GID!\n",Comm_->MyPID(),idx1[j],idx1[j]*dim+k);
        vals2[j*dim+k]= nullspace[k][i] / ne1;  //FIX? is this right????
      }/*end for*/
    Prolongator->InsertGlobalValues(EdgeRangeMap_->GID(i),dim*ne1,vals2,idx2);
  }/*end for*/
  

  /* FillComplete / OptimizeStorage for Prolongator*/
  printf("[%d] EMFP: Optimizing Prolongator\n",Comm_->MyPID());
  Prolongator->FillComplete(*CoarseMap_,*EdgeRangeMap_);
  Prolongator->OptimizeStorage();
  
  /* Cleanup */
  printf("[%d] EMFP: BuildProlongator Cleanup\n",Comm_->MyPID());
  ML_Aggregate_Destroy(&MLAggr);
  ML_Operator_Destroy(&TMT_ML);
  ML_Operator_Destroy(&AbsD0_ML);
  ML_Operator_Destroy(&AbsD0P);
  ML_Operator_Destroy(&P);
  delete Psparse;

  return 0;
}/*end BuildProlongator*/
  



// ================================================ ====== ==== ==== == = 
// Forms the coarse matrix, given the prolongator
int  ML_Epetra::EdgeMatrixFreePreconditioner::FormCoarseMatrix()
{
  ML_Operator *R= ML_Operator_Create(ml_comm_);
  ML_Operator *P= ML_Operator_Create(ml_comm_);
  ML_Operator *CoarseMat_ML = ML_Operator_Create(ml_comm_);
  ML_Operator *Temp_ML = ML_Operator_Create(ml_comm_);
  CoarseMat_ML->data_destroy=free;
  
  /* Build ML_Operator version of Prolongator, Restriction Operator */
  printf("EMFP: Prolongator Prewrap\n");
  ML_CHK_ERR(ML_Operator_WrapEpetraCrsMatrix(Prolongator,P));
  printf("EMFP: Prolongator Transpose\n");
  //NTS: ML_CHK_ERR won't work on this: it returns 1
  ML_Operator_Transpose_byrow(P, R);

  /* Do the A*P */
  printf("EMFP: AP\n");
  Epetra_CrsMatrix *Temp;
  ML_CHK_ERR(Operator_->MatrixMatrix_Multiply(*Prolongator,&Temp));  

  /* Do R * AP */
  printf("EMFP: RAP\n");
  ML_CHK_ERR(ML_Operator_WrapEpetraCrsMatrix(Temp,Temp_ML));
  ML_2matmult(R, Temp_ML,CoarseMat_ML,ML_CSR_MATRIX);
  Epetra_CrsMatrix_Wrap_ML_Operator(CoarseMat_ML,*Comm_,*CoarseMap_,&CoarseMatrix);

  /* Cleanup */
  ML_Operator_Destroy(&P);
  ML_Operator_Destroy(&R);
  ML_Operator_Destroy(&CoarseMat_ML);
  ML_Operator_Destroy(&Temp_ML);
  delete Temp;
  return 0;
}/*end FormCoarseMatrix*/

// ================================================ ====== ==== ==== == = 
// Print the individual operators in the multigrid hierarchy.
void ML_Epetra::EdgeMatrixFreePreconditioner::Print(const char *whichHierarchy)
{
  if(CoarseMatrix) CoarseMatrix->Print(cout);
}/*end Print*/
 

// ================================================ ====== ==== ==== == = 
// Destroys all structures allocated in \c ComputePreconditioner() if the preconditioner has been computed.
int ML_Epetra::EdgeMatrixFreePreconditioner::DestroyPreconditioner(){
  if (ml_comm_) { ML_Comm_Destroy(&ml_comm_); ml_comm_ = 0; }// will need this
  if (Prolongator) {delete Prolongator; Prolongator=0;}
  if (InvDiagonal_) {delete InvDiagonal_; InvDiagonal_=0;}    
  if (CoarseMatrix) {delete CoarseMatrix; CoarseMatrix=0;}
  if (CoarsePC) {delete CoarsePC; CoarsePC=0;}
  if (CoarseMap_) {delete CoarseMap_; CoarseMap_=0;}
  if (Smoother_) {delete Smoother_; Smoother_=0;}
}/*end DestroyPreconditioner*/



// ================================================ ====== ==== ==== == = 
//! Apply the preconditioner to an Epetra_MultiVector X, puts the result in Y
int ML_Epetra::EdgeMatrixFreePreconditioner::ApplyInverse(const Epetra_MultiVector& B, Epetra_MultiVector& X) const{
  /* Sanity Checks */
  int NumVectors=B.NumVectors();
  if (!B.Map().SameAs(*EdgeDomainMap_)) ML_CHK_ERR(-1);
  if (NumVectors != X.NumVectors()) ML_CHK_ERR(-1);

  Epetra_MultiVector r_edge(*EdgeDomainMap_,NumVectors,false);
  Epetra_MultiVector e_edge(*EdgeDomainMap_,NumVectors,false);
  Epetra_MultiVector e_node(*CoarseMap_,NumVectors,false);
  Epetra_MultiVector r_node(*CoarseMap_,NumVectors,false);

  // NTS: Matrices appear to be correct.  Add debugging output in here to see
  // what is breaking.  FIX!!!!!
  double nrm    [NumVectors];
  //  X.Norm2(nrm);printf("Norm 0 %6.4e\n",nrm[0]);      
  for(int i=0;i<num_cycles;i++){    
    /* Pre-smoothing */
    double re0=cms_compute_residual(Operator_,B,X);
    ML_CHK_ERR(Smoother_->ApplyInverse(X,X));
    //cms_residual_check("(1,1)-S1",Operator_,X,Y);//DEBUG
    double re1=cms_compute_residual(Operator_,B,X);

    //    X.Norm2(nrm);printf("Norm 1 %6.4e\n",nrm[0]);
    /* Calculate Residual (r_
       e = b - (S+M+Addon) * x) */
    ML_CHK_ERR(Operator_->Apply(B,r_edge));
    ML_CHK_ERR(r_edge.Update(1.0,B,-1.0));
    //    r_edge.Norm2(nrm);printf("Norm 2 %6.4e\n",nrm[0]);
    
    /* Xfer to coarse grid (r_n = P' * r_e) */
    ML_CHK_ERR(Prolongator->Multiply(true,r_edge,r_node));

    //    r_node.Norm2(nrm);printf("Norm 3 %6.4e\n",nrm[0]);
    
    /* AMG on coarse grid  (e_n = (CoarseMatrix)^{-1} r_n) */
    ML_CHK_ERR(CoarsePC->ApplyInverse(r_node,e_node));

    double rn1=cms_compute_residual(CoarseMatrix,r_node,e_node);

    
    //    cms_residual_check("(1,1)-C ",CoarseMatrix,r_node,e_node);//DEBUG
    //    e_node.Norm2(nrm);printf("Norm 4 %6.4e\n",nrm[0]);

        /* Xfer back to fine grid (e_e = P * e_n) */
    ML_CHK_ERR(Prolongator->Multiply(false,e_node,e_edge));
    //    e_edge.Norm2(nrm);printf("Norm 5 %6.4e\n",nrm[0]);
    
    /* Add in correction (x = x + e_e) */
    ML_CHK_ERR(X.Update(1.0,e_edge,1.0));

    double re2=cms_compute_residual(Operator_,B,X);
    
    //    X.Norm2(nrm);printf("Norm 6 %6.4e\n",nrm[0]);
    
    /* Post-Smoothing*/
    ML_CHK_ERR(Smoother_->ApplyInverse(X,X));

    double re3=cms_compute_residual(Operator_,B,X);
    
    //    X.Norm2(nrm);printf("Norm 7 %6.4e\n",nrm[0]);    
    //    cms_residual_check("(1,1)-S1",Operator_,temp_edge2,Y);//DEBUG    

    if(Comm_->MyPID()==0)
      printf("11 Resid Reduction: [%6.4e] %6.4e / %6.4e / %6.4e / %6.4e\n",rn1,re1/re0,re2/re1,re3/re2,re3/re0);

  }/*end for*/

  
  return 0;
}/*end ApplyInverse*/



#endif
