// @HEADER
// ************************************************************************
//
//               Rapid Optimization Library (ROL) Package
//                 Copyright (2014) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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
// Questions? Contact lead developers:
//              Drew Kouri   (dpkouri@sandia.gov) and
//              Denis Ridzal (dridzal@sandia.gov)
//
// ************************************************************************
// @HEADER

#ifndef ROL_TRUSTREGION_H
#define ROL_TRUSTREGION_H

/** \class ROL::TrustRegion
    \brief Provides interface for and implements trust-region subproblem solvers.
*/

#include "ROL_Types.hpp"
#include "ROL_HelperFunctions.hpp"

namespace ROL { 

template<class Real>
class TrustRegion {
private:

  ETrustRegion etr_;

  bool useSecantPrecond_;
  bool useSecantHessVec_;

  int maxit_;
  Real tol1_;
  Real tol2_;
  Real delmin_;
  Real delmax_;
  Real eta0_;
  Real eta1_;
  Real eta2_;
  Real gamma0_;
  Real gamma1_;
  Real gamma2_; 

  Real pRed_;

  Real TRsafe_;
  Real eps_;

  Real alpha_;

  std::vector<bool> useInexact_;

  Real ftol_old_;

  Real scale_;
  Real omega_;
  Real force_;
  int  updateIter_;
  int  forceFactor_;
  int cnt_;

public:

  virtual ~TrustRegion() {}

  // Constructor
  TrustRegion( Teuchos::ParameterList & parlist ) : ftol_old_(ROL_OVERFLOW), cnt_(0) {
    // Unravel Parameter List
    // Enumerations
    etr_ = StringToETrustRegion( parlist.get("Trust-Region Subproblem Solver Type",  "Cauchy Point"));
    // Trust-Region Parameters
    delmin_ = parlist.get("Minimum Trust-Region Radius",          1.e-8);
    delmax_ = parlist.get("Maximum Trust-Region Radius",          5000.0);
    eta0_   = parlist.get("Step Acceptance Parameter",            0.05);
    eta1_   = parlist.get("Radius Shrinking Threshold",           0.05);
    eta2_   = parlist.get("Radius Growing Threshold",             0.9);
    gamma0_ = parlist.get("Radius Shrinking Rate (Negative rho)", 0.0625);
    gamma1_ = parlist.get("Radius Shrinking Rate (Positive rho)", 0.25);
    gamma2_ = parlist.get("Radius Growing Rate",                  2.5);
    TRsafe_ = parlist.get("Trust-Region Safeguard",               100.0);
    // CG Parameters
    if ( etr_ == TRUSTREGION_TRUNCATEDCG ) {
      maxit_ = parlist.get("Maximum Number of Krylov Iterations",     20);
      tol1_  = parlist.get("Absolute Krylov Tolerance",               1.e-4);
      tol2_  = parlist.get("Relative Krylov Tolerance",               1.e-2);
    }
    eps_    = TRsafe_*ROL_EPSILON;
    alpha_  = -1.0;

    // Inexactness Information
    useInexact_.clear();
    useInexact_.push_back(parlist.get("Use Inexact Objective Function", false));
    useInexact_.push_back(parlist.get("Use Inexact Gradient", false));
    useInexact_.push_back(parlist.get("Use Inexact Hessian-Times-A-Vector", false));
    scale_       = parlist.get("Value Update Tolerance Scaling",1.e-1);
    omega_       = parlist.get("Value Update Exponent",0.9);
    force_       = parlist.get("Value Update Forcing Sequence Initial Value",1.0);
    updateIter_  = parlist.get("Value Update Forcing Sequence Update Frequency",10);
    forceFactor_ = parlist.get("Value Update Forcing Sequence Reduction Factor",0.1);
  }

