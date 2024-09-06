#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <stdbool.h>
#include <wren.h>

#define NET_SOCKET_MAX_CONNECTIONS 50000

typedef struct NetSocket
{
    int fd;
    char name[30];
} NetSocket;

static void netSocketSelect(WrenVM *vm);
static void netSocketConnect(WrenVM *vm);
static void netSocketRead(WrenVM *vm);
static void netSocketWrite(WrenVM *vm);
static void netSocketStats(WrenVM *vm);
static void netSocketAccept(WrenVM *vm);
static void netSocketServe(WrenVM *vm);
static void netSocketListen(WrenVM *vm);
static void netSocketBind(WrenVM *vm);
static void netSocketName(WrenVM *vm);
static void netSocketInit(WrenVM *vm);
static bool netSetNonBlocking(int sock);
static void netSocketClose(WrenVM *vm);
static void _netSocketClose(int sock);
static NetSocket * getNetSocketByFd(int sock);

NetSocket sockets[NET_SOCKET_MAX_CONNECTIONS] = {0};
unsigned long sockets_connected = 0;
int net_socket_max_fd = 0;
int net_socket_server_fd = 0;
bool net_socket_serving = 0;
unsigned long sockets_total = 0;
unsigned long sockets_disconnected = 0;
unsigned long sockets_concurrent_record = 0;
unsigned long sockets_errors = 0;

static bool netSetNonBlocking(int sock)
{
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0)
    {
        perror("fcntl");
        return false;
    }

    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        perror("fcntl");
        return false;
    }

    return true;
}

