// Skeleton class generated by rmic, do not edit.
// Contents subject to change without notice.

package CCJ;

public final class MyList_Skel
    implements java.rmi.server.Skeleton
{
    private static final java.rmi.server.Operation[] operations = {
	new java.rmi.server.Operation("void add(int, int, java.io.Serializable)"),
	new java.rmi.server.Operation("void confirmedAdd(int, int, java.io.Serializable)"),
	new java.rmi.server.Operation("java.io.Serializable futureAdd(java.io.Serializable)"),
	new java.rmi.server.Operation("java.io.Serializable rendezVousAdd(int, int, java.io.Serializable)")
    };
    
    private static final long interfaceHash = -1801133939574880270L;
    
    public java.rmi.server.Operation[] getOperations() {
	return (java.rmi.server.Operation[]) operations.clone();
    }
    
    public void dispatch(java.rmi.Remote obj, java.rmi.server.RemoteCall call, int opnum, long hash)
	throws java.lang.Exception
    {
	if (opnum < 0) {
	    if (hash == 1629625854807050535L) {
		opnum = 0;
	    } else if (hash == 7133574785109645466L) {
		opnum = 1;
	    } else if (hash == -7971877956833888253L) {
		opnum = 2;
	    } else if (hash == 5138648816406637420L) {
		opnum = 3;
	    } else {
		throw new java.rmi.UnmarshalException("invalid method hash");
	    }
	} else {
	    if (hash != interfaceHash)
		throw new java.rmi.server.SkeletonMismatchException("interface hash mismatch");
	}
	
	CCJ.MyList server = (CCJ.MyList) obj;
	switch (opnum) {
	case 0: // add(int, int, Serializable)
	{
	    int $param_int_1;
	    int $param_int_2;
	    java.io.Serializable $param_Serializable_3;
	    try {
		java.io.ObjectInput in = call.getInputStream();
		$param_int_1 = in.readInt();
		$param_int_2 = in.readInt();
		$param_Serializable_3 = (java.io.Serializable) in.readObject();
	    } catch (java.io.IOException e) {
		throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
	    } catch (java.lang.ClassNotFoundException e) {
		throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
	    } finally {
		call.releaseInputStream();
	    }
	    server.add($param_int_1, $param_int_2, $param_Serializable_3);
	    try {
		call.getResultStream(true);
	    } catch (java.io.IOException e) {
		throw new java.rmi.MarshalException("error marshalling return", e);
	    }
	    break;
	}
	    
	case 1: // confirmedAdd(int, int, Serializable)
	{
	    int $param_int_1;
	    int $param_int_2;
	    java.io.Serializable $param_Serializable_3;
	    try {
		java.io.ObjectInput in = call.getInputStream();
		$param_int_1 = in.readInt();
		$param_int_2 = in.readInt();
		$param_Serializable_3 = (java.io.Serializable) in.readObject();
	    } catch (java.io.IOException e) {
		throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
	    } catch (java.lang.ClassNotFoundException e) {
		throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
	    } finally {
		call.releaseInputStream();
	    }
	    server.confirmedAdd($param_int_1, $param_int_2, $param_Serializable_3);
	    try {
		call.getResultStream(true);
	    } catch (java.io.IOException e) {
		throw new java.rmi.MarshalException("error marshalling return", e);
	    }
	    break;
	}
	    
	case 2: // futureAdd(Serializable)
	{
	    java.io.Serializable $param_Serializable_1;
	    try {
		java.io.ObjectInput in = call.getInputStream();
		$param_Serializable_1 = (java.io.Serializable) in.readObject();
	    } catch (java.io.IOException e) {
		throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
	    } catch (java.lang.ClassNotFoundException e) {
		throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
	    } finally {
		call.releaseInputStream();
	    }
	    java.io.Serializable $result = server.futureAdd($param_Serializable_1);
	    try {
		java.io.ObjectOutput out = call.getResultStream(true);
		out.writeObject($result);
	    } catch (java.io.IOException e) {
		throw new java.rmi.MarshalException("error marshalling return", e);
	    }
	    break;
	}
	    
	case 3: // rendezVousAdd(int, int, Serializable)
	{
	    int $param_int_1;
	    int $param_int_2;
	    java.io.Serializable $param_Serializable_3;
	    try {
		java.io.ObjectInput in = call.getInputStream();
		$param_int_1 = in.readInt();
		$param_int_2 = in.readInt();
		$param_Serializable_3 = (java.io.Serializable) in.readObject();
	    } catch (java.io.IOException e) {
		throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
	    } catch (java.lang.ClassNotFoundException e) {
		throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
	    } finally {
		call.releaseInputStream();
	    }
	    java.io.Serializable $result = server.rendezVousAdd($param_int_1, $param_int_2, $param_Serializable_3);
	    try {
		java.io.ObjectOutput out = call.getResultStream(true);
		out.writeObject($result);
	    } catch (java.io.IOException e) {
		throw new java.rmi.MarshalException("error marshalling return", e);
	    }
	    break;
	}
	    
	default:
	    throw new java.rmi.UnmarshalException("invalid method number");
	}
    }
}