  void update( Vector<Real> &x, Real &fnew, Real &del, 
               int &nfval, int &ngrad, int &flagTR,
               const Vector<Real> &s, const Real snorm, 
               const Real fold, const Vector<Real> &g, 
               int iter, ProjectedObjective<Real> &pObj ) { 
    Real tol = std::sqrt(ROL_EPSILON);

    // Compute New Function Value
    Teuchos::RCP<Vector<Real> > xnew = x.clone();
    xnew->set(x);
    xnew->axpy(1.0,s);
    /***************************************************************************************************/
    // BEGIN INEXACT OBJECTIVE FUNCTION COMPUTATION
    /***************************************************************************************************/
    Real fold1 = fold;
    if ( this->useInexact_[0] ) {
      if ( !(this->cnt_%this->updateIter_) && (this->cnt_ != 0) ) {
        this->force_ *= this->forceFactor_;
      }
      Real c = this->scale_*std::max(1.e-2,std::min(1.0,1.e4*std::max(this->pRed_,std::sqrt(ROL_EPSILON))));
      Real ftol = c*std::pow(std::min(this->eta1_,1.0-this->eta2_)
                   *std::min(std::max(this->pRed_,std::sqrt(ROL_EPSILON)),this->force_),1.0/this->omega_);
      if ( this->ftol_old_ > ftol || this->cnt_ == 0 ) {
        this->ftol_old_ = ftol;
        fold1 = pObj.value(x,this->ftol_old_);
      }
      pObj.update(*xnew,true,iter);
      fnew = pObj.value(*xnew,ftol);
      this->cnt_++;
    }
    else {
      pObj.update(*xnew,true,iter);
      fnew = pObj.value(*xnew,tol);
    }
    nfval = 1;   
    Real aRed = fold1 - fnew;
    /***************************************************************************************************/
    // FINISH INEXACT OBJECTIVE FUNCTION COMPUTATION
    /***************************************************************************************************/

    // Compute Ratio of Actual and Predicted Reduction
    aRed -= this->eps_*((1.0 < std::abs(fold1)) ? 1.0 : std::abs(fold1));
    this->pRed_ -= this->eps_*((1.0 < std::abs(fold1)) ? 1.0 : std::abs(fold1));
    Real rho  = 0.0; 
    if ((std::abs(aRed) < this->eps_) && (std::abs(this->pRed_) < this->eps_)) {
      rho = 1.0; 
      flagTR = 0;
    }
    else {
      rho = aRed/this->pRed_;
      if (this->pRed_ < 0 && aRed > 0) { 
        flagTR = 1;
      }
      else if (aRed <= 0 && this->pRed_ > 0) {
        flagTR = 2;
      }
      else if (aRed <= 0 && this->pRed_ < 0) { 
        flagTR = 3;
      }
      else {
        flagTR = 0;
      }
    }

    // Check Sufficient Decrease in the Reduced Quadratic Model
    bool decr = true;
    if ( pObj.isConActivated() && (std::abs(aRed) > this->eps_) ) { 
      // Compute Criticality Measure || x - P( x - g ) ||
      Teuchos::RCP<Vector<Real> > pg = x.clone();
      pg->set(x);
      pg->axpy(-1.0,g);
      pObj.project(*pg);
      pg->scale(-1.0);
      pg->plus(x);
      Real pgnorm = pg->norm();
      // Compute Scaled Measure || x - P( x - lam * PI(g) ) ||
      pg->set(g);
      pObj.pruneActive(*pg,g,x);
      Real lam = std::min(1.0, del/pg->norm());
      pg->scale(-lam);
      pg->plus(x);
      pObj.project(*pg);
      pg->scale(-1.0);
      pg->plus(x);      
      pgnorm *= pg->norm();
      // Sufficient decrease?
      decr = ( aRed >= 0.1*this->eta0_*pgnorm );
    }
    
    // Accept or Reject Step and Update Trust Region
    if ((rho < this->eta0_ && flagTR == 0) || flagTR >= 2 || !decr ) { // Step Rejected 
      pObj.update(x,true,iter);
      fnew = fold1;
      if (rho < 0.0) { // Negative reduction, interpolate to find new trust-region radius
        Real gs = g.dot(s);
        Teuchos::RCP<Vector<Real> > Hs = x.clone();
        pObj.hessVec(*Hs,s,x,tol);
        Real modelVal = Hs->dot(s);
        modelVal *= 0.5;
        modelVal += gs + fold1;
        Real theta = (1.0-this->eta2_)*gs/((1.0-this->eta2_)*(fold1+gs)+this->eta2_*modelVal-fnew);
        del  = std::min(this->gamma1_*snorm,std::max(this->gamma0_,theta)*del);
      }
      else { // Shrink trust-region radius
        del = this->gamma1_*snorm; 
      } 
    }
    else if ((rho >= this->eta0_ && flagTR != 3) || flagTR == 1) { // Step Accepted
      x.axpy(1.0,s);
      if (rho >= this->eta2_) { // Increase trust-region radius
        del = std::min(this->gamma2_*del,this->delmax_);
      }
    }
  }

