var net = require('net');

if (process.argv.length !== 6) {
    console.log('usage: node relayer.js localaddr localport remoteaddr remoteport');
    process.exit(1);
}

var localaddr = process.argv[2];
var localport = process.argv[3];
var remoteaddr = process.argv[4];
var remoteport = process.argv[5];

function make_destroy_func(c) {
    return () => {
        c.destroy();
    };
}

function make_relay_func(c) {
    return (chunk) => {
        c.write(chunk);
    };
}

var server = net.createServer((socket) => {
    var client = net.connect(remoteport, remoteaddr, () => {
        socket.on('end', make_destroy_func(client));
        client.on('end', make_destroy_func(socket));
        socket.on('error', make_destroy_func(client));
        client.on('error', make_destroy_func(socket));
        socket.on('data', make_relay_func(client));
        client.on('data', make_relay_func(socket));
    });
    client.on('error', make_destroy_func(socket));
});
server.listen(localport, localaddr);