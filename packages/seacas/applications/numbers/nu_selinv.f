C $Id: selinv.f,v 1.2 1999/02/16 21:38:01 gdsjaar Exp $
C $Log: selinv.f,v $
C Revision 1.2  1999/02/16 21:38:01  gdsjaar
C Converted to read exodusII database format.  Somewhat tested, not
C ready for production yet.
C
C Revision 1.1.1.1  1991/02/21 15:45:29  gdsjaar
C NUMBERS: Greg Sjaardema, initial Unix release
C
c Revision 1.1  1991/02/21  15:45:28  gdsjaar
c Initial revision
c
      SUBROUTINE SELINV (TIMES, ITMSEL, NINTV)
      DIMENSION TIMES(*)
      LOGICAL ITMSEL(*)
      CHARACTER*16 ENGNOT, STRA, STRB
      CHARACTER*80 STRTMP
C
      include 'nu_ptim.blk'
C
      NLAST  = 0
      NUMSEL = 0
C
C      IFIRST = LOCRL (STMIN, NSTEP, ITMSEL, TIMES)
C      ILAST  = LOCRL (STMAX, NSTEP, ITMSEL, TIMES)
      IFIRST = LOCREA (STMIN, NSTEP, TIMES)
      ILAST  = LOCREA (STMAX, NSTEP, TIMES)
      NBETWN = ILAST - IFIRST
      CALL INILOG (NSTEP, .FALSE., ITMSEL)

      IF (NINTV .EQ. 0) THEN
         CALL PRTERR ('WARNING', 'No time steps selected.')
         RETURN
      ELSE IF (NINTV .LT. 0) THEN
C - Include tmin step
         II = -NINTV - 1
         II = MAX(1, II)
         RINC = FLOAT(NBETWN) / FLOAT(II)
         IB = IFIRST
         IE = ILAST
      ELSE
         II = NINTV
         RINC = FLOAT(NBETWN) / FLOAT(II)
         IB = IFIRST + INT(RINC + 0.5)
         IE = ILAST
      END IF

      NUMSEL = 0
      RTIM = FLOAT(IB)
  100 CONTINUE
         ITMSEL(INT(RTIM+0.5)) = .TRUE.
         NUMSEL = NUMSEL + 1
         RTIM = MIN( RTIM + RINC, FLOAT(IE))
         IF (NUMSEL .LT. ABS(NINTV)) GO TO 100
      ITMSEL(ILAST) = .TRUE.
      STRA = ENGNOT(STMIN,2)
      STRB = ENGNOT(STMAX,2)
      WRITE (STRTMP, 40) NUMSEL, STRA, STRB
      CALL SQZSTR(STRTMP, LSTR)
      WRITE (*, 50) STRTMP(:LSTR)
      RETURN
   40 FORMAT (I5,' Steps Selected from ',A16,' to ',A16)
   50 FORMAT (/,5X,A)
      END
