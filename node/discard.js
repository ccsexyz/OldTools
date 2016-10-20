var net = require('net');

var server = net.createServer(function(socket) {
    socket.write('Discard Server\r\n');
});

server.listen(9378, '127.0.0.1');