package pt.ulisboa.tecnico.tuplespaces.client;

import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import pt.ulisboa.tecnico.tuplespaces.client.grpc.ClientService;
import pt.ulisboa.tecnico.tuplespaces.nameserver.contract.NameServerGrpc;
import pt.ulisboa.tecnico.tuplespaces.nameserver.contract.NameServerOuterClass;

import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.List;

public class ClientMain {

    private static final boolean DEBUG_FLAG = (System.getProperty("debug") != null);

    private static void debug(String debugMessage) {
        /** Set flag to true to print debug messages.
         * The flag can be set using the -Ddebug command line option. */
        if (DEBUG_FLAG)
            System.err.printf(debugMessage);
    }

    public static void main(String[] args) throws InterruptedException {

        debug(ClientMain.class.getSimpleName() + " ");

        // receive and print arguments
        debug(String.format("Received %d arguments%n", args.length));
        for (int i = 0; i < args.length; i++) {
            debug(String.format("arg[%d] = %s%n", i, args[i]));
        }

        // check arguments
        if (args.length != 4) {
            System.err.println("Argument(s) missing!");
            System.err.println("Usage: mvn exec:java -Dexec.args=<host> <port> <service> <clientId>");
            return;
        }

        // get the host and the port
        final String host = args[0];
        final String port = args[1];
        final Integer clientId = Integer.parseInt(args[3]);

        final NameServerGrpc.NameServerBlockingStub stub;
        final ManagedChannel channel;
        String target;
        Boolean reconnectCount = true;

        //Create stub to connect to nameServer
        target = "localhost:5001";
        channel = ManagedChannelBuilder.forTarget(target).usePlaintext().build();
        stub = NameServerGrpc.newBlockingStub(channel);
        Thread thread = new Thread();
        Integer numServer = 0;

        while(reconnectCount) {
        //Get server list from nameServer
            try {
                NameServerOuterClass.LookupResponse response = stub.lookup(NameServerOuterClass.LookupRequest.newBuilder().setName(args[2]).setQualifier("").build());
                numServer = response.getServerCount();
                //debug(("Number of servers: " + numServer + '\n'));
                if(numServer < 3){
                    reconnectCount = true;
                    if((3-numServer) == 1){
                        System.out.println("Waiting for 1 server to start");
                    }
                    else{
                        System.out.println("Waiting for " + (3-numServer) + " servers to start");
                    }
                    thread.sleep(5000);
                }
                else if(numServer > 3){
                    System.out.println("Too many servers started");
                    channel.shutdownNow();
                    return;
                }
                else {
                    reconnectCount = false;
                    String[] addresses = response.getServerList().stream().map(NameServerOuterClass.ServerEntry::toString).toArray(String[]::new);
                    ArrayList<String> hosts = new ArrayList<>();
                    ArrayList<String> ports = new ArrayList<>();

                    for (String address: addresses) {
                        String[] fields = address.split(":");
                        hosts.add(fields[1].replace("\"", "").replace(" ", ""));
                        ports.add(fields[2].replace("\"", "").replace("\n", ""));
                    }

                    //Connect to server
                    CommandProcessor parser = new CommandProcessor(new ClientService(hosts.size(), hosts, ports, clientId));
                    parser.parseInput();
                }
            }
            catch (io.grpc.StatusRuntimeException e) {
                System.err.println("NameServer is not Running");
                channel.shutdownNow();
                return;
            }
        }
    }
}
