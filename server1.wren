import "net" for socket 

var requestStr = ["HTTP/1.1 200 OK\r\n",
    "Content-Type: text/html; charset=UTF-8\r\n",
    "Content-Length: 137\r\n",
    "Connection: close\r\n",
    "\r\n",
    "<!DOCTYPE html>",
    "<html lang=\"en\">",
    "<head><meta charset=\"UTF-8\"><title>Success</title></head>",
    "<body><h1>Hello, World!</h1><p>This is a valid HTML 200 response.</p></body>",
    "</html>"
].join("")


while(true){
    for(s in socket.serve(8080,1000)){
        var c = s 
        System.print(socket.name(c) + ":connected") 
        var data = socket.read(c,1024)
        //System.print(c.toString + ":received")
        socket.write(c, requestStr)
        //System.print(c.toString + ":sent")
        //System.print(socket.name(c).toString + ":closing")
        socket.close(c)
        
    }
    for(stat in socket.stats()){
    	System.print(stat)  
    }
}


    
