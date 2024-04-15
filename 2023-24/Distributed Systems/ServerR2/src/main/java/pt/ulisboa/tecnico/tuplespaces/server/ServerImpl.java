package pt.ulisboa.tecnico.tuplespaces.server;

import pt.ulisboa.tecnico.tuplespaces.centralized.contract.TupleSpacesCentralized;
import pt.ulisboa.tecnico.tuplespaces.replicaXuLiskov.contract.TupleSpacesReplicaGrpc;
import pt.ulisboa.tecnico.tuplespaces.replicaXuLiskov.contract.TupleSpacesReplicaXuLiskov;
import pt.ulisboa.tecnico.tuplespaces.server.domain.ServerState;
import io.grpc.stub.StreamObserver;
import java.util.List;


public class ServerImpl extends TupleSpacesReplicaGrpc.TupleSpacesReplicaImplBase {
    private static final boolean DEBUG_FLAG = (System.getProperty("debug") != null);

    private static void debug(String debugMessage) {
        /** Set flag to true to print debug messages.
         * The flag can be set using the -Ddebug command line option. */
        if (DEBUG_FLAG)
            System.err.println(debugMessage);
    }

    private ServerState state = new ServerState();

    @Override
    public synchronized void put(TupleSpacesReplicaXuLiskov.PutRequest request, StreamObserver<TupleSpacesReplicaXuLiskov.PutResponse> responseObserver){
        state.put(request.getNewTuple());
        notifyAll();
        // Send a single response through the stream.
        responseObserver.onNext(TupleSpacesReplicaXuLiskov.PutResponse.getDefaultInstance());
        // Notify the client that the operation has been completed.
        responseObserver.onCompleted();
        debug(String.format("Put Completed: %s" , request.getNewTuple()));
    }

    @Override
    public synchronized void read(TupleSpacesReplicaXuLiskov.ReadRequest request, StreamObserver<TupleSpacesReplicaXuLiskov.ReadResponse> responseObserver){
        String tuple;
        while((tuple = state.read(request.getSearchPattern())) == null){
            try{
                // If the tuple is not found, the server waits until a new tuple is added
                wait();
            } catch (InterruptedException ignored) {}
        }
        responseObserver.onNext(TupleSpacesReplicaXuLiskov.ReadResponse.newBuilder().setResult(tuple).build());
        responseObserver.onCompleted();
        debug(String.format("Read Completed: %s" , request.getSearchPattern()));
    }

    @Override
    public synchronized void takePhase1(TupleSpacesReplicaXuLiskov.TakePhase1Request request, StreamObserver<TupleSpacesReplicaXuLiskov.TakePhase1Response> responseObserver){
        List<String> tuples;
        tuples = state.takePhase1(request.getSearchPattern(), request.getClientId());
        if(tuples != null){
            responseObserver.onNext(TupleSpacesReplicaXuLiskov.TakePhase1Response.newBuilder().addAllReservedTuples(tuples).build());
        }
        else{
            responseObserver.onNext(TupleSpacesReplicaXuLiskov.TakePhase1Response.getDefaultInstance());
        }

        responseObserver.onCompleted();
        debug(String.format("Take Phase 1 Completed:" + tuples));
    }

    @Override
    public synchronized void takePhase1Release(TupleSpacesReplicaXuLiskov.TakePhase1ReleaseRequest request, StreamObserver<TupleSpacesReplicaXuLiskov.TakePhase1ReleaseResponse> responseObserver){
        state.takePhase1Release(request.getClientId());
        notifyAll();
        responseObserver.onNext(TupleSpacesReplicaXuLiskov.TakePhase1ReleaseResponse.newBuilder().build());
        responseObserver.onCompleted();
        debug(String.format("Take Phase 1 Release Completed: %s" , request.getClientId()));
    }

    @Override
    public synchronized void takePhase2(TupleSpacesReplicaXuLiskov.TakePhase2Request request, StreamObserver<TupleSpacesReplicaXuLiskov.TakePhase2Response> responseObserver){
        state.takePhase2(request.getTuple(), request.getClientId());
        responseObserver.onNext(TupleSpacesReplicaXuLiskov.TakePhase2Response.newBuilder().build());
        responseObserver.onCompleted();
        debug(String.format("Take Phase 2 Completed: %s" , request.getTuple()));

    }



    @Override
    public synchronized void getTupleSpacesState(TupleSpacesReplicaXuLiskov.getTupleSpacesStateRequest request, StreamObserver<TupleSpacesReplicaXuLiskov.getTupleSpacesStateResponse> responseObserver){
        List<String> tuples = state.getTupleSpacesState();
        responseObserver.onNext(TupleSpacesReplicaXuLiskov.getTupleSpacesStateResponse.newBuilder().addAllTuple(tuples).build());
        responseObserver.onCompleted();
        debug("Get TupleSpaces State Completed");
        for (String tuple : tuples) {
            debug(String.format((tuple)));
        }
    }

}
