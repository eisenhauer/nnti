#!/usr/bin/tcsh
cmake -D Trilinos_ENABLE_ALL_PACKAGES:BOOL=OFF \
      -D Trilinos_ENABLE_Teuchos:BOOL=ON \
      -D Trilinos_ENABLE_Shards:BOOL=ON \
      -D Trilinos_ENABLE_Sacado:BOOL=ON \
      -D Trilinos_ENABLE_Epetra:BOOL=ON \
      -D Trilinos_ENABLE_Ifpack:BOOL=ON \
      -D Trilinos_ENABLE_ML:BOOL=ON \
      -D Trilinos_ENABLE_Amesos:BOOL=ON \
      -D Trilinos_ENABLE_AztecOO:BOOL=OFF \
      -D Trilinos_ENABLE_Belos:BOOL=ON \
      -D Trilinos_ENABLE_Phalanx:BOOL=ON \
      -D Trilinos_ENABLE_EXAMPLES:BOOL=OFF \
      -D Trilinos_ENABLE_TESTS:BOOL=OFF \
      -D Phalanx_ENABLE_TESTS:BOOL=ON \
      -D Phalanx_ENABLE_EXAMPLES:BOOL=ON \
      -D TPL_ENABLE_Boost:BOOL=ON \
      -D Boost_INCLUDE_DIRS:FILEPATH="/home/rppawlo/Libs/Boost/boost_1_36_0" \
      -D TPL_ENABLE_CppUnit:BOOL=ON \
      -D CppUnit_INCLUDE_DIRS:FILEPATH="/home/rppawlo/junk/include" \
      -D CppUnit_LIBRARY_DIRS:FILEPATH="/home/rppawlo/junk/lib" \
      -D TPL_ENABLE_ADOLC:BOOL=ON \
      -D ADOLC_INCLUDE_DIRS:FILEPATH="/home/rppawlo/junk/include" \
      -D ADOLC_LIBRARY_DIRS:FILEPATH="/home/rppawlo/junk/lib" \
      -D TPL_ENABLE_TVMET:BOOL=ON \
      -D TVMET_INCLUDE_DIRS:FILEPATH="/home/rppawlo/junk/include" \
      -D CMAKE_CXX_COMPILER:FILEPATH="/home/rppawlo/local/bin/g++" \
      -D CMAKE_C_COMPILER:FILEPATH="/home/rppawlo/local/bin/gcc" \
      -D CMAKE_Fortran_COMPILER:FILEPATH="/home/rppawlo/local/bin/g77" \
      -D CMAKE_CXX_FLAGS:STRING="-O3 -ansi -pedantic -ftrapv -Wall -Wno-long-long" \
      -D CMAKE_EXE_LINKER_FLAGS:STRING="-L/usr/lib -lgfortran" \
      -D HAVE_STRING_H:BOOL=ON \
      -D CMAKE_VERBOSE_MAKEFILE:BOOL=ON \
      -D Trilinos_VERBOSE_CONFIGURE:BOOL=OFF \
      -D CMAKE_SKIP_RULE_DEPENDENCY=ON \
       ../Trilinos
