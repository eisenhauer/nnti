SET(LIB_REQUIRED_DEP_PACKAGES Teuchos)
SET(LIB_OPTIONAL_DEP_PACKAGES Epetra EpetraExt Tpetra Kokkos Belos Ifpack Ifpack2 Amesos Amesos2 ML)
SET(TEST_REQUIRED_DEP_PACKAGES)
SET(TEST_OPTIONAL_DEP_PACKAGES Epetra EpetraExt Tpetra) # TODO: clean up this line
SET(LIB_REQUIRED_DEP_TPLS BLAS LAPACK)
SET(LIB_OPTIONAL_DEP_TPLS)
SET(TEST_REQUIRED_DEP_TPLS)
SET(TEST_OPTIONAL_DEP_TPLS)

#TODO: why there is no EXAMPLE_*_DEP_* ?
