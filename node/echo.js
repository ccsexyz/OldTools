var net = require('net');

var server = net.createServer(function(socket) {
    socket.write('Echo Server\r\n');
    socket.pipe(socket);

    socket.on('error', function(err) {
        console.log('error');
    });
});

server.listen(8089, '127.0.0.1');