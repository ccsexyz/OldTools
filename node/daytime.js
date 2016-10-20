var net = require('net');

var server = net.createServer(function(socket) {
    var date = new Date();
    socket.write(date.toUTCString() + '\n');
    socket.destroy();
});

server.listen(9379, '127.0.0.1');