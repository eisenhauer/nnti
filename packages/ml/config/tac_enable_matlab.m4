AC_DEFUN([TAC_ENABLE_MATLAB],
[
AZ_MATLAB_DEFAULT( )  # Default to no matlab support
AZ_MATLAB_EXEC( )     # Check for --with-matlab-exec
AZ_MATLAB_ROOT( )     # Check for --with-matlab-root

AC_MSG_CHECKING(whether we should build the matlab wrappers)
if test -n "$MATLAB_EXE"; then
  BUILD_MATLAB=yes
  AC_MSG_RESULT(yes)
  AM_CONDITIONAL(BUILD_MATLAB, true)

  # Let's include the appropriate MATLAB include directories
  if test -n "$MATLAB_ROOT"; then
    MATLAB_INCLUDES=-I$MATLAB_ROOT/extern/include
    echo "Setting MATLAB_INCLUDES to: $MATLAB_INCLUDES" #debug line
  else 
    MATLAB_INCLUDES=[]

  fi
    AC_SUBST(ML_MATLAB_INCLUDES,[$MATLAB_INCLUDES])
  
  # Cache the old CPPFLAGS, add $MATLAB_INCLUDE  
  save_CPPFLAGS=$CPPFLAGS; CPPFLAGS="$CPPFLAGS $MATLAB_INCLUDES"

  # These checks are based on the relvant counterparts in epetraext.  They've been
  # cleaned up to be in the style of the autoconf 2.5.9
  # Check for engine.h
  AC_CHECK_HEADER([engine.h], [],
	[
	 AC_MSG_RESULT(no)
	 echo "-----"
     	 echo "Cannot find Matlab libeng header file engine.h."
	 echo "Use --with-matlab-root=<DIR> to add the matlab root directory"
         echo "(such that <DIR>/extern/include constains engine.h)"
     	 echo "-----"
     	 AC_MSG_ERROR(Matlab header file engine.h not found)
     	])
	
  # Check for mex.h
  AC_CHECK_HEADER([mex.h], [],  
	[
	 AC_MSG_RESULT(no)
	 echo "-----"
     	 echo "Cannot find Matlab libmx header file mex.h."
	 echo "Use --with-matlab-root=<DIR> to add the matlab root directory "
         echo "(such that <DIR>/extern/include constains mex.h)"
     	 echo "-----"
     	 AC_MSG_ERROR(Matlab header file mex.h not found)
     	])
  # Restore CPPFLAGS
  CPPFLAGS=$save_CPPFLAGS

  # Since there's no reliable way to figure out the location of the mex libraries, let's just not check form them.  
  # If you've managed to get maltab and mex installed and specified the matlab root so we can find mexext and the 
  # mex headers, I'm going to presume that mex can link ok. 
  # TODO: Fix this later

  # Link checks - if these two libraries work, chances are, they all do.
#  save_LIBS=$LIBS

#  AC_CHECK_LIB(mx,mxCreateDoubleMatrix,[],[AC_MSG_ERROR([Cannot find libmx.a library needed for Matlab subpkg, specify using --with-libdirs=-L<DIR> For information on how to specify the proper Matlab headers/libraries see epetraext/doc/matlab.README.])],[-lut -licudata -licui18n -licuio -licuuc])

 # AC_CHECK_LIB(eng,engPutVariable,[],[
 #    AC_CHECK_LIB(eng,engPutArray,[]
 #       [AC_MSG_ERROR([Cannot find libeng.a library needed for Matlab subpkg, specify using --with-libdirs=-L<DIR> For information on how to specify the proper Matlab headers/libraries see epetraext/doc/matlab.README])],[-lut -lmx -lmat -licudata -licui18n -licuio -licuuc])
 #],[-lut -lmx -lmat -licudata -licui18n -licuio -licuuc])

  # Saves us from tryint to link everything into MATLAB
#  LIBS=$save_LIBS	


  # If user specifies prefix, use it for the MATLAB_PREFIX
#  if test "$prefix" != "$ac_default_prefix"; then
#    MATLAB_PREFIX=$prefix
#  fi
#  if test "$exec_prefix" != "$ac_default_prefix"; then
#    MATLAB_EXECPREFIX=$exec_prefix
#  fi

else
  AC_MSG_RESULT(no)
  BUILD_MATLAB=no
  AM_CONDITIONAL(BUILD_MATLAB, false)
fi
])