  void run( Vector<Real> &s, Real &snorm, Real &del, int &iflag, int &iter, const Vector<Real> &x,
            const Vector<Real> &grad, const Real &gnorm, ProjectedObjective<Real> &pObj ) { 
    // Run Trust Region
    if ( this->etr_ == TRUSTREGION_CAUCHYPOINT ) {
      this->cauchypoint(s,snorm,del,iflag,iter,x,grad,gnorm,pObj);
    }
    else if ( this->etr_ == TRUSTREGION_TRUNCATEDCG ) {
      this->truncatedCG(s,snorm,del,iflag,iter,x,grad,gnorm,pObj);
      if ( pObj.isConActivated() ) {
        pObj.computeProjectedStep(s,x);
      }
#if 0
      if ( pObj.isConActivated() ) {
        this->truncatedCG_proj(s,snorm,del,iflag,iter,x,grad,gnorm,pObj);
      }
      else {
        this->truncatedCG(s,snorm,del,iflag,iter,x,grad,grad,gnorm,pObj);
      }
#endif
    }
    else if ( this->etr_ == TRUSTREGION_DOGLEG ) {
      this->dogleg(s,snorm,del,iflag,iter,x,grad,gnorm,pObj);
    }
    else if ( this->etr_ == TRUSTREGION_DOUBLEDOGLEG ) {
      this->doubledogleg(s,snorm,del,iflag,iter,x,grad,gnorm,pObj);
    }
#if 0
    // If constraints are active, then determine a feasible step
    if ( pObj.isConActivated() && this->etr_ != TRUSTREGION_CAUCHYPOINT ) {
      // Compute projected step stmp = P( x + alpha*s ) - x and xtmp = x + stmp
      Real alpha = 1.0;
      Teuchos::RCP<Vector<Real> > stmp = x.clone();
      stmp->set(s);
      stmp->scale(alpha);
      pObj.computeProjectedStep(*stmp,x);
      Teuchos::RCP<Vector<Real> > xtmp = x.clone();
      xtmp->set(x);
      xtmp->axpy(1.0,*stmp);
      // Compute model components for alpha = 1.0
      Real tol   = std::sqrt(ROL_EPSILON);
      Teuchos::RCP<Vector<Real> > Bs = x.clone();
      pObj.hessVec(*Bs,*stmp,x,tol);
      Real sBs   = Bs->dot(*stmp);
      Real gs    = grad.dot(*stmp);
      Real val   = gs + 0.5*sBs;
      Real val0  = val;
      // Backtrack alpha until x+alpha*s is feasible
      int cnt   = 0;
      int maxit = 10;
      while ( val > val0 || !pObj.isFeasible(*xtmp) ) { 
        // Backtrack alpha
        alpha *= 0.5;
        // Computed projected step P( x + alpha*s ) - x and xtmp = x + stmp
        stmp->set(s);
        stmp->scale(alpha);
        pObj.computeProjectedStep(*stmp,x);        
        xtmp->set(x);
        xtmp->axpy(1.0,*stmp);
        // Evaluate Model
        val0 = val;
        pObj.hessVec(*Bs,*stmp,x,tol);
        sBs = Bs->dot(*stmp);
        gs  = grad.dot(*stmp);
        val = gs + 0.5*sBs;
        // Update iteration count
        cnt++;
        if ( cnt >= maxit ) { break; }
      }
      s.set(*stmp);
      this->pRed_ = -val;
    }
#endif
  }

