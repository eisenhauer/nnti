#include "Sundance.hpp"

/* 
 * Solve the Bratu problem in 1D using fixed-point iteration 
 */

int main(int argc, char** argv)
{
  try
  {
    int nx = 32;
    double convTol = 1.0e-8;
    double lambda = 0.5;
    Sundance::setOption("nx", nx, "Number of elements");
    Sundance::setOption("tol", convTol, "Convergence tolerance");
    Sundance::setOption("lambda", lambda, "Lambda (parameter in Bratu's equation)");

    Sundance::init(&argc, &argv);

    Out::root() << "Bratu problem (lambda=" << lambda << ")" << endl;
    Out::root() << "Newton's method with automated linearization" 
                << endl << endl;

    VectorType<double> vecType = new EpetraVectorType();

    MeshType meshType = new BasicSimplicialMeshType();
    MeshSource mesher = new PartitionedLineMesher(0.0, 1.0, nx, meshType);
    Mesh mesh = mesher.getMesh();

    CellFilter interior = new MaximalCellFilter();
    CellFilter sides = new DimensionalCellFilter(mesh.spatialDim()-1);
    CellFilter left = sides.subset(new CoordinateValueCellPredicate(0, 0.0));
    CellFilter right = sides.subset(new CoordinateValueCellPredicate(0, 1.0));
    
    BasisFamily basis = new Lagrange(1);
    Expr u = new UnknownFunction(basis, "w");
    Expr v = new TestFunction(basis, "v");

    Expr grad = gradient(1);

    Expr x = new CoordExpr(0);

    const double pi = 4.0*atan(1.0);
    Expr uExact = sin(pi*x);
    Expr R = pi*pi*uExact - lambda*exp(uExact);

    QuadratureFamily quad4 = new GaussianQuadrature(4);
    QuadratureFamily quad2 = new GaussianQuadrature(2);

    DiscreteSpace discSpace(mesh, basis, vecType);
    Expr uPrev = new DiscreteFunction(discSpace, 0.5);

    Expr eqn 
      = Integral(interior, (grad*v)*(grad*u) - v*lambda*exp(u) - v*R, quad4);

    Expr h = new CellDiameterExpr();
    Expr bc = EssentialBC(left+right, v*u/h, quad2); 

    NonlinearProblem prob(mesh, eqn, bc, v, u, uPrev, vecType);

    LinearSolver<double> linSolver 
      = LinearSolverBuilder::createSolver("amesos.xml");

    Out::root() << "Newton iteration" << endl;
    int maxIters = 20;
    Expr soln ;
    bool converged = false;

    LinearOperator<double> J = prob.allocateJacobian();
    Vector<double> residVec = J.range().createMember();
    Vector<double> stepVec;

    for (int i=0; i<maxIters; i++)
    {
      prob.setInitialGuess(uPrev);
      prob.computeJacobianAndFunction(J, residVec);
      
      linSolver.solve(J, -1.0*residVec, stepVec);
      
      double deltaU = stepVec.norm2();
      Out::root() << "Iter=" << setw(3) << i << " ||Delta u||=" << setw(20)
                  << deltaU << endl;
      addVecToDiscreteFunction(uPrev, stepVec);
      if (deltaU < convTol) 
      {
        soln = uPrev;
        converged = true;
        break;
      }
    } 
    TEUCHOS_TEST_FOR_EXCEPTION(!converged, std::runtime_error, 
      "Newton iteration did not converge after " 
      << maxIters << " iterations");
    
    FieldWriter writer = new DSVWriter("AutoLinearizedBratu.dat");
    writer.addMesh(mesh);
    writer.addField("soln", new ExprFieldWrapper(soln[0]));
    writer.write();

    Out::root() << "Converged!" << endl << endl;

    double L2Err = L2Norm(mesh, interior, soln-uExact, quad4);
    Out::root() << "L2 Norm of error: " << L2Err << endl;
    
    Sundance::passFailTest(L2Err, 1.5/((double) nx*nx));
  }
	catch(exception& e) 
  {
    Sundance::handleException(e);
  }
  Sundance::finalize(); 
}

