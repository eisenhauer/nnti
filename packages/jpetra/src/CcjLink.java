package Jpetra;

import CCJ.*;
import java.io.Serializable;

/*
 * CCJCom.java
 *
 * Created on Tue June 17 11:59:00 CDT 2003
 */

/**
 * CcjLink provides the wrappers and data representation objects
 * that CcjComm uses to interact with CCJ.
 *
 * @author  Jason Cross 
 */

public class CcjLink extends ColMember {
    
    /**
     * number of vnodees in the <code>group</code>
     */
    private int numVnodes;
    
    /**
     * vnode group; all vnodees belong to the same group
     */
    private ColGroup group;
    
    /**
     * root vnode
     */
    private ColGroupMaster groupMaster;
    
    /**
     * this rank
     */
    private int rank;
    
    /**
     * Contacts the root vnode and joins the vnode
     * <code>group</code> unless <code>this</code> is the root vnode, then it
     * listens for connections.
     */
    CcjLink(ColGroupMaster groupMaster) throws CCJException {
        super();
        this.groupMaster = groupMaster;
    
        this.numVnodes = groupMaster.getNumberOfCpus();
        
        // notice that the group name is a string
        this.groupMaster.addMember("myGroup", this);
        this.group = groupMaster.getGroup("myGroup", this.numVnodes);
        
        this.rank = group.getRank(this);
        
        
        // calls the run method
        // not used by CcjLink
        // begin();
    }
    
    /**
     * Accessor for <code>numVnodes</code> provided by CCJ.
     *
     * @return <code>numVnodes</code>
     */    
    public int getNumVnodes() {
        return this.numVnodes;
    }
    
    /**
     * Accessor for <code>rank</code> provided by CCJ.
     *
     * @return <code>rank</code>
     */
    public int getRank() {
        return this.rank;
    }
    
    /**
     * Wrapper to CCJ <code>barrier</code>.  Causes each vnode in <code>group</group> to wait
     * until all vnodees are ready.
     */
    public void barrier() {
         try {
            barrier(this.group);
         }
         catch (CCJException e) {
            System.err.println("Error in CCJ barrier: " + e);
         }
    }
    
    /**
     * Wrapper to CCJ <code>broadcast</code>.  Broadcasts <code>value</code> from the <code>root</code>
     * vnode to all other vnodees in <code>group</code>.
     */
    public Serializable broadcast(Serializable value, int root) {
        try {
            value = (Serializable) broadcast(group, value, root);
        }
        catch (CCJException e) {
            System.err.println("Error in CCJ broadcast: " + e);
        }
        
        return value;
    }
   
    /**
     * Wrapper to CCJ <code>allGather</code>.  Broadcasts <code>value</code> from the <code>root</code>
     * vnode to all other vnodees in <code>group</code>.
     */    
    public Serializable[] gatherAll(Serializable [] myElements) {
        //used internally by CCJ to handle the <code>gatherAll</code> of arrays
        CcjGatherSerializableArray allElements = new CcjGatherSerializableArray(group.size());
        
        try {
            allGather(group, allElements, myElements);
        }
        catch (CCJException e) {
            System.err.println("Error in CCJ allGather: " + e);
        }
        
        return allElements.getAllElements();
    }

    /**
     * Wrapper to CCJ <code>allGather</code>.  Broadcasts <code>value</code> from the <code>root</code>
     * vnode to all other vnodees in <code>group</code>.
     */    
    public int[] gatherAll(int [] myElements) {
        //used internally by CCJ to handle the <code>gatherAll</code> of arrays
        CcjGatherIntArray allElements = new CcjGatherIntArray(group.size());
        
        try {
            allGather(group, allElements, myElements);
        }
        catch (CCJException e) {
            System.err.println("Error in CCJ allGather: " + e);
        }
        
        return allElements.getAllElements();
    }
    
    /**
     * Wrapper to CCJ <code>allGather</code>.
     */      
    public double[] gatherAll(double [] myElements) {
        //used internally by CCJ to handle the <code>gatherAll</code> of arrays
        CcjGatherDoubleArray allElements = new CcjGatherDoubleArray(group.size());
        
        try {
            allGather(group, allElements, myElements);
        }
        catch (CCJException e) {
            System.err.println("Error in CCJ allGather: " + e);
        }
        
        return allElements.returnAllElements();
    }

