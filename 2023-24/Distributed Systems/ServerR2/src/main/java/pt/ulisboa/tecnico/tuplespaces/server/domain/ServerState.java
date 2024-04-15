package pt.ulisboa.tecnico.tuplespaces.server.domain;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

public class ServerState {

  private List<Tuples> tuples;
  private ReadWriteLock lock = new ReentrantReadWriteLock();

  public ServerState() {
    this.tuples = new ArrayList<Tuples>();
  }

  public void put(String tuple) {
      lock.writeLock().lock();
      tuples.add(new Tuples(tuple));
      lock.writeLock().unlock();
  }

  private String getMatchingTuple(String pattern) {
    String tuple = null;
    for (Integer i=0; i < tuples.size(); i++) {
      if (tuples.get(i).getTuple().matches(pattern)) {
        tuple = tuples.get(i).getTuple();
        return tuple;
        }
      }
    return tuple;
  }

  private List<String> getMatchingTuples(String pattern, Integer clientId) {
    List<String> matchingTuples = new ArrayList<String>();
    Integer locked = 0;
    for (Integer i=0; i < tuples.size(); i++) {
      if (tuples.get(i).getTuple().matches(pattern)) {
        if(tuples.get(i).lock(clientId)) {
          matchingTuples.add(tuples.get(i).getTuple());
        }
        else{
          locked++;
        }
      }
    }
    if(!matchingTuples.isEmpty()){
      return matchingTuples;
    }
    if(locked > 0){
      return Collections.emptyList();
    }
    return null;
  }

  private Tuples getTuple(String pattern, Integer clientId) {
    for (Integer i=0; i < tuples.size(); i++) {
      if (tuples.get(i).getTuple().matches(pattern)) {
        if(tuples.get(i).getClientId() == clientId){
          return tuples.get(i);
        }
        else{
          Tuples tuple = new Tuples();
          tuple.setClientId(null);
          return tuple;
        }
      }
    }
    return null;
  }

  public String read(String pattern) {
    lock.readLock().lock();
    try {
      return getMatchingTuple(pattern);
    } finally {
      lock.readLock().unlock();
    }
  }

  public List<String> takePhase1(String pattern, Integer clientId) {
    lock.writeLock().lock();
    try {
      return getMatchingTuples(pattern, clientId);
    } finally {
      lock.writeLock().unlock();;
    }
  }

  public void takePhase1Release(Integer clientId) {
    // Ver match com o client Id e dar unlock a esses
    lock.writeLock().lock();
    try {
      for (Integer i=0; i < tuples.size(); i++) {
        if (tuples.get(i).isLocked() && tuples.get(i).getClientId() == clientId) {
          tuples.get(i).unlock();
        }
      }
    } finally {
      lock.writeLock().unlock();
    }
  }

  public String takePhase2(String pattern, Integer clientId) {
    lock.writeLock().lock();
    try {
      Tuples tuple = getTuple(pattern, clientId);
      if (tuple != null) {
        if(tuple.getClientId() == null){
          return "";
        }
        tuples.remove(tuple);
        return tuple.getTuple();
      }
    } finally {
      lock.writeLock().unlock();
      // Release dos restantes tuplos
      takePhase1Release(clientId);
    }
    return null;
  }

  public List<String> getTupleSpacesState() {
    List<String> tuples = new ArrayList<String>();
    lock.readLock().lock();
    try{
      for (Integer i=0; i < this.tuples.size(); i++) {
        tuples.add(this.tuples.get(i).getTuple());
      }
      return tuples;
    } finally {
      lock.readLock().unlock();
    }
  }
}