  void cauchypoint( Vector<Real> &s, Real &snorm, Real &del, int &iflag, int &iter, const Vector<Real> &x,
                    const Vector<Real> &grad, const Real &gnorm, ProjectedObjective<Real> &pObj ) {
    if ( pObj.isConActivated() ) {
      bool useM = false;
      if ( useM ) {
        this->cauchypoint_M( s, snorm, del, iflag, iter, x, grad, gnorm, pObj );
      } 
      else {
        this->cauchypoint_CGT( s, snorm, del, iflag, iter, x, grad, gnorm, pObj );
      }
    }
    else {
      this->cauchypoint_unc( s, snorm, del, iflag, iter, x, grad, gnorm, pObj );
    }
  }

  void truncatedCG( Vector<Real> &s, Real &snorm, Real &del, int &iflag, int &iter, const Vector<Real> &x,
                    const Vector<Real> &grad, const Real &gnorm, ProjectedObjective<Real> &pObj ) {
    Real tol = std::sqrt(ROL_EPSILON);
    const Real gtol = std::min(this->tol1_,this->tol2_*gnorm);

    // Old and New Step Vectors
    s.zero(); 
    snorm = 0.0;
    Real snorm2  = 0.0;
    Teuchos::RCP<Vector<Real> > s1 = x.clone();
    s1->zero();
    Real s1norm2 = 0.0;

    // Gradient Vector
    Teuchos::RCP<Vector<Real> > g = x.clone(); 
    g->set(grad);
    Real normg = gnorm;
    if ( pObj.isConActivated() ) {
      pObj.pruneActive(*g,grad,x);
      normg = g->norm();
    }

    // Preconditioned Gradient Vector
    Teuchos::RCP<Vector<Real> > v  = x.clone();
    //pObj.precond(*v,*g,x,tol);
    pObj.reducedPrecond(*v,*g,x,grad,x,tol);

    // Basis Vector
    Teuchos::RCP<Vector<Real> > p = x.clone(); 
    p->set(*v); 
    p->scale(-1.0);
    Real pnorm2 = v->dot(*g);

    // Hessian Times Basis Vector
    Teuchos::RCP<Vector<Real> > Hp = x.clone();

    iter        = 0; 
    iflag       = 0;
    Real kappa  = 0.0;
    Real beta   = 0.0; 
    Real sigma  = 0.0; 
    Real alpha  = 0.0; 
    Real tmp    = 0.0;
    Real gv     = v->dot(*g);
    Real sMp    = 0.0;
    this->pRed_ = 0.0;

    for (iter = 0; iter < this->maxit_; iter++) {
      //pObj.hessVec(*Hp,*p,x,tol);
      pObj.reducedHessVec(*Hp,*p,x,grad,x,tol);

      kappa = p->dot(*Hp);
      if (kappa <= 0.0) {
        sigma = (-sMp+sqrt(sMp*sMp+pnorm2*(del*del-snorm2)))/pnorm2;
        s.axpy(sigma,*p);
        iflag = 2; 
        break;
      }

      alpha = gv/kappa;
      s1->set(s); 
      s1->axpy(alpha,*p);
      s1norm2 = snorm2 + 2.0*alpha*sMp + alpha*alpha*pnorm2;

      if (s1norm2 >= del*del) {
        sigma = (-sMp+sqrt(sMp*sMp+pnorm2*(del*del-snorm2)))/pnorm2;
        s.axpy(sigma,*p);
        iflag = 3; 
        break;
      }

      this->pRed_ += 0.5*alpha*gv;

      s.set(*s1);
      snorm2 = s1norm2;  

      g->axpy(alpha,*Hp);
      normg = g->norm();
      if (normg < gtol) {
        break;
      }

      //pObj.precond(*v,*g,x,tol);
      pObj.reducedPrecond(*v,*g,x,grad,x,tol);
      tmp   = gv; 
      gv    = v->dot(*g);
      beta  = gv/tmp;    

      p->scale(beta);
      p->axpy(-1.0,*v);
      sMp    = beta*(sMp+alpha*pnorm2);
      pnorm2 = gv + beta*beta*pnorm2; 
    }
    if (iflag > 0) {
      this->pRed_ += sigma*(gv-0.5*sigma*kappa);
    }

    if (iter == this->maxit_) {
      iflag = 1;
    }
    if (iflag != 1) { 
      iter++;
    }

    snorm = s.norm();
  }

#if 0
  void truncatedCG_proj( Vector<Real> &s, Real &snorm, Real &del, int &iflag, int &iter, const Vector<Real> &x,
                         const Vector<Real> &grad, const Real &gnorm, ProjectedObjective<Real> &pObj ) {
    Real tol = std::sqrt(ROL_EPSILON);

    const Real gtol = std::min(this->tol1_,this->tol2_*gnorm);

    // Compute Cauchy Point
    Real scnorm = 0.0;
    Teuchos::RCP<Vector<Real> > sc = x.clone();
    this->cauchypoint(*sc,scnorm,del,iflag,iter,x,grad,gnorm,pObj);
    Teuchos::RCP<Vector<Real> > xc = x.clone();
    xc->set(x);
    xc->plus(*sc);

    // Old and New Step Vectors
    s.set(*sc); 
    snorm = s.norm();
    Real snorm2  = snorm*snorm;
    Teuchos::RCP<Vector<Real> > s1 = x.clone();
    s1->zero();
    Real s1norm2 = 0.0;

    // Gradient Vector
    Teuchos::RCP<Vector<Real> > g = x.clone(); 
    g->set(grad);
    Teuchos::RCP<Vector<Real> > Hs = x.clone();
    pObj.reducedHessVec(*Hs,s,*xc,x,tol);
    g->plus(*Hs);
    Real normg = g->norm();

    // Preconditioned Gradient Vector
    Teuchos::RCP<Vector<Real> > v  = x.clone();
    pObj.reducedPrecond(*v,*g,*xc,x,tol);

    // Basis Vector
    Teuchos::RCP<Vector<Real> > p = x.clone(); 
    p->set(*v); 
    p->scale(-1.0);
    Real pnorm2 = v->dot(*g);

    // Hessian Times Basis Vector
    Teuchos::RCP<Vector<Real> > Hp = x.clone();

    iter        = 0; 
    iflag       = 0; 
    Real kappa  = 0.0;
    Real beta   = 0.0; 
    Real sigma  = 0.0; 
    Real alpha  = 0.0; 
    Real tmp    = 0.0;
    Real gv     = v->dot(*g);
    Real sMp    = 0.0;
    this->pRed_ = 0.0;

    for (iter = 0; iter < this->maxit_; iter++) {
      pObj.reducedHessVec(*Hp,*p,*xc,x,tol);

      kappa = p->dot(*Hp);
      if (kappa <= 0) {
        sigma = (-sMp+sqrt(sMp*sMp+pnorm2*(del*del-snorm2)))/pnorm2;
        s.axpy(sigma,*p);
        iflag = 2; 
        break;
      }

      alpha = gv/kappa;
      s1->set(s); 
      s1->axpy(alpha,*p);
      s1norm2 = snorm2 + 2.0*alpha*sMp + alpha*alpha*pnorm2;

      if (s1norm2 >= del*del) {
        sigma = (-sMp+sqrt(sMp*sMp+pnorm2*(del*del-snorm2)))/pnorm2;
        s.axpy(sigma,*p);
        iflag = 3; 
        break;
      }

      this->pRed_ += 0.5*alpha*gv;

      s.set(*s1);
      snorm2 = s1norm2;  

      g->axpy(alpha,*Hp);
      normg = g->norm();
      if (normg < gtol) {
        break;
      }

      pObj.reducedPrecond(*v,*g,*xc,x,tol);
      tmp   = gv; 
      gv    = v->dot(*g);
      beta  = gv/tmp;    

      p->scale(beta);
      p->axpy(-1.0,*v);
      sMp    = beta*(sMp+alpha*pnorm2);
      pnorm2 = gv + beta*beta*pnorm2; 
    }
    if (iflag > 0) {
      this->pRed_ += sigma*(gv-0.5*sigma*kappa);
    }

    if (iter == this->maxit_) {
      iflag = 1;
    }
    if (iflag != 1) { 
      iter++;
    }

    snorm = s.norm();
  }
#endif

