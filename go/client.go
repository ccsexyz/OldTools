package main

import (
	"fmt"
	"io"
	"net"
	"os"
)

var (
	msg chan int
)

func main() {
	if len(os.Args) != 3 {
		print_usage()
		return
	}
	conn, err := net.Dial("tcp", os.Args[1]+":"+os.Args[2])
	if err != nil {
		fmt.Println("connect error ", err.Error())
		return
	}
	go echo(conn)
	io.Copy(os.Stdin, conn)
}

func print_usage() {
	fmt.Printf("usage: ./echo_client server_name server_port!\n")
}

func echo(conn net.Conn) {
	io.Copy(conn, os.Stdout)
}
