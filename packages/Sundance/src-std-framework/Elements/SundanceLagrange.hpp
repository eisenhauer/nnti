/* @HEADER@ */
/* @HEADER@ */

#ifndef SUNDANCE_LAGRANGE_H
#define SUNDANCE_LAGRANGE_H

#include "SundanceDefs.hpp"
#include "Teuchos_RefCountPtr.hpp"
#include "SundanceBasisFamilyBase.hpp"

#ifndef DOXYGEN_DEVELOPER_ONLY

namespace SundanceStdFwk
{
  using namespace SundanceUtils;
  using namespace SundanceStdMesh;
  using namespace SundanceStdMesh::Internal;
  using namespace Internal;
  using namespace SundanceCore;
  using namespace SundanceCore::Internal;

  /** */
  class Lagrange : public BasisFamilyBase 
  {
  public:
    /** */
    Lagrange(int order);

    /** */
    virtual ~Lagrange(){;}

    /** */
    virtual XMLObject toXML() const ;

    /** */
    virtual int order() const {return order_;}

    /** */
    virtual RefCountPtr<BasisFamilyBase> getRcp() {return rcp(this);} 

    /** */
    virtual void getLocalDOFs(const CellType& cellType,
                              Array<Array<Array<int> > >& dofs) const ;

    /** */
    virtual void refEval(const CellType& cellType,
                         const Array<Point>& pts,
                         const MultiIndex& deriv,
                         Array<Array<double> >& result) const ;

  private:
    int order_;

    static Array<int> makeRange(int low, int high);

    /** evaluate on a line cell  */
    void evalOnLine(const Point& pt,
                    const MultiIndex& deriv,
                    Array<double>& result) const ;
    
    /** evaluate on a triangle cell  */
    void evalOnTriangle(const Point& pt,
                        const MultiIndex& deriv,
                        Array<double>& result) const ;
  };
}

#endif  /* DOXYGEN_DEVELOPER_ONLY */

#endif