    /**
     * Wrapper to CCJ <code>allReduce</code>.
     */      
    public double[] sumAll(double[] partialSums) {
        //used internally by CCJ to handle the <code>allReduce</code> of arrays
        CcjReduceAllSumDoubleArray doublesAdder = new CcjReduceAllSumDoubleArray();
        
        double[] toReturn = null;
        try {
            toReturn = (double[]) allReduce(group, partialSums, doublesAdder);
        }
        catch (CCJException e) {
            System.err.println("Error in CCJ doubleSumAll: " + e);
        }
        
        return toReturn;
    }

    /**
     * Wrapper to CCJ <code>allReduce</code>.
     */     
    public int[] sumAll(int[] partialSums) {
        //used internally by CCJ to handle the <code>allReduce</code> of arrays    
        CcjReduceAllSumIntArray intsAdder = new CcjReduceAllSumIntArray();
        
        int[] toReturn=null;
        try {
            toReturn = (int[]) allReduce(group, partialSums, intsAdder);
        }
        catch (CCJException e) {
            System.err.println("Error in CCJ intSumAll: " + e);
        }
        
        return toReturn;
    }

    /**
     * Wrapper to CCJ <code>allGather</code> then calls <code>CcjGatherDoubleArray.getMaxs()</code>.
     */     
    public double[] maxAll(double [] partialMaxs) {
        //used internally by CCJ to handle the <code>gatherAll</code> of arrays    
        CcjGatherDoubleArray globalMaxs = new CcjGatherDoubleArray(group.size());
        
        try {
            allGather(group, globalMaxs, partialMaxs);
        }
        catch (CCJException e) {
            System.err.println("Error in CCJ maxAll: " + e);
        }
        
        return globalMaxs.getMaxs();
    }

    /**
     * Wrapper to CCJ <code>allGather</code> then calls <code>CcjGatherIntArray.getMaxs()</code>.
     */    
    public int[] maxAll(int [] partialMaxs) {
        //used internally by CCJ to handle the <code>gatherAll</code> of arrays
        CcjGatherIntArray globalMaxs = new CcjGatherIntArray(group.size());
        
        try {
            allGather(group, globalMaxs, partialMaxs);
        }
        catch (CCJException e) {
            System.err.println("Error in CCJ maxAll: " + e);
        }
        
        return globalMaxs.getMaxs();
    }

    /**
     * Wrapper to CCJ <code>allGather</code> then calls <code>CcjGatherDoubleArray.getMins()</code>.
     */     
    public double[] minAll(double [] partialMins) {
        //used internally by CCJ to handle the <code>gatherAll</code> of arrays
        CcjGatherDoubleArray globalMins = new CcjGatherDoubleArray(group.size());
        
        try {
            allGather(group, globalMins, partialMins);
        }
        catch (CCJException e) {
            System.err.println("Error in CCJ minAll: " + e);
        }
        
        return globalMins.getMins();
    }

    /**
     * Wrapper to CCJ <code>allGather</code> then calls <code>CcjGatherIntArray.getMins()</code>.
     */         
    public int[] minAll(int [] partialMins) { 
        //used internally by CCJ to handle the <code>gatherAll</code> of arrays
        CcjGatherIntArray globalMins = new CcjGatherIntArray(group.size());
        
        try {
            allGather(group, globalMins, partialMins);
        }
        catch (CCJException e) {
            System.err.println("Error in CCJ minAll: " + e);
        }
        
        return globalMins.getMins();
    }

    /**
     * Wrapper to CCJ <code>allGather</code> then calls <code>CcjGatherDoubleArray.scanSums()</code>.
     */    
    public double[] scanSums(double [] myElements) {
        //used internally by CCJ to handle the <code>gatherAll</code> of arrays
        CcjGatherDoubleArray globalSums = new CcjGatherDoubleArray(group.size());
        
        try {
            allGather(group, globalSums, myElements);
        }
        catch (CCJException e) {
            System.err.println("Error in CCJ scanSums: " + e);
        }
        
        return globalSums.scanSums(rank);
    }

    /**
     * Wrapper to CCJ <code>allGather</code> then calls <code>CcjGatherIntArray.scanSums()</code>.
     */      
    public int[] scanSums(int [] myElements) {
        //used internally by CCJ to handle the <code>gatherAll</code> of arrays
        CcjGatherIntArray globalSums = new CcjGatherIntArray(group.size());
        
        try {
            allGather(group, globalSums, myElements);
        }
        catch (CCJException e) {
            System.err.println("Error in CCJ scanSums: " + e);
        }
        
        return globalSums.scanSums(rank);
    }
        
        //not used
        public void run() {
	    //empty
	    }
    }