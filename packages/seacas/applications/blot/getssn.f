      subroutine getssn(ia, ierr)

      include 'dbase.blk'

      integer ia(*)
      
      CALL MDFIND ('LTSESS', KLTSSS, LESSEL)
      CALL MDFIND ('IXNESS', KIXNSS, NUMESS)
      CALL MDFIND ('IDESS',  KIDSS,  NUMESS)
      CALL MDFIND ('LTNNSS', KLTNNN, LESSEL)
      CALL MDFIND ('LTNESS', KLTNSS, LESSNL)
      CALL MDFIND ('NNESS',  KNNSS,  NUMESS)

c ...Convert sides to nodes.... a(kltsss), 
C offset into element list for current side set
      isoff = 0
C     node count for current side set
      nodcnt = 0
      do i=0,numess-1
C     update index array            
         ia(kixnss+i)=nodcnt+1
C     get num of sides & df            
         call exgsp(ndb,ia(kidss+i),nsess,ndess,ierr)
      if (ierr .gt. 0) goto 170 
         
C     get side set nodes
         call exgssn(ndb,ia(kidss+i),ia(kltnnn+isoff),
     &        ia(kltnss+nodcnt),ierr) 
      if (ierr .gt. 0) goto 170
      nness = 0
C     sum node counts to calculate next index
         do ii=0,nsess-1 
            nness=nness+ia(kltnnn+isoff+ii)
         end do
         ia(knnss+i)=nness
         nodcnt=nodcnt+nness
         isoff=isoff+nsess
      end do
      
 170  continue
      return
      end
      
