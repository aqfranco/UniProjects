package pt.ulisboa.tecnico.tuplespaces.client.grpc;

import java.util.ArrayList;
import java.util.List;

public class ResponseController<R> {
    private List<R> messages = new ArrayList<>();
    private List<Throwable> errors = new ArrayList<>();

    public ResponseController(){}

    public synchronized void addMessage(R message){
        messages.add(message);
        notifyAll();
    }

    public synchronized void addError(Throwable throwable){
        errors.add(throwable);
        notifyAll();
    }

    public List<R> getMessages(){
        return messages;
    }

    public List<Throwable> getErrors() {
        return errors;
    }

    public synchronized void waitUntilAllReceived(Integer numServers) throws InterruptedException {
        while((messages.size() + errors.size()) < numServers){
            this.wait();
        }
    }
}
