#pragma once
#ifndef __ICHOL_LEFT_UNBLOCKED_HPP__
#define __ICHOL_LEFT_UNBLOCKED_HPP__

/// \file ichol_left_unblocked.hpp
/// \brief Unblocked incomplete Chloesky factorization.
/// \author Kyungjoo Kim (kyukim@sandia.gov)

#include "util.hpp"
#include "partition.hpp"
#include "scale.hpp"
#include "dot.hpp"
#include "gemv.hpp"

namespace Example { 

  using namespace std;

  template<>
  template<typename CrsMatViewType>
  KOKKOS_INLINE_FUNCTION
  int 
  IChol<Uplo::Lower,Algo::LeftUnblocked>::invoke(const CrsMatViewType A) {
    typedef typename CrsMatViewType::value_type    value_type;
    typedef typename CrsMatViewType::ordinal_type  ordinal_type;
    typedef typename CrsMatViewType::row_view_type row_view_type;
    
    value_type zero = 0.0;
    
    CrsMatViewType ATL, ATR,      A00,  a01,     A02,
      /**/         ABL, ABR,      a10t, alpha11, a12t,
      /**/                        A20,  a21,     A22;    
    
    Part_2x2(A,   ATL, ATR,
             /**/ ABL, ABR, 
             0, 0, Partition::TopLeft);
    
    while (ATL.NumRows() < A.NumRows()) {
      Part_2x2_to_3x3(ATL, ATR, /**/  A00,  a01,     A02,
                      /*******/ /**/  a10t, alpha11, a12t,
                      ABL, ABR, /**/  A20,  a21,     A22,  
                      1, 1, Partition::BottomRight);
      // -----------------------------------------------------

      // extract diagonal from alpha11 
      row_view_type alpha = alpha11.extractRow(0);
      ordinal_type id = alpha.Index(0);
      value_type &alpha_val = (id < 0 ? zero : alpha.Value(id));
                           
      // if encounter null diag, return the -(row + 1)
      if (abs(alpha_val) == 0.0) 
        return -(ATL.NumRows() + 1);

      // update on alpha_val
      row_view_type r10t = a10t.extractRow(0);
      alpha_val -= dot(r10t, r10t);

      // sparse gemv 
      gemv_nt_t(-1.0, A20, a10t, 1.0, a21);

      // sqrt on diag
      alpha_val = sqrt(real(alpha_val));

      // sparse inverse scale
      scale(1.0/real(alpha_val), a21);

      // -----------------------------------------------------
      Merge_3x3_to_2x2(A00,  a01,     A02,  /**/ ATL, ATR,
                       a10t, alpha11, a12t, /**/ /******/
                       A20,  a21,     A22,  /**/ ABL, ABR,
                       Partition::TopLeft);
    }

    return 0;
  }

}

#endif
