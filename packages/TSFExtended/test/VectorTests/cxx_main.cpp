//@HEADER
//@HEADER

#include "Teuchos_MPISession.hpp"
#include "TSFVector.hpp"
#include "TSFLinearCombination.hpp"
#include "TSFVectorType.hpp"
#include "TSFVectorSpace.hpp"
#include "TSFEpetraVectorType.hpp"
#include "Teuchos_Time.hpp"

using namespace Teuchos;
using namespace TSFExtended;
using namespace TSFExtendedOps;


int main(int argc, void *argv[]) 
{
  try
    {
      int verbosity = 1;

      MPISession::init(&argc, &argv);

      VectorType<double> type = new EpetraVectorType();

      int n = 20;

      int rank = MPISession::getRank();
      int nProc = MPISession::getNProc();

      int dimension = nProc*n;
      int low = n*rank;
      int high = n*(rank+1);
      std::vector<int> localRows(n);
      for (int i=0; i<n; i++)
        {
          localRows[i] = low + i;
        }

      VectorSpace<double> space = type.createSpace(dimension, n, 
                                                   &(localRows[0]));

      Vector<double> u = space.createMember();
      Vector<double> w = space.createMember();
      Vector<double> v = space.createMember();
      Vector<double> x = space.createMember();
      Vector<double> y = space.createMember();
      Vector<double> z = space.createMember();
      
      for (int i=low; i<high; i++)
        {
          u.setElement(i, i);
          v.setElement(i, i*i);
          w.setElement(i, i*i*i);
          x.setElement(i, ::sqrt(i));
          y.setElement(i, ::cos(i));
          z.setElement(i, ::sin(i));
        }

      if (verbosity > 1)
        {
          cerr << "u = " << endl << u << endl;
          cerr << "v = " << endl << v << endl;
          cerr << "w = " << endl << w << endl;
          
          cerr << "x = " << endl << x << endl;
          cerr << "y = " << endl << y << endl;
          cerr << "z = " << endl << z << endl;
        }

      /* assign into an empty vector */
      Vector<double> a = space.createMember();

      a = x + y + z + u + v + w;
      
      if (verbosity > 1)
        {
          cerr << "a = " << endl << a << endl;
        }

      /* check */
      double err = 0.0;
      for (int i=low; i<high; i++)
        {
          double ai = x.getElement(i) + y.getElement(i) + z.getElement(i)
            +  u.getElement(i) + v.getElement(i) + w.getElement(i);
          err += ::fabs(ai-a.getElement(i));
          if (verbosity > 1)
            {
              cerr << i << " |ai-a[i]| " << ::fabs(ai-a.getElement(i)) << endl;
            }
        }

      
      if (verbosity > 0) cerr << "error norm = " << err << endl;
    }
  catch(std::exception& e)
    {
      cerr << "Caught exception: " << e.what() << endl;
    }
  MPISession::finalize();
}

