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

C $Id: output.f,v 1.5 1997/06/20 19:11:27 caforsy Exp $
C $Log: output.f,v $
C Revision 1.5  1997/06/20 19:11:27  caforsy
C Port to ibm
C
C Revision 1.4  1996/06/25 21:48:58  gdsjaar
C Modified output to support large (millions of elements) models
C
c Revision 1.3  1991/09/16  20:07:46  gdsjaar
c Upped integer output format to deal with elements over 100,000
c
c Revision 1.2  1991/02/21  16:38:00  gdsjaar
c Moved ENGNOT function out of write statements
c
c Revision 1.1.1.1  1991/02/21  15:44:38  gdsjaar
c NUMBERS: Greg Sjaardema, initial Unix release
c
c Revision 1.1  1991/02/21  15:44:37  gdsjaar
c Initial revision
c
      SUBROUTINE OUTPUT (MASS, DENS, VOLM, CG, ZI, MAT, NDIM, NBLK,
     *   VOL, VOLMN, IELM, NQUAD, LABMAT, AXI, TIME)
C
      include 'nu_io.blk'
      CHARACTER*16 LABMAT(*)
      CHARACTER*16 ENGNOT, ENG1
      REAL MASS(*), DENS(*), VOLM(*), CG(*), ZI(*)
      DIMENSION MAT(6,*), VOLMN(4,*), IELM(4,*)
      LOGICAL AXI, FIRST
      CHARACTER*6 LABEL(3)
      DATA LABEL/'      ',' Area ','Volume'/
      DATA FIRST /.TRUE./
C
      DO 160 IO=IOMIN,IOMAX
         ENG1 = ENGNOT(TIME,2)
         IF (FIRST) THEN
            WRITE (IO, 10) NQUAD, ENG1
   10       FORMAT ('1',5X,'Mass Properties Calculation',/
     *                  5X,I1,'-Point Quadrature, Time = ',A16)
         ELSE
            WRITE (IO, 20) ENG1
   20       FORMAT ('1     Time = ',A16)
         END IF
         TMASS = 0.
         ILAB = 3
         IF (NDIM .EQ. 2 .AND. .NOT. AXI) ILAB = 2
         WRITE (IO, 30) LABEL(ILAB)
   30    FORMAT(/5X,'Material',4X,'Density',8X,A6,10X,'Mass',
     *      8X,'Label')
         DO 50 ITMP=1,NBLK
            I = MAT(6, ITMP)
            IF (MAT(5,I) .NE. 1) GOTO 50
            WRITE (IO, 40) MAT(1,I),DENS(I),VOLM(I),MASS(I),LABMAT(I)
            TMASS = TMASS + MASS(I)
   40       FORMAT (I10,3(5X,1PE10.3),5X,A16)
   50    CONTINUE
C
         WRITE (IO, 60) VOL, TMASS
   60    FORMAT (25X,2(5X,'----------')/,18X,'Total: ',2(5X,1PE10.3)/)
         IF (NDIM .EQ. 2) THEN
            WRITE (IO, 70) CG(1), CG(2), 0.0
         ELSE
            WRITE (IO, 70) (CG(I),I=1,3)
   70       FORMAT (5x,'MASS PROPERTIES--- (Inertias at centroid)'/
     *         5X,' Xc = ',1PE10.3,'   Yc = ',1PE10.3,
     *         '   Zc = ',1PE10.3)
         END IF
         WRITE (IO, 80) (ZI(I),I=1,3)
   80    FORMAT (5X,'Ixx = ',1PE10.3,'  Iyy = ',1PE10.3,
     *      '  Izz = ',1PE10.3)
         IF (NDIM .EQ. 2 .AND. .NOT. AXI) THEN
            WRITE (IO, 90) ZI(4)
   90       FORMAT (5X,'Ixy = ',1PE10.3/)
         ELSE IF (NDIM .EQ. 3) THEN
            WRITE (IO, 100) (ZI(I),I=4,6)
  100       FORMAT (5X,'Ixy = ',1PE10.3,'  Ixz = ',1PE10.3,
     *         '  Iyz = ',1PE10.3/)
         ELSE
            WRITE (IO, 110)
  110       FORMAT (//)
         END IF
         WRITE (IO, 120)LABEL(NDIM),LABEL(NDIM),LABEL(NDIM)
  120    FORMAT (5X,'Mat',T11,'Minimum',T22,'Element',T32,'Maximum',
     *     T43,'Element',T53,'Average',T63,'Number of',/
     *      5X,'Num',T12,A6,T23,'Number',T33,A6,T44,'Number',T54,A6,
     *     T63,'Elements')
         DO 140 ITMP=1,NBLK
            I = MAT(6,ITMP)
            IF (MAT(5,I) .NE. 1) GOTO 140
            WRITE (IO, 130) MAT(1,I),VOLMN(1,I),IELM(1,I),VOLMN(2,I),
     *         IELM(2,I), VOLMN(3,I)/IELM(3,I), IELM(3,I)
  130       FORMAT (I7,4(2X,1PE9.2,1X,I9))
  140    CONTINUE
  160 CONTINUE
      FIRST = .FALSE.
      RETURN
      END

