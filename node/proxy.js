var net = require('net');
var util = require('util');

if (process.argv.length != 4) {
    console.log('usage: node proxy.js localaddr localport');
    process.exit(1);
}

var localaddr = process.argv[2];
var localport = process.argv[3];

let reading_socks_version = 0;
let reading_socks4_command = 1;
let reading_socks4_handshake = 2;
let transfering_data = 3;
let reading_socks5_nmethod = 4;
let reading_socks5_methods = 5;
let reading_socks5_atyp = 6;
let handle_socks5_atyp1 = 7;
let handle_socks5_atyp3 = 8;

function convert_address_to_string(address) {
    return address.family + '//' + address.address + ':' + address.port;
};

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
    let stat = reading_socks_version;
    let buffer = new Buffer(1024);
    let startIndex = 0;
    let endIndex = 0;
    let version = 0;
    let client = null;
    let excepted_length = 1;
    let len = 0;
    socket.on('data', (chunk) => {
        if (stat != transfering_data) {
            for (let it = 0; it < chunk.length; it++) {
                buffer[endIndex + it] = chunk[it];
            }
            endIndex = endIndex + chunk.length;
            chunk = null;
        }

        for (;;) {
            if (stat !== transfering_data && startIndex + excepted_length > endIndex) {
                return;
            }
            if (stat === reading_socks_version) {
                version = buffer[startIndex];
                if (version === 4) {
                    startIndex = startIndex + excepted_length;
                    stat = reading_socks4_command;
                    continue;
                } else if (version === 5) {
                    startIndex = startIndex + excepted_length;
                    stat = reading_socks5_nmethod;
                    continue;
                }
            } else if (stat === reading_socks4_command) {
                let command = buffer[startIndex];
                if (command == 1) {
                    startIndex = startIndex + excepted_length;
                    excepted_length = 6;
                    stat = reading_socks4_handshake;
                    continue;
                }
            } else if (stat === reading_socks4_handshake) {
                let host = util.format('%d.%d.%d.%d',
                    buffer.readUInt8(startIndex + 2), buffer.readUInt8(startIndex + 3),
                    buffer.readUInt8(startIndex + 4), buffer.readUInt8(startIndex + 5));
                let port = buffer.readUInt16BE(startIndex);
                startIndex = startIndex + excepted_length;
                excepted_length = 0;
                stat = transfering_data;
                buffer = null;
                // console.log(host + ' ' + port);
                client = net.connect(port, host, () => {
                    if (version === 4) {
                        const msg = Buffer.from([0x0, 0x5A, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]);
                        socket.write(msg);
                    }
                    socket.resume();
                    client.on('data', make_relay_func(socket));
                    socket.on('error', make_destroy_func(client));
                    client.on('end', make_destroy_func(socket));
                    socket.on('end', make_destroy_func(client));
                });
                socket.pause();
                client.on('error', make_destroy_func(socket));
                return;
            } else if (stat === transfering_data) {
                if (client !== null) {
                    client.write(chunk);
                    return;
                }
            } else if (stat === reading_socks5_nmethod) {
                let nmethods = buffer.readUInt8(startIndex);
                startIndex = startIndex + excepted_length;
                excepted_length = nmethods;
                stat = reading_socks5_methods;
                continue;
            } else if (stat === reading_socks5_methods) {
                let method = buffer.readUInt8(startIndex);
                if (method === 0) {
                    startIndex = startIndex + excepted_length;
                    excepted_length = 6;
                    stat = reading_socks5_atyp;
                    socket.write(Buffer.from([0x5, 0x0]));
                    continue;
                }
            } else if (stat === reading_socks5_atyp) {
                let ver = buffer[startIndex];
                let cmd = buffer[startIndex + 1];
                let atyp = buffer[startIndex + 3];
                len = buffer[startIndex + 4];
                if (ver === 5 && cmd === 1 && (atyp === 1 || atyp === 3)) {
                    startIndex = startIndex + excepted_length;
                    if (atyp === 1) {
                        excepted_length = 4;
                        stat = handle_socks5_atyp1;
                    } else {
                        excepted_length = len + 1;
                        stat = handle_socks5_atyp3;
                    }
                    continue;
                }
            } else if (stat === handle_socks5_atyp1) {
                startIndex = startIndex - 6;
                let host = util.format('%d.%d.%d.%d',
                    buffer.readUInt8(startIndex + 4), buffer.readUInt8(startIndex + 5),
                    buffer.readUInt8(startIndex + 6), buffer.readUInt8(startIndex + 7));
                let port = buffer.readUInt16BE(startIndex + 8);
                excepted_length = 0;
                stat = transfering_data;
                client = net.connect(port, host, () => {
                    if (version === 4) {
                        const msg = Buffer.from([0x0, 0x5A, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]);
                        socket.write(msg);
                    } else if (version === 5) {
                        let host_buf = Buffer.from(host);
                        const msg = Buffer.from([0x5, 0x0, 0x0, 0x3, host_buf.length]);
                        socket.write(msg);
                        socket.write(host_buf);
                        socket.write(Buffer.from(buffer, 8, 2));
                    }
                    buffer = null;
                    socket.resume();
                    client.on('data', make_relay_func(socket));
                    socket.on('error', make_destroy_func(client));
                    client.on('end', make_destroy_func(socket));
                    socket.on('end', make_destroy_func(client));
                });
                socket.pause();
                client.on('error', make_destroy_func(socket));
                return;
            } else if (stat === handle_socks5_atyp3) {
                startIndex = startIndex - 6;
                let host = buffer.toString('utf8', startIndex + 5, startIndex + len + 5);
                let port = buffer.readUInt16BE(startIndex + len + 5);
                console.log(host, port);
                excepted_length = 0;
                stat = transfering_data;
                client = net.connect(port, host, () => {
                    if (version === 4) {
                        const msg = Buffer.from([0x0, 0x5A, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]);
                        socket.write(msg);
                    } else if (version === 5) {
                        let host_buf = Buffer.from(host);
                        const msg = Buffer.from([0x5, 0x0, 0x0, 0x3, host_buf.length]);
                        socket.write(msg);
                        socket.write(host_buf);
                        socket.write(Buffer.from([buffer[startIndex + len + 5], buffer[startIndex + len + 6]]));
                    }
                    buffer = null;
                    socket.resume();
                    client.on('data', make_relay_func(socket));
                    socket.on('error', make_destroy_func(client));
                    client.on('end', make_destroy_func(socket));
                    socket.on('end', make_destroy_func(client));
                });
                socket.pause();
                client.on('error', make_destroy_func(socket));
                return;
            }

            socket.destroy();
            return;
        }
    });
});

server.listen(localport, localaddr);