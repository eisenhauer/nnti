!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Zoltan Library for Parallel Applications                                   !
! For more info, see the README file in the top-level Zoltan directory.      ! 
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!  CVS File Information :
!     $RCSfile$
!     $Author$
!     $Date$
!     $Revision$
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

module dr_mm_io
use zoltan
use zoltan_user_data
use mpi_h
use dr_const
use dr_input
use dr_chaco_io
implicit none
private

public :: read_mm_file

! Pin distribution is assumed to be linear always.
integer(Zoltan_INT), parameter :: INITIAL_LINEAR = 1

contains

!/****************************************************************************/
!/****************************************************************************/
!/****************************************************************************/

! Function to read MatrixMarket input; for now, reads only standard 
! MatrixMarket, not MatrixMarket+.

logical function read_mm_file(Proc, Num_Proc, prob, pio_info)
integer(Zoltan_INT) :: Proc, Num_Proc
type(PROB_INFO) :: prob
type(PARIO_INFO) :: pio_info

!  /* Local declarations. */
  character(len=FILENAME_MAX+8) :: mm_fname
  character(len=10) :: mm_rep
  character(len=7) :: mm_field
  character(len=19) :: mm_symm
  integer :: i, rest, cnt, sum, n, share, pin, pinproc, tmp, mynext
  integer :: prev_edge, pincnt, edgecnt

! Values read from matrix market
  integer :: mm_nrow, mm_ncol, mm_nnz, mm_max
  integer, pointer :: mm_iidx(:), mm_jidx(:)
  integer, pointer :: mm_ival(:)
  double precision, pointer :: mm_rval(:)
  complex, pointer :: mm_cval(:)

  integer(Zoltan_INT) :: fp, iostat, allocstat, ierr, status
  integer(Zoltan_INT), pointer ::  vtxdist(:) ! vertex distribution data
  integer(Zoltan_INT), pointer ::  pindist(:) ! pin distribution data
! Local values
  integer(Zoltan_INT) :: npins, nedges, nvtxs
  integer(Zoltan_INT), pointer ::  iidx(:) ! pin data
  integer(Zoltan_INT), pointer ::  jidx(:) ! pin data
  integer i, prev, temp
  logical sorted
!/***************************** BEGIN EXECUTION ******************************/

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Read the MatrixMarket file.

  if (Proc == 0) then

!   Open and read the MatrixMarket file. 
!   Use the MatrixMarket reader from NIST.
    fp = 12
    mm_fname = pio_info%pexo_fname(1:len_trim(pio_info%pexo_fname))//".mtx"
    open(unit=fp,file=mm_fname,action='read',iostat=iostat)
    if (iostat /= 0) then
      print *, "fatal:  Could not open MatrixMarket file ", mm_fname
      read_mm_file = .false.
      return
    endif

    call mminfo(fp, mm_rep, mm_field, mm_symm, mm_nrow, mm_ncol, mm_nnz)

!   read the matrix in on processor 0.
    nullify(mm_ival, mm_cval)
    allocate(mm_iidx(0:mm_nnz-1), stat=allocstat)
    allocate(mm_jidx(0:mm_nnz-1), stat=allocstat)
    allocate(mm_rval(0:mm_nnz-1), stat=allocstat)
    if (allocstat /= 0) then
      print *, "fatal: insufficient memory"
      read_mm_file = .false.
      return
    endif

    mm_max = mm_nnz
    call mmread(fp, mm_rep, mm_field, mm_symm, mm_nrow, mm_ncol, mm_nnz, &
                mm_max, mm_iidx, mm_jidx, mm_ival, mm_rval, mm_cval)

!   Don't need the numerical values.
    if (associated(mm_rval)) deallocate(mm_rval)

!   EBEB Currently, our f90 version assumes the i indices are in sorted order.
!   EBEB If the MM input file is not sorted by i index, we transpose the 
!   EBEB matrix hoping that it was sorted by j index!
!   EBEB This is a hack to enable testing and should be removed in the future
!   EBEB when index sorting has been implemented (see KDDKDD).
    sorted = .true.
    prev = 0
    do i = 0, mm_nnz-1
      if (mm_iidx(i) < prev) then
        sorted = .false.
        exit
      endif
      prev = mm_iidx(i)
    enddo

    if (.not. sorted) then
      print *, 'Warning: Matrix not sorted by i index; transposing matrix!'
      ! Swap iidx and jidx arrays
      do i = 0, mm_nnz-1
        temp = mm_iidx(i)
        mm_iidx(i) = mm_jidx(i)
        mm_jidx(i) = temp
      enddo
    endif

  endif ! Proc == 0

