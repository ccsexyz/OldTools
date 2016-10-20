var net = require('net');
var map = new Map();

function convert_address_to_string(address) {
    return address.family + '//' + address.address + ':' + address.port;
};

var server = net.createServer(function(socket) {
    var address = convert_address_to_string(socket.address());
    console.log(address + ' enter the chat room');
    socket.write('Chat Server\n');
    map.set(address, socket);

    var clean_func = () => {
        map.delete(address);
        console.log(address + ' leave the chat room');
    };
    socket.on('data', (chunk) => {
        var out_str = address + ' said: ' + chunk.toString();
        if (out_str.endsWith('\n')) {
            out_str = out_str.substring(0, out_str.length - 1);
        }
        console.log(out_str);
        for (var [addr, s] of map) {
            if (addr !== address) {
                s.write(chunk);
            }
        }
    });
    socket.on('error', (err) => {
        console.log('error');
        clean_func();
    });
    socket.on('end', clean_func);
});

server.listen(9390, '127.0.0.1');