/* @HEADER@ */
//   
/* @HEADER@ */

#include "PlayaGlobalAnd.hpp"
#include "PlayaEpetraVectorType.hpp"
#include "PlayaSerialVectorType.hpp"
#include "PlayaVectorDecl.hpp"
#include "PlayaVectorSpaceDecl.hpp"
#include "PlayaDefaultBlockVectorSpaceDecl.hpp"
#include "PlayaVectorType.hpp"
#include "PlayaOut.hpp"
#include "PlayaRand.hpp"

#include "Teuchos_GlobalMPISession.hpp"

#include "Teuchos_Time.hpp"
#include "Teuchos_MPIComm.hpp"

using namespace Playa;

Array<Vector<double> > vecMaker(int nVecs,
  int nProc, int rank, const VectorType<double>& vecType)
{
  int n1 = 3;
  int n2 = 4;
  int n3 = 2;
  int n4 = 5;
  int n5 = 6;
  int n6 = 4;

  /* This VS will go out of scope when the function is exited, but
   * its vectors will remember it */
  VectorSpace<double> vs1 
    = vecType.createEvenlyPartitionedSpace(MPIComm::world(), n1);
  VectorSpace<double> vs2 
    = vecType.createEvenlyPartitionedSpace(MPIComm::world(), n2);
  VectorSpace<double> vs3 
    = vecType.createEvenlyPartitionedSpace(MPIComm::world(), n3);
  VectorSpace<double> vs4 
    = vecType.createEvenlyPartitionedSpace(MPIComm::world(), n4);
  VectorSpace<double> vs5 
    = vecType.createEvenlyPartitionedSpace(MPIComm::world(), n5);
  VectorSpace<double> vs6
    = vecType.createEvenlyPartitionedSpace(MPIComm::world(), n6);

  VectorSpace<double> vs 
    = blockSpace(vs1, blockSpace(vs2, blockSpace(vs3, vs4)), 
      blockSpace(vs5, vs6));

  Out::root() << "space = " << vs << endl;

  Rand::setLocalSeed(vs.comm(), 314159);

  Array<Vector<double> > rtn(nVecs);
  for (int i=0; i<rtn.size(); i++)
  {
    rtn[i] = vs.createMember();
    rtn[i].randomize();
  }
  return rtn;
}

bool runTest(int nProc, int rank, const VectorType<double>& vecType)
{
  
  bool pass = true;

  Array<Vector<double> > vecs = vecMaker(2, nProc, rank, vecType);
  Vector<double> x = vecs[0];
  Vector<double> y = vecs[1];
  
  Out::root() << "x=" << endl;
  Out::os() << x << endl;

  Out::root() << "y=" << endl;
  Out::os() << y << endl;

  y = x.copy();

  Out::root() << "after copy: y=" << endl;
  Out::os() << y << endl;

  x.update(-1.0, y);

  
  Out::root() << "x-y=" << endl;
  Out::os() << x << endl;

  double diff = x.norm2();

  Out::root() << "norm2(x)=" << diff << endl;
   
  return pass;
}

int main(int argc, char *argv[])
{
  int stat = 0;
  try
  {
    GlobalMPISession session(&argc, &argv);
    int nProc = session.getNProc();
    int rank = session.getRank();

    BlockIterator<double>::debug() = false;

    VectorType<double> type1 = new EpetraVectorType();
    VectorType<double> type2 = new SerialVectorType();

    bool allPass = true;

    Out::root() << endl << endl;
    Out::root() << "---------- type = " << type1 << " -----------------------"
                << endl;
    allPass = runTest(nProc, rank, type1);


    Out::root() << endl << endl;
    Out::root() << "---------- type = " << type2 << " -----------------------"
                << endl;
    if (rank==0)
    {
      allPass = runTest(1, rank, type2) && allPass;
    }

    allPass = globalAnd(allPass);

    if (!allPass)
    {
        Out::root() << "detected a test that FAILED" << std::endl;
        stat = -1;
    }
    else
    {
        Out::root() << "all tests PASSED" << std::endl;
    }

  }
  catch(std::exception& e)
  {
    std::cerr << "Caught exception: " << e.what() << std::endl;
    stat = -1;
  }
  return stat;
}
