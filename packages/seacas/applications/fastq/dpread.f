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

C $Id: dpread.f,v 1.1 1990/11/30 11:06:19 gdsjaar Exp $
C $Log: dpread.f,v $
C Revision 1.1  1990/11/30 11:06:19  gdsjaar
C Initial revision
C
C
CC* FILE: [.MAIN]DPREAD.FOR
CC* MODIFIED BY: TED BLACKER
CC* MODIFICATION DATE: 7/6/90
CC* MODIFICATION: COMPLETED HEADER INFORMATION
C
      SUBROUTINE DPREAD (X, Y, BUTTON)
C***********************************************************************
C
C  SUBROUTINE DPREAD = READS INPUT FROM A DIGIPAD DIGITIZING TABLET
C
C***********************************************************************
C
      CHARACTER*1 BUTTON, DUMMY*5
C
C  SWITCH THE TERMINAL TO PASS-THRU MODE <ESC>[5i
C
      DUMMY (1:1) = '+'
      DUMMY (2:2) = CHAR (27)
      DUMMY (3:5) = '[5i'
      WRITE (*, ' (A)')DUMMY
C
C  INPUT THE BUTTON AND X, Y PAIR FROM THE PAD
C
      BUTTON = ' '
      READ (*, 10000, END = 100)BUTTON, IX, IY
C
C  CONVERT THE BUTTON
C
      IF (BUTTON .EQ. ':') THEN
         BUTTON = 'A'
      ELSEIF (BUTTON .EQ. ';') THEN
         BUTTON = 'B'
      ELSEIF (BUTTON .EQ. '<') THEN
         BUTTON = 'C'
      ELSEIF (BUTTON .EQ. ' = ') THEN
         BUTTON = 'D'
      ELSEIF (BUTTON .EQ. '>') THEN
         BUTTON = 'E'
      ELSEIF (BUTTON .EQ. '?') THEN
         BUTTON = 'F'
      ELSEIF (BUTTON .EQ. ' ') THEN
         BUTTON = 'E'
      END IF
C
C CONVERT  (X,  Y) LOCATION
C
      X = IX
      Y = IY
C
  100 CONTINUE
C
C  SWITCH THE TERMINAL OUT OF PASS-THRU MODE <ESC>[4i
C
      WRITE (*, ' (A)')' '//CHAR (27)//'[4i'
      RETURN
C
10000 FORMAT (A1, I5, 1X, I5)
C
      END
