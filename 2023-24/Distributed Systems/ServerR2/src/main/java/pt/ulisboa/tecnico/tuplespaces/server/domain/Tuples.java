package pt.ulisboa.tecnico.tuplespaces.server.domain;

public class Tuples {
    private boolean lock;
    private String tuple;
    private Integer clientId;

    Tuples(){
    }

    Tuples (String tuple) {
        this.lock = false;
        this.tuple = tuple;
        this.clientId = -1;
    }

    public boolean isLocked() {
        return lock;
    }

    public boolean lock(Integer clientId) {
        if(this.lock && this.clientId != clientId) {
            return false;
        }
        this.lock = true;
        this.clientId = clientId;
        return true;
    }

    public void unlock() {
        this.lock = false; 
    }

    public String getTuple() {
        return tuple;
    }

    public Integer getClientId() {
        return clientId;
    }

    public void setClientId(Integer clientId){
        this.clientId = clientId;
    }
}
