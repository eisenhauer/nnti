C    Copyright (c) 2014, Sandia Corporation.
C    Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
C    the U.S. Government retains certain rights in this software.
C    
C    Redistribution and use in source and binary forms, with or without
C    modification, are permitted provided that the following conditions are
C    met:
C    
C        * Redistributions of source code must retain the above copyright
C          notice, this list of conditions and the following disclaimer.
C    
C        * Redistributions in binary form must reproduce the above
C          copyright notice, this list of conditions and the following
C          disclaimer in the documentation and/or other materials provided
C          with the distribution.
C    
C        * Neither the name of Sandia Corporation nor the names of its
C          contributors may be used to endorse or promote products derived
C          from this software without specific prior written permission.
C    
C    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
C    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
C    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
C    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
C    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
C    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
C    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
C    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
C    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
C    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
C    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
C    

C $Id: fndlnk.f,v 1.1 1990/11/30 11:07:43 gdsjaar Exp $
C $Log: fndlnk.f,v $
C Revision 1.1  1990/11/30 11:07:43  gdsjaar
C Initial revision
C
C
CC* FILE: [.QMESH]FNDLNK.FOR
CC* MODIFIED BY: TED BLACKER
CC* MODIFICATION DATE: 7/6/90
CC* MODIFICATION: COMPLETED HEADER INFORMATION
C
      SUBROUTINE FNDLNK (MXND, LXK, NXL, K, N1, N2, L, ERR)
C***********************************************************************
C
C  SUBROUTINE FNDLNK = FIND THE LINE IN ELEMENT K WITH NODES N1 AND N2
C
C***********************************************************************
C
      DIMENSION LXK (4, MXND), NXL (2, 3 * MXND)
C
      LOGICAL ERR
C
      ERR = .FALSE.
      DO 100 I = 1, 4
         LL = LXK (I, K)
         M1 = NXL (1, LL)
         M2 = NXL (2, LL)
         IF ( ( (M1 .EQ. N1) .AND. (M2 .EQ. N2)) .OR.
     &      ( (M2 .EQ. N1) .AND. (M1 .EQ. N2) ) ) THEN
            L = LL
            RETURN
         ENDIF
  100 CONTINUE
      L = 0
      ERR = .TRUE.
      WRITE ( * , 10000) K, N1, N2
10000 FORMAT (' IN FNDLNK, NO LINE CAN BE FOUND FOR K, N1, N2: ', 3I5)
      RETURN
      END
