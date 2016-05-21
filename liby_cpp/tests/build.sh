#! /bin/sh

cc=g++-5
cflags='-lpthread -std=c++14 -Wall -Werror'
$cc -g -c ../*.cpp $cflags
$cc -g -c ../http/*.cpp $cflags
$cc -g -o all all.cpp *.o $cflags
$cc -g -o echo echo.cpp *.o $cflags
$cc -g -o daytime daytime.cpp *.o $cflags
$cc -g -o discard discard.cpp *.o $cflags
$cc -g -o chat chat.cpp *.o $cflags
$cc -g -o echo_client echo_client.cpp *.o $cflags
$cc -g -o tcp_relayer tcp_relayer.cpp *.o $cflags
$cc -g -o tcp_proxy tcp_proxy.cpp *.o $cflags
$cc -g -o httpd httpd.cpp *.o $cflags
