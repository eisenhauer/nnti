/** \file hyperlu_util.cpp

    \brief Utilities for HyperLU

    \author Siva Rajamanickam

*/

#include <assert.h>

#include "Isorropia_config.h" // Just for HAVE_MPI

#include "Epetra_MpiComm.h"
#include "Epetra_CrsMatrix.h" 

// EpetraExt includes
#include "EpetraExt_RowMatrixOut.h"

//Teuchos includes
#include "Teuchos_XMLParameterListHelpers.hpp"

//Isorropia includes
#include "Isorropia_Epetra.hpp"
#include "Isorropia_EpetraRedistributor.hpp"
#include "Isorropia_EpetraPartitioner.hpp"

#include "hyperlu_util.h"


using namespace std;


// Currently takes onle MpiComm
Epetra_CrsMatrix *balanceAndRedistribute(Epetra_CrsMatrix *A, 
                Teuchos::ParameterList isoList)
{
    int myPID = A->Comm().MyPID();

    // Debug [
    Epetra_Map ARowMap = A->RowMap();
    int nrows = ARowMap.NumMyElements();
    int *rows = ARowMap.MyGlobalElements();
    // ]

    // ==================== Symbolic factorization =========================
    // 1. Partition and redistribute [
    Isorropia::Epetra::Partitioner *partitioner = new 
                            Isorropia::Epetra::Partitioner(A, isoList, false);
    partitioner->partition();

    Isorropia::Epetra::Redistributor rd(partitioner);
    Epetra_CrsMatrix *newA;
    rd.redistribute(*A, newA);
    // ]
    EpetraExt::RowMatrixToMatlabFile("A.mat", *newA);

    delete partitioner;
    return newA;
}

/* TODO : Do this only for Debug ? */
void checkMaps(Epetra_CrsMatrix *A)
{
    // Get column map
    Epetra_Map AColMap = A->ColMap();
    int ncols = AColMap.NumMyElements();
    int *cols = AColMap.MyGlobalElements();

    // Get domain map
    Epetra_Map ADomainMap =  A->DomainMap();
    int nelems = ADomainMap.NumMyElements();
    int *dom_cols = ADomainMap.MyGlobalElements();

    // Get range map
    Epetra_Map ARangeMap =  A->RangeMap();
    int npts = ARangeMap.NumMyElements();
    int *ran_cols = ARangeMap.MyGlobalElements();

    // Get row map
    Epetra_Map ARowMap = A->RowMap();
    int nrows = ARowMap.NumMyElements();
    int *rows = ARowMap.MyGlobalElements();

    //cout <<"In PID ="<< A->Comm().MyPID() <<" #cols="<< ncols << " #rows="<< 
        //nrows <<" #domain elems="<< nelems <<" #range elems="<< npts << endl;
    // See if domain map == range map == row map
    for (int i = 0; i < nelems ; i++)
    {
        // Will this always be the case ? We will find out if assertion fails !
        assert(dom_cols[i] == ran_cols[i]);
        assert(rows[i] == ran_cols[i]);
    }
}

// TODO: SNumGlobalCols never used
void findLocalColumns(Epetra_CrsMatrix *A, int *gvals, int &SNumGlobalCols)
{

    int n = A->NumGlobalRows();
    // Get column map
    Epetra_Map AColMap = A->ColMap();
    int ncols = AColMap.NumMyElements();
    int *cols = AColMap.MyGlobalElements(); // TODO : Indexing using GID !

    // 2. Find column permutation [
    // Find all columns in this proc
    int *vals = new int[n];       // vector of size n, not ncols
    for (int i = 0; i < n ; i++) 
    {
        vals[i] = 0;
        gvals[i] = 0;
    }

    // Null columns in A are not part of any proc
    for (int i = 0; i < ncols ; i++)
    {
        vals[cols[i]] = 1;        // Set to 1 for locally owned columns
    }

    // Bottleneck?: Compute the column permutation
    A->Comm().SumAll(vals, gvals, n);

    SNumGlobalCols = 0;
    for (int i = 0; i < n ; i++)
    {
        //cout << gvals[i] ;
        if (gvals[i] > 1)
            SNumGlobalCols++;
    }
    //cout << endl;
    //cout << "Snum Global cols=" << SNumGlobalCols << endl;

    delete vals;
    return;
}

