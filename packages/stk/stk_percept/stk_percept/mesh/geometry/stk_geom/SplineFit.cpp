#include <stk_percept/mesh/geometry/stk_geom/SplineFit.hpp>
#include <iomanip>
#include <iterator>
#include <utility>

namespace stk {
  namespace geom {

    static int s_precision=15;
    std::ostream& operator<<(std::ostream& out,  const Vectors2D& pts)
    {
      out << std::setprecision(s_precision);
      out << "{";
      for (unsigned i = 0; i < pts.size(); i++)
        {
          out << " " << pts[i] << (i < pts.size()-1?", ":"");
          if ((i+1) % 8 == 0) out << "\n";
        }
      out << "}";
      return out;
    }
    std::ostream& operator<<(std::ostream& out,  const Vectors3D& pts)
    {
      out << std::setprecision(s_precision);
      out << "{";
      for (unsigned i = 0; i < pts.size(); i++)
        {
          out << " " << pts[i] << (i < pts.size()-1?", ":"");
          if ((i+1) % 8 == 0) out << "\n";
        }
      out << "}";
      return out;
    }
    std::ostream& operator<<(std::ostream& out,  const Points3D& pts)
    {
      out << std::setprecision(s_precision);
      out << "{";
      for (unsigned i = 0; i < pts.size(); i++)
        {
          out << " " << pts[i] << (i < pts.size()-1?", ":"");
          if ((i+1) % 8 == 0) out << "\n";
        }
      out << "}";
      return out;
    }
    std::ostream& operator<<(std::ostream& out,  const Vector2D& pt)
    {
      out << std::setprecision(s_precision);
      out << " {" << pt[0] << ", " << pt[1] << "}";
      return out;
    }
    std::ostream& operator<<(std::ostream& out,  const Point3D& pt)
    {
      out << std::setprecision(s_precision);
      out << " {" << pt[0] << ", " << pt[1] << ", " << pt[2] << "}";
      return out;
    }
    std::ostream& operator<<(std::ostream& out,  const Vector3D& pt)
    {
      out << std::setprecision(s_precision);
      out << " {" << pt[0] << ", " << pt[1] << ", " << pt[2] << "}";
      return out;
    }
    std::ostream& operator<<(std::ostream& out, const std::vector<double>& vec)
    {
      out << std::setprecision(s_precision);
      out << "{";
      for (unsigned i = 0; i < vec.size(); i++)
        {
          out << " " << vec[i] << (i<vec.size()-1?",  ":"");
          if ((i+1) % 8 == 0) out << "\n";
        }
      out << "}";
      return out;
    }

  }
}

