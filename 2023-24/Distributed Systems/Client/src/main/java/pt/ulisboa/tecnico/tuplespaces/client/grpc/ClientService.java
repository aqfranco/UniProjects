package pt.ulisboa.tecnico.tuplespaces.client.grpc;

import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import io.grpc.StatusRuntimeException;
import pt.ulisboa.tecnico.tuplespaces.client.util.OrderedDelayer;
import pt.ulisboa.tecnico.tuplespaces.replicaXuLiskov.contract.TupleSpacesReplicaGrpc;
import pt.ulisboa.tecnico.tuplespaces.replicaXuLiskov.contract.TupleSpacesReplicaXuLiskov;

import java.util.*;

import static java.lang.System.exit;

public class ClientService {
    ManagedChannel[] channels;
    private int numServers;
    List<TupleSpacesReplicaGrpc.TupleSpacesReplicaStub> stubs = new ArrayList<TupleSpacesReplicaGrpc.TupleSpacesReplicaStub>();
    OrderedDelayer delayer;
    Integer clientId = -1;

    public ClientService(int numServers, ArrayList<String> host, ArrayList<String> port, Integer clientId){
        this.numServers = numServers;
        String target;
        this.clientId = clientId;
        delayer = new OrderedDelayer(numServers);
        channels = new ManagedChannel[numServers];
        for (int i = 0; i < numServers; i++) {
            target = host.get(i) + ":" + port.get(i);
            channels[i] = ManagedChannelBuilder.forTarget(target).usePlaintext().build();
            stubs.add(i, TupleSpacesReplicaGrpc.newStub(channels[i]));
        }
    }

    public void put(String tuple) throws InterruptedException, StatusRuntimeException {
        ResponseController<TupleSpacesReplicaXuLiskov.PutResponse> responseController = new ResponseController<TupleSpacesReplicaXuLiskov.PutResponse>();
        ClientObserver<TupleSpacesReplicaXuLiskov.PutResponse> clientObserver = new ClientObserver<TupleSpacesReplicaXuLiskov.PutResponse>(responseController);
        for (Integer i : delayer) {
            stubs.get(i).put(TupleSpacesReplicaXuLiskov.PutRequest.newBuilder().setNewTuple(tuple).build(), clientObserver);
        }
        waitResponse(responseController, numServers);
    }

    public String read(String tuple) throws InterruptedException {
        ResponseController<TupleSpacesReplicaXuLiskov.ReadResponse> responseController = new ResponseController<TupleSpacesReplicaXuLiskov.ReadResponse>();
        ClientObserver<TupleSpacesReplicaXuLiskov.ReadResponse> clientObserver = new ClientObserver<TupleSpacesReplicaXuLiskov.ReadResponse>(responseController);
        Thread callStubs = new Thread(() -> {
            for (Integer i : delayer) {
                stubs.get(i).read(TupleSpacesReplicaXuLiskov.ReadRequest.newBuilder().setSearchPattern(tuple).build(), clientObserver);
            }
        });

        callStubs.start();
        waitResponse(responseController, 1);

        for(TupleSpacesReplicaXuLiskov.ReadResponse message : responseController.getMessages()){
            return message.toString().replace("result: \"", "").replace("\"\n", "");
        }
        return null;
    }