! BCast pertinent info to all procs.
  call MPI_Bcast(mm_ncol, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
  call MPI_Bcast(mm_nrow, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
  call MPI_Bcast(mm_nnz, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!  Assume linear distribution of vertices.
!  Calculate uniform vertex distribution.
  if (.not. associated(vtxdist)) then
    allocate(vtxdist(0:Num_Proc), stat=allocstat)
    if (allocstat /= 0) then
      print *, "fatal: insufficient memory"
      read_mm_file = .false.
      return
    endif
  endif
  vtxdist(0) = 0
  rest = mm_ncol
  do i=0, Num_Proc-1
    n = rest/(Num_Proc-i)
    vtxdist(i+1) = vtxdist(i) + n
    rest = rest - n
  end do

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Create elements associated with owned vertices.
! Initialize Mesh structure for MM mesh. 
  nvtxs = vtxdist(Proc+1) - vtxdist(Proc)
  Mesh%num_elems = nvtxs
  Mesh%elem_array_len = Mesh%num_elems + 5
  Mesh%num_dims = 0
  Mesh%num_el_blks = 1

  allocate(Mesh%eb_ids(0:Mesh%num_el_blks-1), &
           Mesh%eb_cnts(0:Mesh%num_el_blks-1), &
           Mesh%eb_nnodes(0:Mesh%num_el_blks-1), &
           Mesh%eb_nattrs(0:Mesh%num_el_blks-1), stat=allocstat)
  if (allocstat /= 0) then
    print *, "fatal: insufficient memory"
    read_mm_file = .false.
    return
  endif

  allocate(Mesh%eb_names(0:Mesh%num_el_blks-1),stat=allocstat)
  if (allocstat /= 0) then
    print *, "fatal: insufficient memory"
    read_mm_file = .false.
    return
  endif

  Mesh%eb_ids(0) = 1
  Mesh%eb_cnts(0) = nvtxs
! Assume no coordinates for MatrixMarket vertices.
  Mesh%eb_nnodes(0) = 0
  Mesh%eb_nattrs(0) = 0
  Mesh%eb_names(0) = "mm"

! allocate the element structure array.
  allocate(Mesh%elements(0:Mesh%elem_array_len-1), stat=allocstat)
  if (allocstat /= 0) then
    print *, "fatal: insufficient memory"
    read_mm_file = .false.
    return
  endif

! intialize all of the element structs as unused by
! setting the globalID to -1
  do i = 0, Mesh%elem_array_len-1
    call initialize_element(Mesh%elements(i))
  end do

  do i = 0,nvtxs-1
    Mesh%elements(i)%globalID = vtxdist(Proc) + i
    Mesh%elements(i)%elem_blk = 0
    Mesh%elements(i)%my_part = Proc
    Mesh%elements(i)%perm_value = -1
    Mesh%elements(i)%invperm_value = -1
    Mesh%elements(i)%cpu_wgt = 1
    Mesh%elements(i)%mem_wgt = 1
  enddo
  if (associated(vtxdist)) deallocate(vtxdist)
  
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!  Calculate pin distribution 
!  Send pins to owning processor

  allocate(pindist(0:Num_Proc))
  if (pio_info%init_dist_pins == INITIAL_LINEAR) then
    share = mm_nnz / Num_Proc;
    rest = mm_nnz - (Num_Proc * share);
    sum = 0
    do i = 0, Num_Proc
      pindist(i) = sum
      sum = sum + share
      if (i < rest) sum = sum + 1
    enddo
  else
    print *, "INITIAL_LINEAR IS ONLY DISTRIBUTION SUPPORTED FOR PINS"
    read_mm_file = .false.
    return
  endif

! Allocate arrays to receive pins.
  npins = pindist(Proc+1) - pindist(Proc)
  allocate(iidx(0:npins-1),jidx(0:npins-1),stat=allocstat)

  if (Proc == 0) then
!   Fill communication buffer with pins to be sent.
!   Assume INITIAL_LINEAR pin distribution.
    do i = 1, Num_Proc-1
      call MPI_Send(mm_iidx(pindist(i)), (pindist(i+1)-pindist(i)), MPI_INTEGER, &
                    i, 1, MPI_COMM_WORLD, ierr)
      call MPI_Send(mm_jidx(pindist(i)), (pindist(i+1)-pindist(i)), MPI_INTEGER, &
                    i, 2, MPI_COMM_WORLD, ierr)
    enddo
!   Copy Proc zero's pins.
    do i = 0, pindist(1)-1
      iidx(i) = mm_iidx(i)
      jidx(i) = mm_jidx(i)
    enddo
  else
    call MPI_Recv(iidx, npins, MPI_INTEGER, 0, 1, MPI_COMM_WORLD, &
                  status, ierr)
    call MPI_Recv(jidx, npins, MPI_INTEGER, 0, 2, MPI_COMM_WORLD, &
                  status, ierr)
  endif
     
  if (associated(pindist)) deallocate(pindist)
  if (associated(mm_iidx)) deallocate(mm_iidx)
  if (associated(mm_jidx)) deallocate(mm_jidx)

! KDDKDD
! KDDKDD We assume the MatrixMarket file is sorted by row numbers.
! KDDKDD This condition is true for all our test cases.
! KDDKDD If this condition were not true, we would have to sort the
! KDDKDD pins here based on their iidx values.
! KDDKDD 

! Count number of unique edge IDs on this processor.
  prev_edge = -1
  nedges = 0
  do i = 0, npins-1
    if (iidx(i) .ne. prev_edge) nedges = nedges + 1
    if (iidx(i) < prev_edge) then
!     KDDKDD see note above.
      print *, "Error in MatrixMarket file.  Entries are not sorted by I index."
      read_mm_file = .false.
      return
    endif
    prev_edge = iidx(i)
  enddo
  Mesh%nhedges = nedges

! Allocate the index and pin arrays.
  allocate(Mesh%hgid(0:nedges-1),Mesh%hindex(0:nedges), &
           Mesh%hvertex(0:npins-1),stat=allocstat)

! Fill the index and pin arrays.
  pincnt = 0
  edgecnt = 0
  prev_edge = -1
  do i = 0, npins-1
    if (iidx(i) .ne. prev_edge) then
      Mesh%hindex(edgecnt) = pincnt
      Mesh%hgid(edgecnt) = iidx(i)
      edgecnt = edgecnt + 1
      prev_edge = iidx(i)
    endif
    Mesh%hvertex(pincnt) = jidx(i)
    pincnt = pincnt + 1
  enddo
  Mesh%hindex(nedges) = npins

! Almost done.
  if (associated(iidx)) deallocate(iidx)
  if (associated(jidx)) deallocate(jidx)
  read_mm_file = .true.
end function read_mm_file

end module dr_mm_io
