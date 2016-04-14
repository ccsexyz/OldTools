#! /bin/sh

cc=g++-5
cflags='-lpthread -std=c++14 -Wall -Werror'
$cc -g -o all all.cpp ../*.cpp $cflags
$cc -g -o echo echo.cpp ../*.cpp $cflags
$cc -g -o daytime daytime.cpp ../*.cpp $cflags
$cc -g -o discard discard.cpp ../*.cpp $cflags
$cc -g -o chat chat.cpp ../*.cpp $cflags
$cc -g -o echo_client echo_client.cpp ../*.cpp $cflags
$cc -g -o tcp_relayer tcp_relayer.cpp ../*.cpp $cflags
$cc -g -o tcp_proxy tcp_proxy.cpp ../*.cpp $cflags
$cc -g -o httpd httpd.cpp ../*.cpp ../http/*.cpp $cflags
