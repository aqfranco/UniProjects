package pt.ulisboa.tecnico.tuplespaces.server;

import io.grpc.*;
import pt.ulisboa.tecnico.tuplespaces.nameserver.contract.NameServerGrpc;
import pt.ulisboa.tecnico.tuplespaces.nameserver.contract.NameServerOuterClass;

import java.io.IOException;

public class ServerMain {
    private static final boolean DEBUG_FLAG = (System.getProperty("debug") != null);

    private static void debug(String debugMessage) {
	/* Set flag to true to print debug messages.
	The flag can be set using the -Ddebug command line option. */
        if (DEBUG_FLAG)
            System.err.printf(debugMessage);
    }

    public static void main(String[] args) throws InterruptedException, IOException {
        debug(String.format("Received %d arguments%n", args.length));
        for (int i = 0; i < args.length; i++) {
            debug(String.format("arg[%d] = %s%n", i, args[i]));
        }

        if (args.length < 4) {
            System.err.println("Argument(s) missing!");
            return;
        }

        final int port = Integer.parseInt(args[1]);
        final BindableService impl = new ServerImpl();

        //Create a new server
        Server server = ServerBuilder.forPort(port).addService(impl).build();

        final NameServerGrpc.NameServerBlockingStub stub;
        final ManagedChannel channel;
        String target;

        //Create stub to connect to nameServer
        target = "localhost:5001";
        channel = ManagedChannelBuilder.forTarget(target).usePlaintext().build();
        stub = NameServerGrpc.newBlockingStub(channel);

        System.out.print(stub.register(NameServerOuterClass.RegisterRequest.newBuilder().setName(args[3]).setAddress(args[0] + ":" + args[1]).setQualifier(args[2]).build()).getMessage());

        //Starts server
        server.start();

        /*Create shutdownHook to delete server information from
        nameServer when server shuts down due to SIGINT*/
        Runtime.getRuntime().addShutdownHook(new Thread()
        {
            @Override
            public void run() {
                NameServerOuterClass.DeleteResponse response = stub.delete(NameServerOuterClass.DeleteRequest.newBuilder().setName(args[3]).setAddress(args[0] + ":" + args[1]).build());
                System.out.print(response.getMessage());
                debug("Server shutting down");
                server.shutdown();
            }
        });
        while(true){
            server.awaitTermination();
        }
    }
}

