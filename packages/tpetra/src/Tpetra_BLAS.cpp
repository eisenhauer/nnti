/*Paul
27-May-2002 General cleanup. Checked for newNamingConvention (nothing changed).
06-August-2002 Changed to images, (nothing changed).
*/

#include "Tpetra_BLAS_wrappers.hpp"
#include "Tpetra_ScalarTraits.hpp"

namespace Tpetra {

	template<typename OrdinalType, typename ScalarType>
	class BLAS {
		
	};

	template<>
	class BLAS<OrdinalType, float> {

	};

	template<>
	class BLAS<OrdinalType, double> {

	};

	template<>
	class BLAS<OrdinalType, complex<float> > {

	};

	template<>
	class BLAS<OrdinalType, complex<double> > {

	};


  // GEMM to sgemm shim (float)
  template<> 
  void BLAS<float>::GEMM(char TRANSA, char TRANSB, int M, int N, int K, float ALPHA, float* A, 
												 int LDA, float* B, int LDB, float BETA, float* C, int LDC) const
  {
    sgemm_(&TRANSA, &TRANSB, &M, &N, &K, &ALPHA, A, &LDA, B, &LDB, &BETA, C, &LDC);
  }

  // GEMM to dgemm shim (double)
  template<> 
	void BLAS<double>::GEMM(char TRANSA, char TRANSB, int M, int N, int K, double ALPHA, 
													double* A, int LDA, double * B, int LDB, double BETA, double * C, int LDC) const
  {
    dgemm_(&TRANSA, &TRANSB, &M, &N, &K, &ALPHA, A, &LDA, B, &LDB, &BETA, C, &LDC);
  }
  
  // GEMM generic loopset (other)
  template<class scalarType> 
	void BLAS<scalarType>::GEMM(char TRANSA, char TRANSB, int M, int N, int K, scalarType ALPHA, 
															scalarType* A, int LDA, scalarType* B, int LDB, scalarType BETA, 
															scalarType* C, int LDC) const
	{
    int incra, incca, incrb, inccb;
    
    if (TRANSA=='N')
    {
      incra = LDA; 
      incca = 1;
    }
    else
    {
      incca = LDA; 
      incra = 1;
    }
    if (TRANSB=='N')
    {
      incrb = LDB; 
      inccb = 1;
    }
    else
    {
      inccb = LDB; 
      incrb = 1;
    }
  
    scalarType* curC = C;
    scalarType* curB = B;
    scalarType* curA = A;
    scalarType zero = ScalarTraits<scalarType>::zero();
    scalarType one =  ScalarTraits<scalarType>::one();

    if (BETA==zero) 
      for (int i=0; i<M; i++) 
	C[i] = zero;
    else 
      for (int i=0; i<M; i++) 
	C[i] *= BETA;

    for (int i=0; i<M; i++)
    {
      for (int j=0; j<N; j++)
      {
	scalarType* tmpB = curB;
	scalarType* tmpA = curA;
	scalarType  tmpC = zero;
	for (int l=0; l<K; l++)
	{
	  tmpC += (*tmpA) * (*tmpB); // Note: This is not optimal.  work on it later
	  tmpA += incra;
	  tmpB += inccb;
	}
	*curC += ALPHA * tmpC;
	curC += LDC;
	curB += incrb;
      }
      curA += incca;
      curB = B;
      curC = C+i+1;
    }
    return;
  }
} // namespace Tpetra