// This function uses a very simple tie-breaking heuristic to find a
// narrow separator from a wide separator. The vertices in the proc with
// smaller procID will become part of the separator
// TODO : This assumes symmetry I guess, Check
void findNarrowSeparator(Epetra_CrsMatrix *A, int *gvals)
{
    int nentries;
    double *values;
    int *indices;
    int n = A->NumGlobalRows();

    // Get row map
    int myPID = A->Comm().MyPID();
    int numProcs = A->Comm().NumProc();
    Epetra_Map rMap = A->RowMap();
    Epetra_Map cMap = A->ColMap();
    int *rows = rMap.MyGlobalElements();
    int relems = rMap.NumMyElements();

    int *vals = new int[n];       // vector of size n, not ncols
    int *allGIDs = new int[n];       // vector of size n, not ncols
    for (int i = 0; i < n ; i++) // initialize to zero
    {
        vals[i] = 0;
    }

    // Rows are uniquely owned, so this will work
    for (int i = 0; i < relems ; i++)
    {
        vals[rows[i]] = myPID;        // I own relems[i]
    }


    // **************** Collective communication **************
    // This will not scale well for very large number of nodes
    // But on the node this should be fine
    A->Comm().SumAll(vals, allGIDs, n);

    // At this point all procs know who owns what rows

    int gid, err, cgid;
    for (int i = 0; i < relems; i++)
    {
        gid = rows[i];
        //cout << "PID=" << myPID << " " << "rowid=" << gid ;
        if (gvals[gid] != 1)
        {
            //cout << " in the sep ";
            bool movetoBlockDiagonal = false;
            err = A->ExtractMyRowView(i, nentries, values, indices);
            //cout << " with nentries= "<< nentries;

            assert(nentries != 0);
            for (int j = 0; j < nentries; j++)
            {
                cgid = cMap.GID(indices[j]);
                assert(cgid != -1);
                if (gvals[cgid] == 1 || allGIDs[cgid] == myPID)
                    continue; // simplify the rest

                /*if (numProcs == 2)
                {*/
                    if (allGIDs[cgid] < myPID)
                    {
                        // row cgid is owned by a proc with smaller PID
                        movetoBlockDiagonal = true;
                        //cout << "\t mving to diag because of column" << cgid;
                    }
                    else
                    {
                        //cout << "\tNo problem with cgid=" << cgid << "in sep";
                    }
                /*}
                else
                {
                    if (myPID == 0 && allGIDs[cgid] == numProcs-1)
                    {
                        // row cgid is owned by a proc with smaller PID
                        movetoBlockDiagonal = true;
                        cout << "\t I AM HERE mving to diag because of column" << cgid;
                    }
                    else if (myPID == numProcs-1 && allGIDs[cgid] == 0)
                    {
                        cout << "\t I AM HERE to continue " << cgid;
                        continue;
                    }
                    else if (allGIDs[cgid] < myPID)
                    {
                        // row cgid is owned by a proc with smaller PID
                        movetoBlockDiagonal = true;
                        cout << "\t mving to diag because of column" << cgid;
                    }
                    else
                    {
                        //cout << "\tNo problem with cgid=" << cgid << "in sep";
                    }
                }*/
            }
            if (movetoBlockDiagonal)
            {
                //cout << "Moving to Diagonal";
                gvals[gid] = 1;
            }
        }
        else
        {
            // do nothing, in the diagonal block already
            //cout << "In the diagonal block";
        }
        //cout << endl;
    }
    delete vals;
    delete allGIDs;

}

void findBlockElems(int nrows, int *rows, int *gvals, int Lnr, int *LeftElems, 
        int Rnr, int *RightElems, string s1, string s2)
{
 
    int gid;
    int rcnt = 0, lcnt = 0;
    // Assemble ids in two arrays
    ostringstream ssmsg1;
    ostringstream ssmsg2;

    ssmsg1 << s1;
    ssmsg2 << s2;
    for (int i = 0; i < nrows; i++)
    {
        gid = rows[i];
        assert (gvals[gid] >= 1);
        if (gvals[gid] == 1)
        {
            assert(lcnt < Lnr);
            LeftElems[lcnt++] = gid;
            ssmsg1 << gid << " ";
        }
        else
        {
            assert(rcnt < Rnr);
            RightElems[rcnt++] = gid; 
            ssmsg2 << gid << " ";
        }
    }
    cout << ssmsg1.str() << endl;
    cout << ssmsg2.str() << endl; // TODO: Enable it only in debug mode
    ssmsg1.clear(); ssmsg1.str("");
    ssmsg2.clear(); ssmsg2.str("");

    assert(lcnt == Lnr);
    assert(rcnt == Rnr);
}