  void dogleg( Vector<Real> &s, Real &snorm, Real &del, int &iflag, int &iter, const Vector<Real> &x,
               const Vector<Real> &grad, const Real &gnorm, ProjectedObjective<Real> &pObj ) {
    Real tol = std::sqrt(ROL_EPSILON);

    // Compute quasi-Newton step
    Teuchos::RCP<Vector<Real> > sN = x.clone();
    pObj.reducedInvHessVec(*sN,grad,x,grad,x,tol);
    sN->scale(-1.0);
    Real sNnorm = sN->norm();
    Real gsN    = grad.dot(*sN);
    bool negCurv = false;
    if ( gsN >= 0.0 ) {
      negCurv = true;
    }

    if ( negCurv ) {
      this->cauchypoint(s,snorm,del,iflag,iter,x,grad,gnorm,pObj);
      iflag = 2;
    }  
    else {
      // Approximately solve trust region subproblem using double dogleg curve
      if (sNnorm <= del) {        // Use the quasi-Newton step
        s.set(*sN); 
        snorm = sNnorm;
        this->pRed_ = -0.5*gsN;
        iflag = 0;
      }
      else {                      // quasi-Newton step is outside of trust region
        Teuchos::RCP<Vector<Real> > Bg = x.clone(); 
        pObj.reducedHessVec(*Bg,grad,x,grad,x,tol);
        Real alpha  = 0.0;
        Real beta   = 0.0;
        Real gnorm2 = gnorm*gnorm;
        Real gBg    = grad.dot(*Bg);
        Real gamma  = gnorm2/gBg;
        if ( gamma*gnorm >= del || gBg <= 0.0 ) {
            alpha = 0.0;
            beta  = del/gnorm;
            s.set(grad); 
            s.scale(-beta); 
            snorm = del;
            iflag = 2;
        }
        else {
          Real a = sNnorm*sNnorm + 2.0*gamma*gsN + gamma*gamma*gnorm2;
          Real b = -gamma*gsN - gamma*gamma*gnorm2;
          Real c = gamma*gamma*gnorm2 - del*del;
          alpha  = (-b + sqrt(b*b - a*c))/a;
          beta   = gamma*(1.0-alpha);
          s.set(grad);
          s.scale(-beta);
          s.axpy(alpha,*sN);
          snorm = del;
          iflag = 1;
        }
        this->pRed_ = (alpha*(0.5*alpha-1)*gsN - 0.5*beta*beta*gBg + beta*(1-alpha)*gnorm2);
      }
    }
  }

