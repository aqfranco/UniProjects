package pt.ulisboa.tecnico.tuplespaces.server;

import pt.ulisboa.tecnico.tuplespaces.centralized.contract.TupleSpacesCentralized;
import pt.ulisboa.tecnico.tuplespaces.server.domain.ServerState;
import pt.ulisboa.tecnico.tuplespaces.centralized.contract.TupleSpacesGrpc;
import pt.ulisboa.tecnico.tuplespaces.centralized.contract.TupleSpacesCentralized.ReadResponse;
import io.grpc.stub.StreamObserver;
import java.util.List;


public class ServerImpl extends TupleSpacesGrpc.TupleSpacesImplBase {
    private static final boolean DEBUG_FLAG = (System.getProperty("debug") != null);

    private static void debug(String debugMessage) {
        /** Set flag to true to print debug messages.
         * The flag can be set using the -Ddebug command line option. */
        if (DEBUG_FLAG)
            System.err.println(debugMessage);
    }

    private ServerState state = new ServerState();

    @Override
    public synchronized void put(TupleSpacesCentralized.PutRequest request, StreamObserver<TupleSpacesCentralized.PutResponse> responseObserver){
        state.put(request.getNewTuple());
        notifyAll();
        // Send a single response through the stream.
        responseObserver.onNext(TupleSpacesCentralized.PutResponse.getDefaultInstance());
        // Notify the client that the operation has been completed.
        responseObserver.onCompleted();
        debug(String.format("Put Completed: %s" , request.getNewTuple()));
    }

    @Override
    public synchronized void read(TupleSpacesCentralized.ReadRequest request, StreamObserver<TupleSpacesCentralized.ReadResponse> responseObserver){
        String tuple;
        while((tuple = state.read(request.getSearchPattern())) == null){
            try{
                // If the tuple is not found, the server waits until a new tuple is added
                wait();
            } catch (InterruptedException ignored) {}
        }
        responseObserver.onNext(TupleSpacesCentralized.ReadResponse.newBuilder().setResult(tuple).build());
        responseObserver.onCompleted();
        debug(String.format("Read Completed: %s" , request.getSearchPattern()));
    }

    @Override
    public synchronized void take(TupleSpacesCentralized.TakeRequest request, StreamObserver<TupleSpacesCentralized.TakeResponse> responseObserver){
        String tuple;
        while((tuple = state.take(request.getSearchPattern())) == null){
            try{
                // If the tuple is not found, the server waits until a new tuple is added
                wait();
            } catch (InterruptedException ignored) {}
        }
        responseObserver.onNext(TupleSpacesCentralized.TakeResponse.newBuilder().setResult(tuple).build());
        responseObserver.onCompleted();
        debug(String.format("Take Completed: %s" , request.getSearchPattern()));
    }

    @Override
    public synchronized void getTupleSpacesState(TupleSpacesCentralized.getTupleSpacesStateRequest request, StreamObserver<TupleSpacesCentralized.getTupleSpacesStateResponse> responseObserver){
        List<String> tuples = state.getTupleSpacesState();
        responseObserver.onNext(TupleSpacesCentralized.getTupleSpacesStateResponse.newBuilder().addAllTuple(tuples).build());
        responseObserver.onCompleted();
        debug("Get TupleSpaces State Completed");
        for (String tuple : tuples) {
            debug(String.format((tuple)));
        }
    }

}
