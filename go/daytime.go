package main

import (
	"fmt"
	"io"
	"net"
	"os"
	"time"
)

func main() {
	service := ":9390"
	tcpAddr, err := net.ResolveTCPAddr("tcp4", service)
	checkErr(err)
	listener, err := net.ListenTCP("tcp", tcpAddr)
	checkErr(err)
	for {
		conn, err := listener.Accept()
		checkErr(err)
		go handleClient(conn)
	}
}

func handleClient(conn net.Conn) {
	defer conn.Close()
	now := time.Now()
	io.WriteString(conn, fmt.Sprintln(now))
}

func checkErr(err error) {
	if err != nil {
		fmt.Fprintf(os.Stderr, "Fatal error: %s", err.Error())
	}
}