  void doubledogleg( Vector<Real> &s, Real &snorm, Real &del, int &iflag, int &iter, const Vector<Real> &x,
                     const Vector<Real> &grad, const Real &gnorm, ProjectedObjective<Real> &pObj ) {
    Real tol = std::sqrt(ROL_EPSILON);

    // Compute quasi-Newton step
    Teuchos::RCP<Vector<Real> > sN = x.clone();
    pObj.reducedInvHessVec(*sN,grad,x,grad,x,tol);
    sN->scale(-1.0);
    Real sNnorm = sN->norm();
    Real tmp    = grad.dot(*sN);
    bool negCurv = false;
    if ( tmp >= 0.0 ) {
      negCurv = true;
    }
    Real gsN = std::abs(tmp);

    if ( negCurv ) {
      this->cauchypoint(s,snorm,del,iflag,iter,x,grad,gnorm,pObj);
      iflag = 2;
    }  
    else {
      // Approximately solve trust region subproblem using double dogleg curve
      if (sNnorm <= del) {        // Use the quasi-Newton step
        s.set(*sN); 
        snorm = sNnorm;
        this->pRed_ = 0.5*gsN;
        iflag = 0;
      }
      else {                      // quasi-Newton step is outside of trust region
        Teuchos::RCP<Vector<Real> > Bg = x.clone(); 
        pObj.reducedHessVec(*Bg,grad,x,grad,x,tol);
        Real alpha  = 0.0;
        Real beta   = 0.0;
        Real gnorm2 = gnorm*gnorm;
        Real gBg    = grad.dot(*Bg);
        Real gamma1 = gnorm/gBg;
        Real gamma2 = gnorm/gsN;
        Real eta    = 0.8*gamma1*gamma2 + 0.2;
        if (eta*sNnorm <= del || gBg <= 0.0) { // Dogleg Point is inside trust region
          alpha = del/sNnorm;
          beta  = 0.0;
          s.set(*sN);
          s.scale(alpha);
          snorm = del;
          iflag = 1;
        }
        else {
          if (gnorm2*gamma1 >= del) { // Cauchy Point is outside trust region
            alpha = 0.0;
            beta  = -del/gnorm;
            s.set(grad); 
            s.scale(beta); 
            snorm = del;
            iflag = 2;
          }
          else {              // Find convex combination of Cauchy and Dogleg point
            s.set(grad);
            s.scale(-gamma1*gnorm);
            Teuchos::RCP<Vector<Real> > w = x.clone(); 
            w->set(s);
            w->scale(-1.0);
            w->axpy(eta,*sN);
            Real wNorm = w->dot(*w);
            Real sigma = del*del-std::pow(gamma1*gnorm,2.0);
            Real phi   = s.dot(*w);
            Real theta = (-phi + std::sqrt(phi*phi+wNorm*sigma))/wNorm;
            s.axpy(theta,*w); 
            snorm = del;
            alpha = theta*eta;
            beta  = (1.0-theta)*(-gamma1*gnorm);
            iflag = 3;
          }
        }
        this->pRed_ = -(alpha*(0.5*alpha-1)*gsN + 0.5*beta*beta*gBg + beta*(1-alpha)*gnorm2);
      }
    }
  }

