#include "PySundanceFIATQuadratureAdapter.hpp"

using namespace std;

namespace SundanceStdFwk 
{
	FIATQuadratureAdapter::FIATQuadratureAdapter( PyObject *py_quad_factory ,
												  int order ) :
		QuadratureFamilyBase( order ) , pts_(3) , wts_(3)
	{
		
		for (unsigned i=0;i<3;i++) {
			stack<PyObject *> to_decref;
			// call the factory function with shape and order
			// to get the FIAT quadrature rule for that shape

			PyObject *py_quad_rule = 
				PyObject_CallFunction( py_quad_factory , "(ii)" , i+1 , order() );
			TEST_FOR_EXCEPTION( !py_quad_rule , RuntimeError , 
								"Unable to construct quadrature rule" );
			to_decref.push( py_quad_rule );

			PyObject *py_quad_points =
				PyObject_CallMethod( py_quad_rule , "get_points" , NULL );
			
			TEST_FOR_EXCEPTION( !py_quad_points , RuntimeError ,
								"Unable to extract quadrature points" );

			to_decref.push( py_quad_points );

			PyObject *py_quad_weights =
				PyObject_CallMethod( py_quad_rule , "get_weights" , NULL );
		
			
			TEST_FOR_EXCEPTION( !py_quad_weights , RuntimeError , 
								"Unable to extract quadrature weights" );
			
			int num_points = PyObject_Length( py_quad_points );
			
			TEST_FOR_EXCEPTION( (num_points < 1) , RuntimeError ,
								"Not enough points" );
			
			TEST_FOR_EXCEPTION( (num_points != PyObject_Length( py_quad_weights ) ) ,
								RuntimeError ,
								"Number of points and weights don't match" );

			to_decref.push( py_quad_weights );


			// get ready to copy data
			pts_[i].resize( num_points );
			wts_[i].resize( num_points );
			
			for (unsigned j=0;j<num_points;j++) {
				pts_[i][j].resize(i+1);				
			}
			
			// copy data for the points
			// lookup functions into list and tuple are borrowed reference
			// and hence don't need to be decrefed
			for (unsigned j=0;j<num_points;j++) {
				// Get a tuple that is the current point
				PyObject *py_pt_cur = PyList_GetItem( py_quad_points , j );
				for (unsigned k=0;k<i+1;k++) {
					PyObject *py_pt_cur_k = PyTuple_GetItem( py_pt_cur , k );
					pts_[i][j][k] = PyFloat_AsDouble( py_pt_cur_k );
				}
			}
			
			// copy data for weights
			// since they are a Numeric array, I go through the
			// object protocol and have to decref.
			for (unsigned j=0;j<num_points;j++) {
				PyObject *py_j = PyInt_FromLong( (long) j );
				PyObject *py_wt_cur = PyObject_GetItem( py_quad_weights , py_j );
				wts_[i][j] = PyFloat_AsDouble( py_wt_cur );
				to_decref.push( py_j );
				to_decref.push( py_wt_cur );
			}
			
			while( !to_decref.empty() ) {
				PyObject *foo = to_decref.top();
				Py_DECREF( foo );
				to_decref.pop();
			}
		}
	}

	virtual FIATQuadratureAdapter::~FIATQuadratureAdapter()
	{
		for (unsigned i=0;i<3;i++) {
			Py_DECREF( pts_[i] ); Py_DECREF( wts_[i] );
		}
	}
	
	virtual void FIATQuadratureAdapter::getPoints( const CellType& cellType ,
												   Array<Point>& quadPoints,
												   Array<double>& quadWeights ) const
	{
		int cellDim = dimension( cellType );
		const Array<Point>& pts = pts_[cellDim-1];
		const Array<double>& wts = wts_[cellDim-1];
		quadPoints.resize( pts.size() );
		quadWeights.resize( wts.size() );
		for (unsigned i=0;i<pts.size();i++) {
			quadPoints[i].resize( pts[i].size() );
			for (unsigned j=0;j<pts[i].size();j++) {
				quadPoints[i][j] = pts[i][j];
			}
			quadWeights[i] = wts[i];
		}

		return;
	}


}