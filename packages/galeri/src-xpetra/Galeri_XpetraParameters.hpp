// @HEADER
//
// ***********************************************************************
//
//           Galeri: Finite Element and Matrix Generation Package
//                 Copyright (2006) ETHZ/Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact
//                    Jeremie Gaidamour (jngaida@sandia.gov)
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#ifndef GALERI_XPETRAPARAMETERS_HPP
#define GALERI_XPETRAPARAMETERS_HPP

#include <Teuchos_CommandLineProcessor.hpp>
#include <Teuchos_Describable.hpp>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_VerboseObject.hpp>

namespace Galeri {

  namespace Xpetra {

    // TODO nx/ny/nz == GO or global_size_t ? But what is the best to do?

    template<typename GO>
    class Parameters : public Teuchos::VerboseObject<Parameters<GO> >, public Teuchos::Describable {
    public:

      Parameters(Teuchos::CommandLineProcessor& clp, GO nx = 16, GO ny = -1, GO nz = -1, const std::string& matrixType = "Laplace1D",
                 int keepBCs = 0, double stretchx = 1.0, double stretchy = 1.0, double stretchz = 1.0, double h = 1.0, double delta = 0.0,
                 int PMLXL = 0, int PMLXR = 0, int PMLYL = 0, int PMLYR = 0, int PMLZL = 0, int PMLZR = 0,
                 double omega = 2.0*M_PI, double shift = 0.5, int mx = -1, int my = -1, int mz = -1, int model = 0)
	: nx_(nx), ny_(ny), nz_(nz), mx_(mx), my_(my), mz_(mz), stretchx_(stretchx), stretchy_(stretchy), stretchz_(stretchz), matrixType_(matrixType), keepBCs_(keepBCs),
      h_(h), delta_(delta), PMLx_left(PMLXL), PMLx_right(PMLXR), PMLy_left(PMLYL), PMLy_right(PMLYR), PMLz_left(PMLZL), PMLz_right(PMLZR),
      omega_(omega), shift_(shift), model_(model) {
        clp.setOption("nx",         &nx_,           "mesh points in x-direction.");
        clp.setOption("ny",         &ny_,           "mesh points in y-direction.");
        clp.setOption("nz",         &nz_,           "mesh points in z-direction.");
        clp.setOption("mx",         &mx_,           "processors in x-direction.");
        clp.setOption("my",         &my_,           "processors in y-direction.");
        clp.setOption("mz",         &mz_,           "processors in z-direction.");
        clp.setOption("stretchx",   &stretchx_,     "stretch mesh in x-direction.");
        clp.setOption("stretchy",   &stretchy_,     "stretch mesh in y-direction.");
        clp.setOption("stretchz",   &stretchz_,     "stretch mesh in z-direction.");
        clp.setOption("keepBCs",    &keepBCs_,      "keep Dirichlet boundary rows in matrix (0=false,1=true)");
        clp.setOption("matrixType", &matrixType_,   "matrix type: Laplace1D, Laplace2D, Laplace3D, ..."); //TODO: Star2D, numGlobalElements=...
        clp.setOption("h",          &h_,            "mesh width for uniform h");
        clp.setOption("delta",      &delta_,        "maximum PML damping value");
        clp.setOption("PMLx_left",  &PMLx_left,     "PML grid points in x-direction (left boundary)");
        clp.setOption("PMLx_right", &PMLx_right,    "PML grid points in x-direction (right boundary)");
        clp.setOption("PMLy_left",  &PMLy_left,     "PML grid points in y-direction (left boundary)");
        clp.setOption("PMLy_right", &PMLy_right,    "PML grid points in y-direction (right boundary)");
        clp.setOption("PMLz_left",  &PMLz_left,     "PML grid points in z-direction (left boundary)");
        clp.setOption("PMLz_right", &PMLz_right,    "PML grid points in z-direction (right boundary)");
        clp.setOption("omega",      &omega_,        "angular frequency omega");
        clp.setOption("shift",      &shift_,        "complex frequency shift");
        clp.setOption("mx",         &mx_,           "processors in x-direction.");
        clp.setOption("my",         &my_,           "processors in y-direction.");
        clp.setOption("mz",         &mz_,           "processors in z-direction.");
        clp.setOption("model",      &model_,        "velocity model");
      }

      GO GetNumGlobalElements() const {
        const Teuchos::ParameterList& pL = GetParameterList();

        const std::string& matrixType = pL.get<std::string>("matrixType");
        const GO nx = pL.get<GO>("nx");
        const GO ny = pL.get<GO>("ny");
        const GO nz = pL.get<GO>("nz");

        GO numGlobalElements = -1;
        if (matrixType == "Laplace1D" || matrixType == "Helmholtz1D")
          numGlobalElements = nx;

        else if (matrixType == "Laplace2D"    ||
                 matrixType == "Star2D"       ||
                 matrixType == "BigStar2D"    ||
                 matrixType == "Elasticity2D" ||
                 matrixType == "Helmholtz2D")
          numGlobalElements = nx*ny;

        else if (matrixType == "Laplace3D"    ||
                 matrixType == "Brick3D"      ||
                 matrixType == "BigStar2D"    ||
                 matrixType == "Elasticity3D" ||
                 matrixType == "Helmholtz3D")
          numGlobalElements = nx*ny*nz;

        TEUCHOS_TEST_FOR_EXCEPTION(numGlobalElements < 0, std::runtime_error,
                                   "Gallery: numGlobalElements < 0 (did you forget --ny (or --nz) for 2D (3D) problems?)");

        return numGlobalElements;
      }