static void netSocketInit(WrenVM *vm)
{
    memset(sockets, 0, sizeof(sockets));
    int opt = 1;
    if ((net_socket_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(net_socket_server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("Setsockopt failed");
        close(net_socket_server_fd);
        wrenSetSlotBool(vm, 0, false);
    }
    else
    {
        netSetNonBlocking(net_socket_server_fd);
        wrenSetSlotBool(vm, 0, true);
    }
}

static void netSocketName(WrenVM *vm)
{
    int fd = (int)wrenGetSlotDouble(vm, 1);
    NetSocket * netSocket = getNetSocketByFd(fd);
    if(netSocket)
   {
            wrenSetSlotString(vm, 0, netSocket->name);
   }else{
        // If socket disconnected or is no client from server
        wrenSetSlotNull(vm,0);
   }
}

static void netSocketBind(WrenVM *vm)
{
    struct sockaddr_in address;
    int port = (int)wrenGetSlotDouble(vm, 1);
    
    address.sin_family = AF_INET;         // IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Bind to any available address
    address.sin_port = htons(port);       // Convert port to network byte order

    if (bind(net_socket_server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        close(net_socket_server_fd);
        wrenSetSlotBool(vm, 0, false);
    }
    else
    {

        wrenSetSlotBool(vm, 0, true);
    }
}

static void netSocketListen(WrenVM *vm)
{
    int backlog = (int)wrenGetSlotDouble(vm, 1);

    if (listen(net_socket_server_fd, backlog) < 0)
    { // '3' is the backlog size
        perror("Listen failed");
        close(net_socket_server_fd);
        wrenSetSlotBool(vm, 0, false);
    }
    else
    {

        wrenSetSlotBool(vm, 0, true);
    }
}

static void netSocketServe(WrenVM *vm)
{
    if (net_socket_serving == false)
    {
        // It expects these two parameters in related function that get called
        //int port = (int)wrenGetSlotDouble(vm, 1);
        int backlog = (int)wrenGetSlotDouble(vm, 2);
        netSocketInit(vm);
        if (!wrenGetSlotBool(vm, 0))
        {
            return;
        }
        netSocketBind(vm);
        if (!wrenGetSlotBool(vm, 0))
        {
            return;
        }
        wrenSetSlotDouble(vm, 1, (double)backlog);
        netSocketListen(vm);
        if (!wrenGetSlotBool(vm, 0))
        {
            return;
        }
        net_socket_serving = true;
    }
    netSocketSelect(vm);
}

static void netSocketAccept(WrenVM *vm)
{
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int new_socket = 0;
    if ((new_socket = accept(net_socket_server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        close(net_socket_server_fd);
        wrenSetSlotDouble(vm, 0, 0);
    }
    else
    {

        wrenSetSlotDouble(vm, 0, (double)new_socket);
    }
}

static void netSocketStats(WrenVM *vm)
{

    wrenSetSlotNewList(vm, 0);

    wrenSetSlotString(vm, 1, "sockets_total");
    wrenInsertInList(vm, 0, -1, 1);

    wrenSetSlotDouble(vm, 1, (double)sockets_total);
    wrenInsertInList(vm, 0, -1, 1);

    wrenSetSlotString(vm, 1, "sockets_concurrent_record");
    wrenInsertInList(vm, 0, -1, 1);

    wrenSetSlotDouble(vm, 1, (double)sockets_concurrent_record);
    wrenInsertInList(vm, 0, -1, 1);

    wrenSetSlotString(vm, 1, "sockets_connected");
    wrenInsertInList(vm, 0, -1, 1);

    wrenSetSlotDouble(vm, 1, (double)sockets_connected);
    wrenInsertInList(vm, 0, -1, 1);

    wrenSetSlotString(vm, 1, "sockets_disconnected");
    wrenInsertInList(vm, 0, -1, 1);

    wrenSetSlotDouble(vm, 1, (double)sockets_disconnected);
    wrenInsertInList(vm, 0, -1, 1);
}
static void netSocketWrite(WrenVM *vm)
{

    int sock = (int)wrenGetSlotDouble(vm, 1);
    const char *message = wrenGetSlotString(vm, 2);
    ssize_t sent_total = 0;
    ssize_t sent = 0;
    ssize_t to_send = strlen(message) + 1;
    while ((sent = send(sock, message, to_send, 0)))
    {
        if (sent == -1)
        {
            sockets_errors++;
            _netSocketClose(sock);
            break;
        }
        if(sent == 0){
            printf("EDGE CASE?\n");
            exit(1);
            sockets_errors++;
            _netSocketClose(sock);
            break;
        }
        sent_total += sent;
        if (sent_total == to_send)
            break;
    }
    wrenSetSlotDouble(vm, 0, (double)sent_total);
}

static void netSocketRead(WrenVM *vm)
{

    int sock = (int)wrenGetSlotDouble(vm, 1);
    int buffSize = (int)wrenGetSlotDouble(vm, 2);
    static char buffer[1024 * 1024];
    int received = recv(sock, buffer, buffSize, 0);
    if (received <= 0)
    {
        _netSocketClose(sock);
        if(net_socket_serving && received < 0){
            sockets_errors++;
        }
    }
    buffer[received] = 0;
    wrenSetSlotString(vm, 0, buffer);
}

static void netSocketConnect(WrenVM *vm)
{
    const char *host = wrenGetSlotString(vm, 1);
    int port = (int)wrenGetSlotDouble(vm, 2);
    char port_str[10] = {0};
    int status;
    sprintf(port_str, "%d", port);
    int socket_fd = 0;
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *p;
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        wrenSetSlotDouble(vm, 0, 0);
        return;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       
    hints.ai_socktype = SOCK_STREAM; 

    if ((status = getaddrinfo(host, port_str, &hints, &res)) != 0)
    {
        wrenSetSlotDouble(vm,0,0);
        return;;
    }

    for (p = res; p != NULL; p = p->ai_next)
    {
        if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            continue;
        }

        
        if (connect(socket_fd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(socket_fd);
            continue;
        }

        break; 
    }

    if (p == NULL)
    {
        freeaddrinfo(res);
        wrenSetSlotDouble(vm, 0, 0);
        return;
    }

    freeaddrinfo(res);

    wrenSetSlotDouble(vm, 0, (double)socket_fd);
}

static void netSocketSelect(WrenVM *vm)
{
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(net_socket_server_fd, &read_fds);
    if (net_socket_server_fd > net_socket_max_fd)
    {
        net_socket_max_fd = net_socket_server_fd;
    }
    int socket_fd;
    for (int i = 0; i < net_socket_max_fd; i++)
    {
        socket_fd = sockets[i].fd;
        if (socket_fd > 0)
        {
            FD_SET(socket_fd, &read_fds);
        }
        if (socket_fd > net_socket_max_fd)
        {
            net_socket_max_fd = socket_fd;
        }
    }
    int new_socket = 0;
    struct sockaddr_in address;
    int addrlen = sizeof(struct sockaddr_in);
    int activity = select(net_socket_max_fd + 1, &read_fds, NULL, NULL, NULL);
    if ((activity < 0) && (errno != EINTR))
    {
        perror("Select error");
    }
    if (FD_ISSET(net_socket_server_fd, &read_fds))
    {
        if ((new_socket = accept(net_socket_server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
        netSetNonBlocking(new_socket);
        char name[30] = {0};
        sprintf(name, "fd:%d:ip:%s:port:%d\n", new_socket,
                inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        for (int i = 0; i < NET_SOCKET_MAX_CONNECTIONS; i++)
        {
            if (sockets[i].fd == 0)
            {
                sockets[i].fd = new_socket;
                strcpy(sockets[i].name, name);

                sockets_connected++;
                sockets_total++;
                sockets_concurrent_record = sockets_connected > sockets_concurrent_record ? sockets_connected : sockets_concurrent_record;
                if (new_socket > net_socket_max_fd)
                {
                    net_socket_max_fd = new_socket;
                }
                break;
            }
        }
    }
    wrenSetSlotNewList(vm, 0);
    for (int i = 0; i < net_socket_max_fd; i++)
    {
        socket_fd = sockets[i].fd;

        if (FD_ISSET(socket_fd, &read_fds))
        {
            wrenSetSlotDouble(vm, 1, (double)socket_fd);
            wrenInsertInList(vm, 0, -1, 1);
        }
    }
}

static NetSocket * getNetSocketByFd(int sock){
    if(!net_socket_serving){
        // client sockets do not have NetSocket object
        return NULL;
    }
   for (int i = 0; i < net_socket_max_fd; i++)
    {
        if (sockets[i].fd == sock)
        {
            return &sockets[i];
        }
    }
    return NULL; 
}

static void _netSocketClose(int sock)
{
    NetSocket * netSocket = getNetSocketByFd(sock);
    if(netSocket){
        sockets_connected--;
        sockets_disconnected++;
        netSocket->fd = 0;
    }
    close(sock); 
}

static void netSocketClose(WrenVM *vm)
{
    int sock = (int)wrenGetSlotDouble(vm, 1);
    _netSocketClose(sock);
    wrenSetSlotDouble(vm, 0, 1);
}

typedef struct NetModuleFunction
{
    char moduleName[1024];
    void (*function)(WrenVM *);
    char className[1024];
    char signature_vm[1034];
    char signature_script[1034];
} NetModuleFunction;

static NetModuleFunction NetModuleFunctions[] = {
    {"net", netSocketInit, "Socket", "init()", "init()"},
    {"net", netSocketBind, "Socket", "bind(_)", "bind(port)"},
    {"net", netSocketListen, "Socket", "listen(_)", "listen(backlog)"},
    {"net", netSocketServe, "Socket", "serve(_,_)", "serve(port,backlog)"},
    {"net", netSocketServe, "Socket", "serve()", "serve()"},
    {"net", netSocketAccept, "Socket", "accept()", "accept()"},
    {"net", netSocketRead, "Socket", "read(_,_)", "read(sock,buffsize)"},
    {"net", netSocketWrite, "Socket", "write(_,_)", "write(sock,data)"},
    {"net", netSocketClose, "Socket", "close(_)", "close(sock)"},
    {"net", netSocketSelect, "Socket", "select()", "select()"},
    {"net", netSocketName, "Socket", "name(_)", "name(sock)"},
    {"net", netSocketStats, "Socket", "stats()", "stats()"},
    {"net", netSocketConnect, "Socket", "connect(_,_)", "connect(host,port)"}};
static unsigned int netModuleFunctionCount = sizeof(NetModuleFunctions) / sizeof(NetModuleFunction);

static const char *createNetModuleString()
{
    static char definition[8000];
    definition[0] = 0;
    strcat(definition, "class Socket {\n");
    strcat(definition, "construct new() {}\n");
    for (unsigned int i = 0; i < netModuleFunctionCount; i++)
    {   
        strcat(definition, "foreign ");
        strcat(definition, NetModuleFunctions[i].signature_script);
        strcat(definition, "\n");
    }
    strcat(definition, "}\n");

    strcat(definition, "var socket = Socket.new()\n");
    strcat(definition, "class Client {\n"
                       "construct patch(sock) {\n"
                       "socket = Socket.new()\n"
                       "_fd = sock\n"
                       "_connected = true\n"
                       "_bytesWritten = 0\n"
                       "_bytesRead = 0\n"
                       "_bufferSize = 4096\n"
                       "}\n"
                       "fd {\n"
                       "    return _fd\n"
                       "}\n"
                       "fd=(val){\n"
                       "    _fd = val\n"
                       "}\n"
                       "connected {\n"
                       "    return _connected\n"
                       "}\n"
                       "connected=(val){\n"
                       "    _connected = val\n"
                       "}\n"
                       "bytesWritten {\n"
                       "    return _bytesWritten\n"
                       " }\n"
                       "bytesRead {\n"
                       "    return _bytesRead\n"
                       "}\n"
                       "bytesWritten=(val){\n"
                       "    _bytesWritten = val\n"
                       "}\n"
                       "bytesRead=(val){\n"
                       "    _bytesRead = val\n"
                       "}\n"
                       "bufferSize {\n"
                       "    return _bufferSize\n"
                       "}\n"
                       "bufferSize=(val){\n"
                       "    _bufferSize = val\n"
                       "}\n"
                       "construct connect(host, port){\n"
                        "socket = Socket.new()\n"
                       "    _bytesWritten = 0\n"
                       "    _bytesSent = 0\n"
                       "    _bufferSize = 4096\n"
                       "    var res = socket.connect(host,port)\n"
                       "    this.connected = res > 0\n"
                       "    if(this.connected){\n"
                       "        this.fd = res\n"
                       "    }\n"
                       "}\n"
                       "socket {\n"
                        "return _socket\n"
                       "}\n"
                       "socket=(val){\n"
                       "     _socket = val\n"
                       "}\n"
                       "communicate(toWrite){\n"
                       "    var v = this.write(toWrite)\n"
                       "    if(v == null){\n"
                       "        return null\n"
                       "    }\n"
                       "    return this.read(this.bufferSize)\n"
                       "}\n"
                       "write(data){\n"
                       "   var sent = socket.write(this.fd,data)\n"
                       "    if(sent == -1){\n"
                       "        this.close()\n"
                       "        return null\n"
                       "    }else{\n"
                       "        this.bytesWritten = this.bytesWritten + sent\n"
                       "    }\n"
                       "    return sent\n"
                       "}\n"
                       "\n"
                       "read(){\n"
                       "    var data = socket.read(this.fd,this.bufferSize)\n"
                       "    if(data == null || data.count == 0){\n"
                       "        this.close()\n"
                       "        return null\n"
                       "    }else {\n"
                       "        this.bytesRead = this.bytesRead + data.count\n"
                       "     }\n"
                       "    return data\n"
                       "}\n"
                       "close() {\n"
                       "    this.connected = false\n"
                       "    return socket.close(this.fd)\n"
                       "}\n"
                       "}\n");
    return definition;
}

WrenLoadModuleResult loadNetModule(WrenVM *vm, const char *name)
{
    WrenLoadModuleResult result = {0};
    result.source = createNetModuleString();
    return result;
}

WrenForeignMethodFn bindNetModuleClass(WrenVM *vm,
                                       const char *module, const char *className, bool isStatic,
                                       const char *signature)
{
    for (unsigned int i = 0; i < netModuleFunctionCount; i++)
    {
        NetModuleFunction wf = NetModuleFunctions[i];
        if(strcmp(module, wf.moduleName))
            continue;
        if(strcmp(className, wf.className))
            continue;
        if (strcmp(signature, wf.signature_vm))
            continue;    
        return wf.function;
    }
    return NULL;
}
