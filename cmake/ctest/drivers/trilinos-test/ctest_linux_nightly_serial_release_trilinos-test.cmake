
INCLUDE("${CTEST_SCRIPT_DIRECTORY}/TrilinosCTestDriverCore.trilinos-test.gcc.cmake")
INCLUDE("${CTEST_SCRIPT_DIRECTORY}/../../../TrilinosVersion.cmake")

#
# Set the options specific to this build case
#

SET(COMM_TYPE SERIAL)
SET(BUILD_TYPE RELEASE)
SET(BUILD_DIR_NAME "SERIAL_RELEASE_${Trilinos_VERSION}")
SET(Trilinos_TRACK ${Trilinos_TESTING_TRACK})

SET(Trilinos_BRANCH ${Trilinos_REPOSITORY_BRANCH})

SET(Trilinos_ENABLE_SECONDARY_STABLE_CODE ON)

SET( EXTRA_CONFIGURE_OPTIONS
  "-DTrilinos_ENABLE_EXPLICIT_INSTANTIATION:BOOL=ON"
  "-DTrilinos_DATA_DIR:STRING=$ENV{TRILINOSDATADIRECTORY}"
  "-DTPL_ENABLE_Pthread:BOOL=ON"
  "-DTPL_ENABLE_Boost:BOOL=ON"
  "-DBoost_INCLUDE_DIRS:FILEPATH=/home/trilinos/tpl/gcc4.1.2/boost_1_38_0"
  "-DTPL_ENABLE_ExodusII:BOOL=ON"
  "-DTPL_ENABLE_Nemesis:BOOL=ON"
  "-DNemesis_INCLUDE_DIRS:FILEPATH=/home/trilinos/tpl/gcc4.1.2/nemesis_3.09/include"
  "-DNemesis_LIBRARY_DIRS:FILEPATH=/home/trilinos/tpl/gcc4.1.2/nemesis_3.09/lib"
  )

#
# Set the rest of the system-specific options and run the dashboard build/test
#

TRILINOS_SYSTEM_SPECIFIC_CTEST_DRIVER()
