#include "RBGen_EpetraMVFileIOFactory.h"

namespace RBGen {

  EpetraMVFileIOFactory::EpetraMVFileIOFactory() 
  { 
     // Insert the acceptable input file types for this factory
     file_formats.push_back("Burkardt");
     file_formats.push_back("netCDF");
     file_formats.push_back("Matrix Market");
  }

  Teuchos::RefCountPtr< FileIOHandler< Epetra_MultiVector > >
  EpetraMVFileIOFactory::create( const Teuchos::ParameterList& params )
  {
    if ( !params.isParameter( "File IO Type" ) ) {
      //  TO DO:  THROW EXCEPTION!!!!
    }

    // Get the file format type
    std::string file_format = Teuchos::getParameter<std::string>( const_cast<Teuchos::ParameterList&>(params),
                                                                  "File IO Type" );

    Teuchos::RefCountPtr< FileIOHandler< Epetra_MultiVector > > RBFileIO;

    // File input format based on Burkardt's input files
    if ( file_format == "Burkardt" ) {
      RBFileIO = Teuchos::rcp( new BurkardtFileIOHandler() );
    } else
    // File input format for netCDF files
    if ( file_format == "netCDF" ) {
      RBFileIO = Teuchos::rcp( new netCDFFileIOHandler() );
    } else
    // File input format for Matrix Market files
    if ( file_format == "Matrix Market" ) {
      RBFileIO = Teuchos::rcp( new MatrixMarketFileIOHandler() );
    } else {
	// Throw and exception because the format type is not recognized by this factory
    }
    //
    // Return the method created
    //
    return RBFileIO;
  }
  
} // end of RBGen namespace

