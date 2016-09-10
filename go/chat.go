package main

import (
	"fmt"
	"io"
	"net"
	"os"
)

var (
	queues map[string]chan string
)

func init() {
	queues = make(map[string]chan string)
}

func main() {
	service := ":9391"
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
	finish := make(chan bool)
	queue := make(chan string, 16)

	remoteAddr := conn.RemoteAddr().String()
	queues[remoteAddr] = queue

	defer func() {
		delete(queues, remoteAddr)
	}()

	go func() {
		defer conn.Close()
		for {
			select {
			case <-finish:
				return
			case str := <-queue:
				if _, err := io.WriteString(conn, str); err != nil {
					return
				}
			}
		}
	}()

	buf := make([]byte, 4096)

	for {
		len, err := conn.Read(buf)

		if err != nil {
			finish <- true
			return
		} else {
			go func(str string, self string) {
				for name, queue := range queues {
					if name == self {
						continue
					}

					queue <- str
				}
			}(string(buf[0:len]), remoteAddr)
		}
	}
}

func checkErr(err error) {
	if err != nil {
		fmt.Fprintf(os.Stderr, "Fatal error: %s", err.Error())
	}
}
