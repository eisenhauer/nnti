#include <string>

#include <stk_percept/function/Function.hpp>
#include <stk_percept/ExceptionWatch.hpp>


namespace stk
{
  namespace percept
  {

    Function::NameToFunctionMap Function::s_nameToFunctionMap;
    unsigned Function::s_integration_order_default = 1;
    unsigned Function::s_spatialDimDefault = 3;  // 3 space, 1 for time
    unsigned Function::s_codomainDimDefault = 1;

    class IdentityFunction : public Function
    {
    public:
      IdentityFunction() : Function("Identity") {}
      using Function::operator();
      virtual void operator()(MDArray& domain, MDArray& codomain, double time )
      {
        codomain.initialize(1.0);
      }
    };

    static IdentityFunction id = IdentityFunction();
    const Function& Function::Identity = id;

    Function::Function(const char *name, 
                       Dimensions domain_dimensions,
                       Dimensions codomain_dimensions,
                       unsigned integration_order) : m_name( (name ? name : "null") ), m_integration_order(integration_order)
    {
      if (domain_dimensions.size() == 0)
        {
          m_domain_dimensions.resize(1); // assume we have input points of dimension 2 or 3
          m_domain_dimensions[0] = s_spatialDimDefault;
        }
      else if (domain_dimensions.size() <= 2)
        {
          m_domain_dimensions = domain_dimensions;
          for (unsigned ii = 0; ii < domain_dimensions.size(); ii++)
            {
              if (domain_dimensions[ii] <= 0)
                {
                  throw new std::runtime_error("Function::Function: domain_dimensions error");
                }
            }
        }
      else
        {
          throw new std::runtime_error("Function::Function: domain_dimensions error");
        }
      if (codomain_dimensions.size() == 0)
        {
          m_codomain_dimensions.resize(1); 
          m_codomain_dimensions[0] = s_codomainDimDefault; 
        }
      else if (codomain_dimensions.size() <= 2)
        {
          m_codomain_dimensions = codomain_dimensions;
          for (unsigned ii = 0; ii < codomain_dimensions.size(); ii++)
            {
              if (codomain_dimensions[ii] <= 0)
                {
                  throw new std::runtime_error("Function::Function: codomain_dimensions error");
                }
            }
        }
      else
        {
          throw new std::runtime_error("Function::Function: codomain_dimensions error");
        }
      
      getNameToFunctionMap()[m_name] = this;
    }

    Function * Function::addAlias(const char *alias)
    {
      const std::string str_alias(alias);
      getNameToFunctionMap()[str_alias] = this;
      return this;
    }

    Function::NameToFunctionMap& Function::getNameToFunctionMap() 
    {
      return s_nameToFunctionMap;
    }

    static std::string join(std::string str1, std::string str2)
    {
      return str1+str2;
    }

    bool Function::argsAreValid(const MDArray& inp, const MDArray& out)
    {
      EXCEPTWATCH;
      VERIFY_OP((unsigned)inp.rank() , >= , m_domain_dimensions.size(), 
                join("Function::argsAreValid inp.rank,dom.size: the input MDArray's rank must be >= to the Function's domain rank.\n Function.name= ",getName()) );
      VERIFY_OP((unsigned)out.rank(), >= , m_codomain_dimensions.size(), 
                join("Function::argsAreValid out.rank,codom.size: the output MDArray's rank must be >= to the Function's codomain rank.\n Function.name= ",getName()) );

      int domain_rank = m_domain_dimensions.size();
      int codomain_rank = m_codomain_dimensions.size();
      int inp_rank = inp.rank();
      int out_rank = out.rank();
      int inp_offset = inp_rank - domain_rank;
      int out_offset = out_rank - codomain_rank;

      // the last dimensions must match
      for (int idomain = 0; idomain < domain_rank; idomain++)
        {
          VERIFY_OP(inp.dimension(idomain+inp_offset), ==,  m_domain_dimensions[idomain], 
                    join("Function::argsAreValid: inp dimensions are inconsistent with function's domain dimensions. \nFunction.name= ",getName()) );
        }
      for (int icodomain = 0; icodomain < codomain_rank; icodomain++)
        {
          VERIFY_OP(out.dimension(icodomain+out_offset), ==,  m_codomain_dimensions[icodomain], 
                    join("Function::argsAreValid: in dimensions are inconsistent with function's codomain dimensions. \nFunction.name= ", getName()) );
        }
      return true;
    }
    
    void Function::setDomainDimensions(const Dimensions dims)
    {
      VERIFY_OP(dims.size(), >, 0, "Function::setDomainDimensions: dims.size() > 0= " );
      VERIFY_OP(dims.size(), <, 3, "Function::setDomainDimensions: dims.size() < 3= " );
      for (unsigned idomain = 0; idomain < dims.size(); idomain++)
        {
          VERIFY_OP(dims[idomain], >=, 1, "Function::setDomainDimensions: dims[idomain] ");
        }
      m_domain_dimensions = dims;
    }

    void Function::setCodomainDimensions(const Dimensions dims)
    {
      VERIFY_OP(dims.size(), >, 0, "Function::setCodomainDimensions: dims.size() > 0= " );
      VERIFY_OP(dims.size(), <, 3, "Function::setCodomainDimensions: dims.size() < 3= " );
      for (unsigned icodomain = 0; icodomain < dims.size(); icodomain++)
        {
          VERIFY_OP(dims[icodomain], >=, 1, "Function::setCodomainDimensions: dims[icodomain] ");
        }
      m_codomain_dimensions = dims;
    }

    std::ostream &operator<<(std::ostream& out,  Function& func)
    {
      out << "Function: " << func.getName() << " domain dims: " << func.getDomainDimensions() << " codomain dims: " << func.getCodomainDimensions();
      return out;
    }

    double eval(double x, double y, double z, double t, Function& func)
    {
      MDArray val(1);
      MDArray pt(3);
      pt(0)=x;
      pt(1)=y;
      pt(2)=z;

      func(pt, val, t);
      return val(0);
    }

    void evalPrint(double x, double y, double z, double t, Function& func)
    {
      MDArray pt(3);
      pt(0)=x;
      pt(1)=y;
      pt(2)=z;
      std::cout << "evalPrint:: pt=\n" << pt << " val= " << eval(x,y,z,t,func) << std::endl;
    }

    MDArray evalVec3(double x, double y, double z, double t, Function& func)
    {
      MDArray pt(3);
      MDArray val(3);
      pt(0)=x;
      pt(1)=y;
      pt(2)=z;
      func(pt, val, t);
      return val;
    }

    void evalVec3Print(double x, double y, double z, double t, Function& func)
    {
      MDArray pt(3);
      pt(0)=x;
      pt(1)=y;
      pt(2)=z;
      std::cout << "evalVec3Print:: pt= \n" << pt << " val= \n" << evalVec3(x,y,z,t,func) << std::endl;
    }

  }
}
