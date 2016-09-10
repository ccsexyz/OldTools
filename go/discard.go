package main

import (
	"fmt"
	"net"
	"os"
)

func main() {
	service := ":9378"
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
	buf := make([]byte, 128)
	for {
		len, err := conn.Read(buf)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Fatal error: %s", err.Error())
			break
		} else {
			it := len - 1
			for {
				if it >= 0 && buf[it] == '\n' {
					it--
				} else {
					break
				}
			}
			fmt.Printf("Clinet discard: %v\n", string(buf[0:it]))
		}
	}
}

func checkErr(err error) {
	if err != nil {
		fmt.Fprintf(os.Stderr, "Fatal error: %s", err.Error())
	}
}
