// @HEADER
// ***********************************************************************
// 
// Moocho: Multi-functional Object-Oriented arCHitecture for Optimization
//                  Copyright (2003) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Roscoe A. Bartlett (rabartl@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#include "FortranTypes_f_open_file.hpp"
#include "FortranTypes_CppFortranStrings.hpp"
#include "Teuchos_TestForException.hpp"

typedef FortranTypes::f_int	f_int;

extern "C" {

FORTRAN_FUNC_DECL_UL_(f_int,F_OPEN_FILE,f_open_file) ( const f_int& IUNIT
	, const f_int I_FILE[], const f_int& I_FILE_LEN, const f_int& ISTATUS
	, const f_int& IFORM, const f_int& IBLANK, const f_int& IACCESS
	, const f_int& IRECL );

FORTRAN_FUNC_DECL_UL_(void,F_CLOSE_FILE,f_close_file) ( const f_int& IUNIT, const f_int& KEEP );

}	// end extern "C"

void FortranTypes::f_open_file( const f_int iunit, const char file[]
	, EOpenStatus status, EOpenForm form, EOpenBlank blank
	, EOpenAccess access, f_int recl )
{
	int result;
	// Convert the file name to an integer to pass to Fortran.
	f_int I_FILE[100], I_FILE_LEN;
	result = convert_to_f_int_string( file, I_FILE, &I_FILE_LEN );
	TEST_FOR_EXCEPTION(
		result, InvalidFileNameException
		,"f_open_file(...) : Error, the "
		<< -result << " Character of the file name \""
		<< file << "\" is not a valid ASCII character." );

	if( result = FORTRAN_FUNC_CALL_UL_(F_OPEN_FILE,f_open_file)(iunit, I_FILE, I_FILE_LEN
		, status, form, blank, access, recl ) )
	{
		TEST_FOR_EXCEPTION(
			result < 0, InvalidFileNameException
			,"f_open_file(...) : Error, the "
			<< -result << " Character of the file name \""
			<< file << "\" is not a valid ASCII character." );
		TEST_FOR_EXCEPTION(
			result > 0, OpenException
			,"f_open_file(...) : Error, the file named \""
			<< file << "\" could not be opened and OPEN(...) "
			<< "returned and IOSTAT = " << result );
	}
}

void FortranTypes::f_close_file( const f_int iunit, bool keep )
{
	FORTRAN_FUNC_CALL_UL_(F_CLOSE_FILE,f_close_file)( iunit, keep ? 1 : 0 ); 
}