  void cauchypoint_unc( Vector<Real> &s, Real &snorm, Real &del, int &iflag, int &iter, const Vector<Real> &x,
                        const Vector<Real> &grad, const Real &gnorm, ProjectedObjective<Real> &pObj ) {
    Real tol = std::sqrt(ROL_EPSILON);
    Teuchos::RCP<Vector<Real> > Bg = x.clone();
    pObj.hessVec(*Bg,grad,x,tol);
    Real gBg = Bg->dot(grad);
    Real tau = 1.0;
    if ( gBg > 0.0 ) {
      tau = std::min(1.0, gnorm*gnorm*gnorm/gBg);
    }

    s.set(grad);
    s.scale(-tau*del/gnorm);
    snorm = tau*del;
    iflag = 0;
    iter  = 0;
    this->pRed_ = tau*del/gnorm * pow(gnorm,2.0) - 0.5*pow(tau*del/gnorm,2.0)*gBg;
  }

  void cauchypoint_M( Vector<Real> &s, Real &snorm, Real &del, int &iflag, int &iter, const Vector<Real> &x,
                      const Vector<Real> &grad, const Real &gnorm, ProjectedObjective<Real> &pObj ) {
    Real tol = std::sqrt(ROL_EPSILON);

    // Parameters
    Real mu0   = 1.e-2;
    Real mu1   = 1.0;
    Real beta1 = 0.0;
    Real beta2 = 0.0;
    bool decr  = true;
    bool stat  = true;

    // Initial step length
    Real alpha  = 1.0;
    if ( this->alpha_ > 0.0 ) {
      alpha = this->alpha_;
    } 
    Real alpha0   = alpha;
    Real alphamax = 1.e4*alpha;
    
    // Initial model value
    s.set(grad);
    s.scale(-alpha);
    pObj.computeProjectedStep(s,x);
    snorm = s.norm();
    Teuchos::RCP<Vector<Real> > Hs = x.clone();
    pObj.hessVec(*Hs,s,x,tol);
    Real gs   = grad.dot(s);
    Real val  = gs + 0.5*Hs->dot(s);
    Real val0 = val;

    // Determine whether to increase or decrease alpha
    if ( val > mu0 * gs || snorm > mu1 * del ) { 
      beta1 = 0.5; 
      beta2 = 0.5; 
      decr  = true;
    }
    else {
      beta1 = 2.0;
      beta2 = 2.0;
      decr  = false;
    }

    while ( stat ) {
      // Update step length
      alpha0 = alpha;
      val0   = val;
      alpha *= (beta1+beta2)*0.5;
  
      // Update model value
      s.set(grad);
      s.scale(-alpha);
      pObj.computeProjectedStep(s,x);
      snorm = s.norm();
      pObj.hessVec(*Hs,s,x,tol);
      gs    = grad.dot(s);
      val   = gs + 0.5*Hs->dot(s);

      // Update termination criterion
      if ( decr ) {
        stat = ( val > mu0 * gs || snorm > mu1 * del );
        if ( std::abs(val) < this->eps_ && std::abs(mu0 *gs) < this->eps_ ) {
          stat = (snorm > mu1 * del);
        }
      }
      else {
        stat = !( val > mu0 * gs || snorm > mu1 * del );
        if ( std::abs(val) < this->eps_ && std::abs(mu0 *gs) < this->eps_ ) {
          stat = !(snorm > mu1 * del);
        }
        if ( alpha > alphamax ) {
          stat = false;
        }
      } 
    }
    // Reset to last 'successful' step
    val   = val0;
    alpha = alpha0;
    s.set(grad);
    s.scale(-alpha);
    pObj.computeProjectedStep(s,x);
    snorm = s.norm();
    
    this->alpha_ = alpha;
    this->pRed_  = -val;
  }

