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

C $Id: showfl.f,v 1.1 1991/02/21 15:45:37 gdsjaar Exp $
C $Log: showfl.f,v $
C Revision 1.1  1991/02/21 15:45:37  gdsjaar
C Initial revision
C
      SUBROUTINE SHOWFL (TYPE, NUMESS, IDESS, NEESS, IPEESS)
      DIMENSION IDESS(*), NEESS(*), IPEESS(*)
      CHARACTER*1 TYPE
C
      IF (TYPE .EQ. 'S') THEN
      WRITE (*, 20)
      DO 10 I=1, NUMESS
          WRITE (*, 30) I, IDESS(I), NEESS(I), IPEESS(I)
   10 CONTINUE
      ELSE IF (TYPE .EQ. 'N') THEN
      WRITE (*, 40)
      DO 15 I=1, NUMESS
          WRITE (*, 30) I, IDESS(I), NEESS(I)
   15 CONTINUE
      END IF
C
   20 FORMAT (/' Side Set Flags:'/
     *    '           ID     Elements    Nodes')
   40 FORMAT (/' Node Set Flags:'/
     *    '           ID     Nodes')
   30 FORMAT (I6,':',3(I6,5X))
      RETURN
      END
