// @HEADER
// ***********************************************************************
//
//               Java Implementation of the Petra Library
//                 Copyright (2004) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ***********************************************************************
// @HEADER

package Jpetra;

import java.util.Set;
import java.util.Iterator;
import java.util.Map;

/**
 *
 * @author  Jason Cross
 */
public class CisMatrix extends JpetraObject {
    public static final boolean ROW_ORIENTED = true;
    public static final boolean COL_ORIENTED = false;
    private Graph graph;
    private VectorSpace primaryVectorSpace;
    private boolean rowOriented;
    private boolean filled;
    private int[] intValues;
    private double[] doubleValues;
    private int[] numEntries;  // number of entries per array
    private JpetraTreeMap OuterTree;
    private int numTotalEntries;
    
    public CisMatrix(VectorSpace primaryVectorSpace, boolean rowOriented) {
        this.filled = false;
        this.primaryVectorSpace=primaryVectorSpace;
        //this.graph = new Graph(this.primaryVectorSpace);
        this.rowOriented = rowOriented;
        this.OuterTree = new JpetraTreeMap();
    }
    
    public void insertEntries(int globalRowColId, int[] indices, int[] entries) {
        if (this.filled) {
            this.println("ERR", "insertEntiries cannot be called after fillComplete().");
        }
        
        // need to check if I own the globalRowColId
        
        // need to see if the JpetraTreeMap exists for the specified row/col
        JpetraTreeMap rowColTreeMap;
        if (!this.OuterTree.containsKey(new Integer(globalRowColId))) {
            rowColTreeMap = new JpetraTreeMap();
            this.println("STD", "globalRowCol does not exist, creating...");
            this.OuterTree.put(new Integer(globalRowColId), rowColTreeMap); 
        }
        else {
            this.println("STD", "globalRowCol exists, setting rowColTreeMap to existing JpetraTreeMap...");
            rowColTreeMap = (JpetraTreeMap) OuterTree.get(new Integer(globalRowColId));
        }
        
        // now that we know the row/col exists, insert entries into the row/col
        // and sum them with any pre-existing entries
        this.println("STD", "CisMatrix is internally inserting entries...");
        Object temp;
        int toInsert;
        for(int i=0; i < indices.length; i++) {
            this.println("STD", "Inserting index " + indices[i]);
            temp = rowColTreeMap.get(new Integer(indices[i]));
            if (temp == null) {
                    this.println("STD", "Index " + indices[i] + " does not exist, creating...");
                this.numTotalEntries++;
                rowColTreeMap.put(new Integer(indices[i]), new Integer(entries[i]));
            }
            else {
                this.println("STD", "Index " + indices[i] + " does exists, adding to prevous value...");
                rowColTreeMap.put(new Integer(indices[i]), new Integer(entries[i] + ((Integer) temp).intValue()));
            }
        }
    }
    
    public void fillComplete() {
        if (this.filled) {
            this.print("ERR", "fillComplete() may only be called once.");
        }
        this.filled = true;
        this.intValues = new int[this.numTotalEntries];
        this.numEntries = new int[primaryVectorSpace.getNumMyGlobalEntries()];
        
        Set outerKeysValues = this.OuterTree.entrySet();
        Iterator outerIterator = outerKeysValues.iterator();
        Map.Entry outerMapEntry;
        
        JpetraTreeMap innerTree;
        Set innerKeysValues;
        Iterator innerIterator;
        Map.Entry innerMapEntry;
        
        int nextEntryIndex=0;
        int numEntriesColRow;
        int[] tempGraph = new int[this.numTotalEntries];
        int i=0;
        while(outerIterator.hasNext()) {
            this.println("STD", "Doing an outer loop...");
            outerMapEntry = (Map.Entry) outerIterator.next();
            innerTree = (JpetraTreeMap) outerMapEntry.getValue();
            innerKeysValues = innerTree.entrySet();
            innerIterator = innerKeysValues.iterator();
            numEntriesColRow = 0;
            while (innerIterator.hasNext()) {
                this.println("STD", "Doing an inner loop...");
                innerMapEntry = (Map.Entry) innerIterator.next();
                this.intValues[nextEntryIndex] = ((Integer) innerMapEntry.getValue()).intValue();
                tempGraph[nextEntryIndex++] = ((Integer) innerMapEntry.getKey()).intValue();
                numEntriesColRow++;
            }
            this.numEntries[i++] = numEntriesColRow;
        }
        this.graph = new Graph(primaryVectorSpace, tempGraph, this.numEntries);
    }
    
    public void printOut(String iostream) {
        this.println("STD", "Printout() is starting...");
        if (!filled) {
            this.print("ERR", "You must call fillComplete() before calling printOut(String iostream)");
        }
        int i=0;
        this.println("STD", "this.numEntries.length=" + numEntries.length);
        for(int row=0; row < this.numEntries.length; row++) {
            this.println("STD", "Doing a row loop...");
            for(int entry=0; entry < numEntries[row]; entry++) {
                this.println("STD", "\nDoing an entry loop...");
                this.print(iostream, row + " " + graph.getIndex(i) + " " + intValues[i++]);
            }
        }
        this.println("STD", "Printout() has ended...");
    }
}
