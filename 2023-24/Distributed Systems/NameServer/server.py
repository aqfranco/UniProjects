import sys
sys.path.insert(1, '../Contract/target/generated-sources/protobuf/python')
import grpc
from concurrent import futures
import NameServer_pb2 as pb2
import NameServer_pb2_grpc as pb2_grpc
import os

# define the port
PORT = 5001

class ServerEntry:
    def __init__(self, address, qualifier):
        self.address = address
        self.qualifier = qualifier


class ServiceEntry:
    serverList = list()

    def __init__(self, name):
        self.name = name

    def addServer(self, s):
        for _server in self.serverList:
            if _server.address == s.address and _server.qualifier == s.qualifier:
                return False
        self.serverList.append(s)
        return True

class NamingServer:
    def __init__(self):
        pass

    serverMap = {}

    def checkName(self, name, qualifier, address):
        entry = self.serverMap.get(name)
        if entry is not None:
            added = entry.addServer(ServerEntry(address, qualifier))
            if added is True:
                self.serverMap[name] = entry
        else:
            service_ = ServiceEntry(name)
            service_.addServer(ServerEntry(address, qualifier))
            self.serverMap[name] = service_

class NamingServerServiceImpl(pb2_grpc.NameServerServicer):

    def __init__(self):
        self.namingServer = NamingServer()

    def register(self, request, context):
        NamingServer.checkName(self.namingServer, request.name, request.qualifier, request.address)
        debug("Server registered")
        return pb2.RegisterResponse()

    def lookup(self, request, context):
        debug("Looking for servers with %s service" % request.name)
        servers = []
        for _server in self.namingServer.serverMap.get(request.name).serverList:
            if request.qualifier == "" or _server.qualifier == request.qualifier:
                servers.append(pb2.ServerEntry(address = _server.address))
        return pb2.LookupResponse(server = servers)

    def delete(self, request, context):
        list = self.namingServer.serverMap.get(request.name).serverList
        if list is not None:
            for _server in list:
                if _server.address == request.address:
                    self.namingServer.serverMap.get(request.name).serverList.remove(_server)
                    debug("Server removed, %s" % self.namingServer.serverMap)
                    return pb2.DeleteResponse()
        debug("Not possible to remove the server")
        return pb2.DeleteResponse(message = "Not possible to remove the server")

if __name__ == '__main__':
    if "-debug" in sys.argv:
        os.environ["debug"] = "1"

    DEBUG_FLAG = os.getenv("debug") is not None
    def debug(debug_message):
        """Set flag to True to print debug messages.
        The flag can be set using the -Ddebug command line option."""
        if DEBUG_FLAG:
            print(debug_message, file=sys.stderr)

    try:
        # print received arguments
        debug("Received arguments:")
        for i in range(1, len(sys.argv)):
            debug("  " + sys.argv[i])
        # create server
        server = grpc.server(futures.ThreadPoolExecutor(max_workers=1))
        pb2_grpc.add_NameServerServicer_to_server(NamingServerServiceImpl(), server)
        # listen on port
        server.add_insecure_port('[::]:' + str(PORT))
        # start server
        server.start()
        debug("Server listening on port " + str(PORT))
        # print termination message
        print("Press CTRL+C to terminate")

        # wait for server to finish
        server.wait_for_termination()
    except KeyboardInterrupt:
        print(" HelloServer stopped")
        exit(0)
