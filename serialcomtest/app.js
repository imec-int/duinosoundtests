var SerialPort = require("serialport").SerialPort;
var sleep = require('sleep');
var async = require('async');
var serialPort = new SerialPort("/dev/cu.usbserial-DN008Z9K", {
  baudrate: 57600
});
var send = true;
var dataBuf;
// var buffy;
// var complete = false;
var length = 0;
var incomingByteCounter = 0;
var incomingInt = 0;
if(!process.argv[2] || !process.argv[3]) {
	console.log('usage: node app.js /path/to/filetowrite itemid');
	process.exit(1);
}

// serialPort.open(function(error) {
// 		serialPort.on('data', function(data) {
// 			for(var i = 0; i < data.length; i++) {
// 				console.log(data[i]);
// 			}
// 	});

// 	setTimeout(function(){
// 		serialPort.write(new Buffer([2]));
// 	}, 2000);

// });

// auto reset on serial connection: rfduino resets on open, therefore we need a timeout
// http://playground.arduino.cc/Main/DisablingAutoResetOnSerialConnection
serialPort.on("open", function() {
	console.log('open... waiting 2 seconds');
	var teller = 0;
	setTimeout(sendFile, 2000);
	serialPort.on('data', function(data) {
		// console.log(data.length+": "+data[0].toString(16));
		// console.log(data.toString('ascii'));
		for(var i = 0; i < data.length; i++) {
			if(send) {
				if(incomingByteCounter < 3) {
					incomingInt |= data[i] << (16 - incomingByteCounter * 8);
					incomingByteCounter++;
				}
				if(incomingByteCounter == 3) {
					send = false;
					console.log('pausing send');
					incomingInt = 0;
					incomingByteCounter = 0;
				}
			} else {
				if(incomingByteCounter < 3) {
					incomingInt |= data[i] << (16 - incomingByteCounter * 8);
					incomingByteCounter++;
				}
				if(incomingByteCounter == 3) {
					send = true;
					console.log('resuming send from: ' + incomingInt);
					sendBufferFromPosition(dataBuf, incomingInt);
					incomingInt = 0;
					incomingByteCounter = 0;
				}
			}
		}

  	     //else console.log('received ' + teller++ + ": " + data[0].toString(16));
  	    // else {
  	    // 	if(teller>0){
  	    // 		console.log(data[0].toString(16));
  	    // 		buffy[teller -1 ] = data[0].toString(16);
  	    // 	}
  	    // 	teller++;
  	    // 	// console.log(teller);
  	    // }
  	    // if(complete) {

  	    // }
	});
});

function sendBufferFromPosition(buf, index) {
	var data = buf.slice(index);
	async.eachSeries(data, function(value, callback) {
		if(send) {
			serialPort.write([value], function(error, results) {
				serialPort.drain(function(err, res) {
					callback(err);
				});
			});
		} else {
			callback (new Error("waiting for empty space in buffer"));
		}
	}, function (err) {
		if(err) console.log(err);
		else console.log('sent file');
	});
}

function sendFile() {
	dataBuf = require('fs').readFileSync(process.argv[2]);
	// for(var i = 0; i < dataBuf.length; i++) {
	// 	console.log(dataBuf[i].toString(16));
	// }
	length = dataBuf.length;
	// buffy = new Buffer(length);
	console.log('length ' + length);
	var id = process.argv[3];
	console.log('id ' + id);
	serialPort.write([id[0] /* id.charCodeAt(0) for ascii code */, (length >> 16) & 0xFF, (length >> 8) & 0xFF, length & 0xFF]);
	// for(var i = 0; i < dataBuf.length; i++) {
	// 	// console.log(dataBuf[i].toString(16));
	// 	serialPort.write([dataBuf[i]]);
	// }
	sendBufferFromPosition(dataBuf, 0);
	// for(var i = 0; i < length; i++) {
	// 	console.log(i);
	// 	if(send) {
	// 		// console.log('sending byte ' + i + ': ' + dataBuf[i].toString(16));
	// 		serialPort.write([dataBuf[i]]);
	// 	} else {
	// 		i = i - 1;
	// 		while(!send) {
	// 			sleep.usleep(200000);
	// 			console.log('resending byte ' + i + ': ' + dataBuf[i].toString(16));
	// 			serialPort.write([dataBuf[i]]);
	// 		}
	// 	}
	// }
}



// serialPort.on("open", function () {
//   console.log('open');
//   serialPort.write(new Buffer([2,3,4,5,6]));
//   serialPort.on('data', function(data) {
//   	for(var i = 0; i < data.length; i++) {
//   		console.log(data[i]);
//   	}
//     // console.log('data received: ' + data.length);
//     if(data[0] == 0) {
//     	console.log('pause');
//     	send = false;
//     }
//     if(data[0] == 1) {
//     	console.log('resume');
//     	send = true;
//     }
//     // if(data[0] == 1) {
//     // 	console.log('start');
//     // 	send = true;
//     // }
//   });
//   // serialPort.write(1, function(er, res){});
//   // serialPort.write("ls\n", function(err, results) {
//   // 	serialPort.drain(function(er, res) {
//   // 		console.log('er ' + er);
//   // 		console.log('res ' + res);
//   // 	});
//   //   console.log('err ' + err);
//   //   console.log('results ' + results);
//   // });
//   setTimeout(function(){
//   		serialPort.write(new Buffer([3]), function(err, res){});
//   }, 2000);

//   setInterval(function(){
//   	if(send) {
//   		serialPort.write(new Buffer([2]), function(err, res){
//   			console.log('writing');
//   		});

//   	}
//   }, 100);
// });