    public String takePhase1(String tuple) throws InterruptedException {
        int noResponses = numServers;
        int numExecs = 0;
        Random random = new Random();
        List<List<String>> reservedTuples = new ArrayList<List<String>>();
        while(noResponses != 0){
            ResponseController<TupleSpacesReplicaXuLiskov.TakePhase1Response> responseController = new ResponseController<TupleSpacesReplicaXuLiskov.TakePhase1Response>();
            reservedTuples.clear();
            noResponses = numServers;
            if(numExecs == 0){
                //Only first execution of while loop uses set delay
                for (Integer i : delayer) {
                    stubs.get(i).takePhase1(TupleSpacesReplicaXuLiskov.TakePhase1Request.newBuilder().setSearchPattern(tuple).setClientId(clientId).build(), new ClientObserver<TupleSpacesReplicaXuLiskov.TakePhase1Response>(responseController));
                }
            }
            else{
                for(int i = 0; i < numServers; i++){
                    stubs.get(i).takePhase1(TupleSpacesReplicaXuLiskov.TakePhase1Request.newBuilder().setSearchPattern(tuple).setClientId(clientId).build(), new ClientObserver<TupleSpacesReplicaXuLiskov.TakePhase1Response>(responseController));
                }
            }
            //Waits for response from server
            waitResponse(responseController, numServers);
            // Divides the received tuple in the tuples received
            for(TupleSpacesReplicaXuLiskov.TakePhase1Response message : responseController.getMessages()){
                if(message == null || message.getReservedTuplesList().isEmpty()){
                    continue;
                }
                String[] tuples = message.toString().replace("reservedTuples: \"", "")
                        .replace("\"\n", "").replace("><", ">,,<")
                        .split(",,");

                List<String> reservedTuple = Arrays.asList(tuples);
                reservedTuples.add(reservedTuple);
                noResponses--;
            }
            // If minority accept the request, they are asked to release locks
            if(noResponses > numServers/2){
                takePhase1Release();
                Thread.sleep(random.nextInt(500, 1500));
                return null;
            }
            numExecs++;
        }

        // Checks the interception of the lists
        Set<String> commonElements = new HashSet<>(reservedTuples.get(0));
        commonElements.retainAll(reservedTuples.get(1));
        commonElements.retainAll(reservedTuples.get(2));

        // If there are no common elements, they are asked to release locks
        if(commonElements.isEmpty()){
            takePhase1Release();
            Thread.sleep(random.nextInt(1000)+500);
            return null;
        }
        else{ // Returns a random element from the common elements
            if(commonElements.size() == 1){
                return (String) commonElements.toArray()[0];
            }
            int randomIndex = random.nextInt((commonElements.size()-1));
            return (String) commonElements.toArray()[randomIndex];
        }
    }

    public void waitResponse(ResponseController<?> responseController, Integer numServers){
        try {
            responseController.waitUntilAllReceived(numServers);
        } catch (InterruptedException e) {
            System.out.println(e.getMessage());
        }

        for (Throwable throwable: responseController.getErrors()) {
            if (throwable.getClass() == io.grpc.StatusRuntimeException.class) {
                System.err.println("A Server has shut down, therefore client shutting down");
                exit(0);
            }
        }
    }

    public void takePhase1Release(){
        ResponseController<TupleSpacesReplicaXuLiskov.TakePhase1ReleaseResponse> responseControllerRelease = new ResponseController<TupleSpacesReplicaXuLiskov.TakePhase1ReleaseResponse>();
        ClientObserver<TupleSpacesReplicaXuLiskov.TakePhase1ReleaseResponse> clientObserverRelease = new ClientObserver<TupleSpacesReplicaXuLiskov.TakePhase1ReleaseResponse>(responseControllerRelease);
        for(int i = 0; i < numServers; i++){
            stubs.get(i).takePhase1Release(TupleSpacesReplicaXuLiskov.TakePhase1ReleaseRequest.newBuilder().setClientId(clientId).build(), clientObserverRelease);
        }
    }

    public String takePhase2(String tuple) {
        ResponseController<TupleSpacesReplicaXuLiskov.TakePhase2Response> responseController = new ResponseController<TupleSpacesReplicaXuLiskov.TakePhase2Response>();
        ClientObserver<TupleSpacesReplicaXuLiskov.TakePhase2Response> clientObserver = new ClientObserver<TupleSpacesReplicaXuLiskov.TakePhase2Response>(responseController);

        for (int i = 0; i < numServers; i++) {
            stubs.get(i).takePhase2(TupleSpacesReplicaXuLiskov.TakePhase2Request.newBuilder().setTuple(tuple).setClientId(clientId).build(), clientObserver);
        }
        waitResponse(responseController, numServers);
        takePhase1Release();
        return tuple;
    }

    public String[] getTupleSpacesState(Integer server){
        try{
            TupleSpacesReplicaGrpc.TupleSpacesReplicaBlockingStub blockingStub = TupleSpacesReplicaGrpc.newBlockingStub(channels[server]);
            TupleSpacesReplicaXuLiskov.getTupleSpacesStateResponse response = blockingStub.getTupleSpacesState(TupleSpacesReplicaXuLiskov.getTupleSpacesStateRequest.getDefaultInstance());
            return response.getTupleList().toArray(new String[0]);
        }
        catch (io.grpc.StatusRuntimeException e){
            System.err.println("A Server has shut down, therefore client shutting down");
            exit(0);
        }
        return null;
    }

    public void setDelay(int id, int delay) {
        delayer.setDelay(id, delay);
    }
}