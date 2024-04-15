package pt.ulisboa.tecnico.tuplespaces.client.grpc;

import io.grpc.stub.StreamObserver;

public class ClientObserver<R> implements StreamObserver<R> {
    private ResponseController<R> responseController;

    public ClientObserver(ResponseController<R> responseController){
        this.responseController = responseController;
    }

    public void setResponseController(ResponseController<R> responseController) {
        this.responseController = responseController;
    }

    public ResponseController<R> getResponseController() {
        return responseController;
    }

    @Override
    public void onNext(R r){
        responseController.addMessage(r);
    }

    @Override
    public void onError(Throwable throwable) {
        responseController.addError(throwable);
    }

    @Override
    public void onCompleted() {}
}
