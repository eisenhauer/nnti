#include "PDEOptPointData.hpp"
#include "PlayaTabs.hpp"

namespace Sundance
{
using namespace Teuchos;
using namespace Playa;

PointData::PointData(const Array<Point>& locations, 
  const Array<double>& values,
  const double& tol)
  : sensorVals_(), sensorLocations_()
{
  init(locations, values, tol);
}

PointData::PointData(const XMLObject& xml,
  const Mesh& mesh)
  : sensorVals_(), sensorLocations_()
{
  TEST_FOR_EXCEPTION(xml.getTag() != "PointData", RuntimeError,
    "expected tag PointData, found " << xml.getTag());

  double tol = xml.getRequiredDouble("tol");

  Array<Point> locations;
  Array<double> values;

  for (int i=0; i<xml.numChildren(); i++)
  {
    const XMLObject& pt = xml.getChild(i);
    values.append( pt.getRequiredDouble("value") );
    const string& ptStr = pt.getRequired("pt");
    Array<string> tok = StrUtils::stringTokenizer(ptStr);
    Point p;
    if (tok.size()==1)
    {
      p = Point(StrUtils::atof(tok[0]));
    }
    else if (tok.size()==2)
    {
      p = Point(StrUtils::atof(tok[0]),
        StrUtils::atof(tok[1]));
    }
    else if (tok.size()==3)
    {
      p = Point(StrUtils::atof(tok[0]),
        StrUtils::atof(tok[1]),
        StrUtils::atof(tok[2]));
    }
    locations.append(p);
  }

  locations = snapToMesh(mesh, locations);
  init(locations, values, tol);
}


void PointData::init(const Array<Point>& locations, 
  const Array<double>& values,
  const double& tol) 
{
  TEST_FOR_EXCEPTION(locations.size() != values.size(), RuntimeError,
    "inconsistent measurement data: num locations = "
    << locations.size() << " but num readings = " 
    << values.size());

  TEST_FOR_EXCEPTION(locations.size() < 1, RuntimeError,
    "Empty data set in PointData ctor");

  /* make sure all points have the same dimension */
  int dim = locations[0].dim();
  for (int i=0; i<locations.size(); i++)
  {
    TEST_FOR_EXCEPTION(dim != locations[i].dim(), RuntimeError,
      "inconsistent point dimensions in PointData ctor. "
      "points are " << locations);
  }

  CellFilter allPoints = new DimensionalCellFilter(0);
  sensorLocations_ 
    = allPoints.subset(new PointDataCellPredicateFunctor(locations, tol));

  RefCountPtr<UserDefFunctor> op 
    = rcp(new PointDataExprFunctor(locations, values, tol));



  Expr x = new CoordExpr(0);
  Expr y = new CoordExpr(1);
  Expr z = new CoordExpr(2);

  Expr coords;
  if (dim==1) coords = x;
  else if (dim==2) coords = List(x, y);
  else coords = List(x, y, z);

  sensorVals_ = new UserDefOp(coords, op);
}


Array<Point> PointData::snapToMesh(const Mesh& mesh, 
  const Array<Point>& locations)
{
  Array<Point> rtn(locations.size());

  for (int i=0; i<locations.size(); i++)
  {
    rtn[i] = nearestMeshPoint(mesh, locations[i]);
  }
  return rtn;
}

Point PointData::nearestMeshPoint(const Mesh& mesh, const Point& x)
{
  double d2Min = 1.0e100;
  int iPt = 0;
  for (int i=0; i<mesh.numCells(0); i++)
  {
    const Point& p = mesh.nodePosition(i);
    Point dx = x - p;
    double d2 = dx*dx;
    if (d2 < d2Min)
    {
      d2Min = d2;
      iPt = i;
    }
  }
  return mesh.nodePosition(iPt);
}



PointDataExprFunctor::PointDataExprFunctor(const Array<Point>& locations, 
  const Array<double>& values,
  const double& tol)
  : PointwiseUserDefFunctor0("pointData", locations[0].dim(), 1), pointToValueMap_(tol),
    dim_(locations[0].dim())
{
  Tabs tabs;
  for (int i=0; i<locations.size(); i++)
  {
    TEST_FOR_EXCEPTION(pointToValueMap_.find(locations[i]) != pointToValueMap_.end(),
      RuntimeError,
      "Data set contains duplicate point " << locations[i]
      << " to within tolerance "
      << tol << ". Points are "
      << locations);
    pointToValueMap_[locations[i]] = values[i];

    TEST_FOR_EXCEPTION(pointToValueMap_.find(locations[i]) == pointToValueMap_.end(),
      InternalError,
      "Bad map integrity in PointDataExprFunctor ctor: point "
      << locations[i] << " could not be recovered from map");
  }
}

void PointDataExprFunctor::eval0(const double* in, double* out) const
{
  Point p;
  if (dim_==1) p = Point(in[0]);
  else if (dim_==2) p = Point(in[0], in[1]);
  else p = Point(in[0], in[1], in[2]);
  
  typedef std::map<Point, double, SloppyPointComparitor>::const_iterator iter;
  iter pos = pointToValueMap_.find(p);
  TEST_FOR_EXCEPTION(pos == pointToValueMap_.end(), RuntimeError,
    "Point " << p << " not found in list of data values");


  Tabs tabs;

  *out = pos->second;
}
 

PointDataCellPredicateFunctor
::PointDataCellPredicateFunctor(const Array<Point>& locations,
  const double& tol)
  : pointSet_(SloppyPointComparitor(tol))
{
  for (int i=0; i<locations.size(); i++)
  {
    pointSet_.insert(locations[i]);
  }
}

bool PointDataCellPredicateFunctor::operator()(const Point& x) const
{
  Tabs tabs;
  bool rtn = pointSet_.find(x) != pointSet_.end();
  return rtn;
}
 
 
bool SloppyPointComparitor::operator()(const Point& p1, const Point& p2) const
{
  /* first, compare dimensions */
  if (p1.dim() < p2.dim()) return true;
  if (p2.dim() < p1.dim()) return false;

  /* if the dimensions are the same, do lexigraphic ordering on the
   * coordinate directions */
  for (int i=0; i<p1.dim(); i++)
  {
    if (p1[i] < p2[i] - tol_) return true;
    if (p1[i] > p2[i] + tol_) return false;
  }
  return false;
}

}
