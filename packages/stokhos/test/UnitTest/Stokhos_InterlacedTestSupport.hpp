#ifndef __test_support_hpp__
#define __test_support_hpp__

#include <Teuchos_RCP.hpp>
#include <Teuchos_ParameterList.hpp>

#include "Epetra_MpiComm.h"
#include "Epetra_LocalMap.h"

// FEApp is defined in Trilinos/packages/sacado/example/FEApp
// #include "FEApp_ModelEvaluator.hpp"

#include "Stokhos_Epetra.hpp"
  
Teuchos::RCP<Teuchos::ParameterList> buildAppParams(int num_KL,bool full_expansion);

Teuchos::RCP<const Stokhos::CompletePolynomialBasis<int,double> > buildBasis(int num_KL,int porder);

/*
Teuchos::RCP<EpetraExt::ModelEvaluator> buildFEAppME(int nelem,int num_KL,
                                                     const Teuchos::RCP<const Stokhos::CompletePolynomialBasis<int,double> > & basis,
                                                     const Teuchos::RCP<const Epetra_Comm> & app_comm,
                                                     const Teuchos::RCP<Teuchos::ParameterList> & appParams);
*/

Teuchos::RCP<Stokhos::ParallelData> buildParallelData(bool full_expansion,int num_KL,
                                                      const Teuchos::RCP<const Epetra_Comm> & globalComm,
                                                      const Teuchos::RCP<const Stokhos::CompletePolynomialBasis<int,double> > & basis);

Teuchos::RCP<Stokhos::ParallelData> buildParallelData(bool full_expansion,int num_KL,
                                                      const Teuchos::RCP<const Epetra_Comm> & globalComm,
                                                      const Teuchos::RCP<const Stokhos::CompletePolynomialBasis<int,double> > & basis);

#endif