      const std::string& GetMatrixType() const {
        const Teuchos::ParameterList& paramList = GetParameterList();
        return paramList.get<std::string>("matrixType");
      }

      void check() const { }

      Teuchos::ParameterList& GetParameterList() const {
        if (!paramList_.is_null())
          return *paramList_;

        paramList_ = rcp(new Teuchos::ParameterList());

        paramList_->set("nx",          nx_);
        paramList_->set("ny",          ny_);
        paramList_->set("nz",          nz_);
        paramList_->set("mx",          mx_);
        paramList_->set("my",          my_);
        paramList_->set("mz",          mz_);
        paramList_->set("model",       model_);
        paramList_->set("stretchx",    stretchx_);
        paramList_->set("stretchy",    stretchy_);
        paramList_->set("stretchz",    stretchz_);
        paramList_->set("keepBCs",     static_cast<bool>(keepBCs_));
        paramList_->set("matrixType",  matrixType_);
        paramList_->set("h",           h_);
        paramList_->set("delta",       delta_);
        paramList_->set("PMLx_left",   PMLx_left);
        paramList_->set("PMLx_right",  PMLx_right);
        paramList_->set("PMLy_left",   PMLy_left);
        paramList_->set("PMLy_right",  PMLy_right);
        paramList_->set("PMLz_left",   PMLz_left);
        paramList_->set("PMLz_right",  PMLz_right);
        paramList_->set("omega",       omega_);
        paramList_->set("shift",       shift_);

        check();

        return *paramList_;
      }

      //! @name Overridden from Teuchos::Describable
      //@{

      //! Return a simple one-line description of this object.
      std::string description() const {
        std::ostringstream out;
        out << Teuchos::Describable::description();
        out << "{type = "  << GetMatrixType() << ", size = " << GetNumGlobalElements() << "} ";

        return out.str();
      }

      //! Print the object with some verbosity level to an FancyOStream object.
      void describe(Teuchos::FancyOStream &out, const Teuchos::EVerbosityLevel verbLevel = verbLevel_default) const {
        int vl = (verbLevel == Teuchos::VERB_DEFAULT) ? Teuchos::VERB_LOW : verbLevel;
        if (vl == Teuchos::VERB_NONE)
          return;

        if (vl == Teuchos::VERB_LOW)
          out << description() << std::endl;
        else
          out << Teuchos::Describable::description() << std::endl;

        if (vl == Teuchos:: VERB_MEDIUM || vl == Teuchos::VERB_HIGH || vl == Teuchos::VERB_EXTREME) {
          Teuchos::OSTab tab1(out);

          const Teuchos::ParameterList& paramList = GetParameterList();
          std::string matrixType = paramList.get<std::string>("matrixType");
          GO nx = paramList.get<GO>("nx");
          GO ny = paramList.get<GO>("ny");
          GO nz = paramList.get<GO>("nz");

          out << "Matrix type: "  << matrixType << std::endl
              << "Problem size: " << GetNumGlobalElements();

          if      (matrixType == "Laplace2D" || matrixType == "Elasticity2D" || matrixType == "Helmholtz2D")  out << " (" << nx << "x" << ny << ")";
          else if (matrixType == "Laplace3D" || matrixType == "Elasticity3D" || matrixType == "Helmholtz3D")  out << " (" << nx << "x" << ny << "x" << nz << ")";

          out << std::endl;
        }
      }

      //@}

    private:
      // See Teuchos BUG 5249: https://software.sandia.gov/bugzilla/show_bug.cgi?id=5249
      mutable GO     nx_, ny_, nz_;
      mutable int    mx_, my_, mz_;
      mutable double stretchx_, stretchy_, stretchz_;

      std::string    matrixType_;

      mutable int    keepBCs_;

      mutable double    h_;
      mutable double    delta_;
      mutable int       PMLx_left, PMLx_right;
      mutable int       PMLy_left, PMLy_right;
      mutable int       PMLz_left, PMLz_right;
      mutable double    omega_;
      mutable double    shift_;
      mutable int       model_;

      // There is a major assumption here:
      // As soon as somebody call GetParameterList(), we freeze all other variables into the list,
      // and ignore them. This allows us to make the modification of the list from outside.
      mutable Teuchos::RCP<Teuchos::ParameterList> paramList_;
    };

  }
}

#endif
