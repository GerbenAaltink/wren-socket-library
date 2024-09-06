import "net" for socket
import "net" for Client 



class Semaphore {
	
	construct new(limit){
		_counter = 0
                _max = limit
                _running = []
                _queued = [] 
                _concurrency = 600  
	}
	running {
		return _running  
	}
	queued {
		return _queued 
	}
	queued=(val){
		_queued = val  
	}
	running=(val){
		_running = val  
	}
	max {
		return _max 
	}
	max=(val){ 
		_max = val 
	}
	counter {
		return _counter 
	}
	counter=(val){
		_counter = val 
	}
	acquire(){
		if(counter < max){
			counter = counter + 1  
			return true  
		}else{
			return false  
		}
	}
	release(){
		counter = counter - 1  
	}
	step() {
		var task = _running.count > 0 ? _running.removeAt(0) : null
		if(!task){
			task = _queued.count > 0 ? _queued.removeAt(0) : null 
			
			  
		}
		if(!task){
			return false
		}
		var result = task.call()
		if(result){
			System.print(result) 
		}
		if(!task.isDone){
			_running.add(task)
			return true  
		}else{
			task = _queued.count > 0 ?  _queued.removeAt(0) : null 
			if(task){
				_running.add(task)
				return true 
			}else{
				return false 
			
			}
		}
	}
	run() {
		while(step()){
			//step()
		}
	}
	add(task){
		if(_running.count >= _concurrency){
			_queued.add(task) 
		}else{
			_running.add(task)
		}
		return step()
	}

}





var sema = Semaphore.new(100)


var fibers = []
var start = System.clock
for(i in 0..100000){
    sema.add(Fiber.new {

		Fiber.yield("connecting")
	    var c = socket.connect("127.0.0.1",8080) 
        Fiber.yield("writing")
		socket.write(c,"GET / HTTP/1.1r\n\r\n")
    	Fiber.yield("reading")
		var data = socket.read(c,1024)
		Fiber.yield("closing")
		socket.close(c)
        Fiber.yield("closed")
	

		/*	
      	Fiber.yield("connecting")
	    var c = Client.patch(socket.connect("127.0.0.1",8080)) 
        Fiber.yield("writing")
		c.write("GET / HTTP/1.1r\n\r\n")
       	Fiber.yield("reading")
		var data = c.read()
		Fiber.yield("closing")
		c.close()
        Fiber.yield("closed")
		*/
		
		
    })
}
sema.run()
var end = System.clock
var duration = end - start 
System.print("Time taken for list operations: %(duration) seconds")