  void cauchypoint_CGT( Vector<Real> &s, Real &snorm, Real &del, int &iflag, int &iter, const Vector<Real> &x,
                        const Vector<Real> &grad, const Real &gnorm, ProjectedObjective<Real> &pObj ) {
    Real tol = std::sqrt(ROL_EPSILON);
    bool tmax_flag = true;
    int maxit      = 20;
    Real t         = del/gnorm;
    Real tmax      = 1.e10;
    Real tmin      = 0.0;
    Real gs        = 0.0;
    Real c1        = 0.25;
    Real c2        = 0.75;
    Real c3        = 0.9;
    Real c4        = 0.25;
    Real pgnorm    = 0.0;
    Teuchos::RCP<Vector<Real> > gnew = x.clone();
    Teuchos::RCP<Vector<Real> > p    = x.clone();
    Teuchos::RCP<Vector<Real> > Bs   = x.clone();
    for ( int i = 0; i < maxit; i++ ) {
      // Compute p = x + s = P(x - t*g)
      p->set(x);
      p->axpy(-t,grad); 
      pObj.project(*p);
      // Compute s = p - x = P(x - t*g) - x
      s.set(*p);
      s.axpy(-1.0,x);
      snorm = s.norm();
      // Evaluate Model
      pObj.hessVec(*Bs,s,x,tol);
      gs = grad.dot(s);
      this->pRed_ = -gs - 0.5*Bs->dot(s);

      // Check Stopping Conditions
      gnew->set(grad);
      pObj.pruneActive(*gnew,grad,*p); // Project gradient onto tangent cone at p
      pgnorm = gnew->norm();
      if ( snorm > del || this->pRed_ < -c2*gs ) {
        tmax = t;
        tmax_flag = false;
      }
      else if ( snorm < c3*del && this->pRed_ > -c1*gs && pgnorm > c4*std::abs(gs)/del ) {
        tmin = t;
      } 
      else {
        break;
      }
   
      // Update t
      if ( tmax_flag ) {
        t *= 2.0;
      }
      else {
        t = 0.5*(tmax + tmin);
      }
    }
  }

};

}

#endif
