Level 0
 Setup Smoother (MueLu::IfpackSmoother{type = point relaxation stand-alone})
 relaxation: type = symmetric Gauss-Seidel   [unused]
 relaxation: sweeps = 1   [unused]
 relaxation: damping factor = 1   [unused]

Level 1
 Prolongator smoothing (MueLu::SaPFactory)
  Build (MueLu::TentativePFactory)
   Build (MueLu::CoupledAggregationFactory)
    Build (MueLu::CoalesceDropFactory)
    lightweight wrap = 1
    aggregation: drop tol = 0   [default]
    aggregation: Dirichlet threshold = 0   [default]
    aggregation: drop scheme = classical   [default]

   [empty list]

   Build (MueLu::AmalgamationFactory)
   [empty list]

   Nullspace factory (MueLu::NullspaceFactory)
   Fine level nullspace = Nullspace

   Build (MueLu::CoarseMapFactory)
   Striding info = {}   [default]
   Strided block id = -1   [default]
   Domain GID offsets = {0}   [default]

  [empty list]

 sa: damping factor = 1.33333   [default]
 sa: calculate eigenvalue estimate = 0   [default]
 sa: eigenvalue estimate num iterations = 10   [default]

 Transpose P (MueLu::TransPFactory)
 [empty list]

 Computing Ac (MueLu::RAPFactory)
 transpose: use implicit = 0   [default]
 Keep AP Pattern = 0   [default]
 Keep RAP Pattern = 0   [default]
 CheckMainDiagonal = 0   [default]
 RepairMainDiagonal = 0   [default]

 Setup Smoother (MueLu::IfpackSmoother{type = point relaxation stand-alone})
 relaxation: type = symmetric Gauss-Seidel   [unused]
 relaxation: sweeps = 1   [unused]
 relaxation: damping factor = 1   [unused]

Level 2
 Prolongator smoothing (MueLu::SaPFactory)
  Build (MueLu::TentativePFactory)
   Build (MueLu::CoupledAggregationFactory)
    Build (MueLu::CoalesceDropFactory)
    lightweight wrap = 1
    aggregation: drop tol = 0   [default]
    aggregation: Dirichlet threshold = 0   [default]
    aggregation: drop scheme = classical   [default]

   [empty list]

   Build (MueLu::AmalgamationFactory)
   [empty list]

   Nullspace factory (MueLu::NullspaceFactory)
   Fine level nullspace = Nullspace

   Build (MueLu::CoarseMapFactory)
   Striding info = {}   [default]
   Strided block id = -1   [default]
   Domain GID offsets = {0}   [default]

  [empty list]

 sa: damping factor = 1.33333   [default]
 sa: calculate eigenvalue estimate = 0   [default]
 sa: eigenvalue estimate num iterations = 10   [default]

 Transpose P (MueLu::TransPFactory)
 [empty list]

 Computing Ac (MueLu::RAPFactory)
 transpose: use implicit = 0   [default]
 Keep AP Pattern = 0   [default]
 Keep RAP Pattern = 0   [default]
 CheckMainDiagonal = 0   [default]
 RepairMainDiagonal = 0   [default]

 Setup Smoother (MueLu::AmesosSmoother{type = Superlu})
 presmoother ->
  [empty list]


 --------------------------------------------------------------------------------
 ---                            Multigrid Summary                             ---
 --------------------------------------------------------------------------------
 Number of levels    = 3
 Operator complexity = 1.44
 Max Coarse Size     = 2000
 Implicit Transpose  = false

 matrix rows    nnz  nnz/row procs
 A 0    9999  29995     3.00  1
 A 1    3333   9997     3.00  1
 A 2    1111   3331     3.00  1

 Smoother (level 0) both : MueLu::IfpackSmoother{type = point relaxation stand-alone}

 Smoother (level 1) both : MueLu::IfpackSmoother{type = point relaxation stand-alone}

 Smoother (level 2) pre  : MueLu::AmesosSmoother{type = Superlu}
 Smoother (level 2) post : no smoother

