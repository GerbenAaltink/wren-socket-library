# Wren sockets

## Summary
This project makes the C socket API avaiable to wren with minimal overhead. It supports sync and async. For asynchronous support it uses the blazing fast select system call. It is to be used in a functional or OOP way. The library is very stable, tested with simulated low quality clients closing unexpectetly and communicating slow while being benchmarked for concurrency. It did not affect performance.

## Examples of using this library

### Server

#### Asynchronious using functions
This is an example of an asynchronious http server using `serve.serve()` in combination with the function api. function API is preferred for sake performance. The OOP way takes a performance hit. The example below can be faster by using `server.serve()` once above the loop and `serve.select()` in the loop condition. It spares two parameters to be communicated each iteration and skips the checks `server.serve()` normally does.

```wren
import "net" for socket

// Since wren is slow in concat of strings, declare response outside the loop.
var responseStr = [
    "HTTP/1.1 200 OK\r\n",
    "Content-Type: text/html; charset=UTF-8\r\n",
    "Content-Length: 137\r\n",
    "Connection: close\r\n\r\n",
    "Hi!"
].join("")

while(true){
    // socket.serve returns a list of socket file descriptors
    for(c in socket.serve(8080,10)){
        // socket.name() returns a string with format: <fd number>:ip:<remote ip>:port:<remote port>
        var name = socket.name(c)
        System.print(name + " connected.") 
        var data = socket.read(c,1024)
        System.print(name + " sent: " + data + ".")
        socket.write(c, responseStr)
        System.print(name + " has received our response.")
        socket.close(c)
        System.print(name + " closed.")
    }
}
```
#### Asynchronous using OOP
As in previous examples, the `socket.serve()` returns an list of socket file descriptors. To convert that fd to a client object is done with `Client.patch()`.
```wren
import "net" for socket
import "net" for Client

for(fd in socket.serve(8080,1000)){
    var c = Client.patch(fd)
    var data = c.read()
    c.write(
        [
            "HTTP/1.1 200 OK\r\n",
            "Content-Type: text/html; charset=UTF-8\r\n",
            "Content-Length: 3\r\n",
            "Connection: close\r\n\r\n",
            "Hi!"
        ].join("")
    )
    c.close()
}

```
#### Synchronous
Handles one client at a time. When using the synchronous method, the socket that is connected first will be handled first despite how long it takes to receive data. The asynchronous method will handle the first sockets being able to read so it doesn't block for others without reason.
```wren
from "net" import socket

socket.init()
socket.bind(8080) // parameter is port
socket.listen(10) // parameter is backlog
while(c = socket.accept()){
    // Your code here
}
```
### Manually, without using serve
Serve handles initialization, binding and listening for you. If you prefer to do it yourself for debugging purposes or for implementing errors correctly, see example below. 
```wren
from "net" import socket

if(!socket.init()){
    System.out("Socket init failed")
    // error handling
}
if(!socket.bind(8080)){ // parameter is port
    System.out("Socket bind failed") 
    // error handling 
}
if(!socket.listen(10)){ // parameter is backlog
    System.out("Socket listen failed")
    // error handling
}
while(true){
    // we're using select instead of serve since we're doing it all by ourselves
    for(c in socket.select()){
        // Your code here
    }
}
```
### Client

#### Using functions
This is the most performant way of using a socket as client.    
```wren
import "net" for socket

var c = socket.connect("127.0.0.1",8080) 
socket.write(c,"GET / HTTP/1.1r\n\r\n")
var data = socket.read(c,1024)
System.print(data)
socket.close(c)
```
#### Using OOP
This example uses the OOP API. It has overhead in comparison to the core function API but is easier.
```wren
import "net" for Client

var c = Client.connect("127.0.0.1",8080)) 
c.write("GET / HTTP/1.1r\n\r\n")
var data = c.read()
System.print(data)
c.close()
```
