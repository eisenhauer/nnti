#include "Teuchos_MPISession.hpp"
#include "Teuchos_Error.hpp"

using namespace Teuchos;

int MPISession::rank_ = 0 ;
int MPISession::nProc_ = 1 ;

void MPISession::init(int* argc, void*** argv)
{
#ifdef HAVE_MPI
	/* initialize MPI */
	int mpiHasBeenStarted = 0;
	MPI_Initialized(& mpiHasBeenStarted);
	int mpierr = 0 ;
	if (!mpiHasBeenStarted)
		{
			mpierr = ::MPI_Init (argc, (char ***) argv);
			if (mpierr != 0)
				{
					Error::raise("Error detected in MPI_Init()");
				}
		}
	
	/* find rank */
	mpierr = ::MPI_Comm_rank (MPI_COMM_WORLD, &rank_);
	if (mpierr != 0)
		{
			Error::raise("Error detected in MPI_Comm_rank()");
		}

	/* find number of procs */
	mpierr = ::MPI_Comm_size (MPI_COMM_WORLD, &nProc_);
	if (mpierr != 0)
		{
			Error::raise("Error detected in MPI_Comm_size()");
		}
#endif
}

void MPISession::finalize()
{
#ifdef HAVE_MPI
	int mpierr = ::MPI_Finalize();
	if (mpierr != 0)
		{
			Error::raise("Error detected in MPI_Finalize()");
		}
#endif
}
