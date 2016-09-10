package main

import (
	"fmt"
	"io"
	"net"
	"os"
)

func main() {
	checkArgs()
	tcpAddr, err := net.ResolveTCPAddr("tcp4", os.Args[1])
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
	conn2, err := net.Dial("tcp4", os.Args[2])
	queue1 := make(chan string, 4)
	queue2 := make(chan string, 4)
	finished := make(chan bool, 2)

	if err != nil {
		fmt.Fprintf(os.Stderr, "Connect error: %s", err.Error())
		return
	}

	f := func(self net.Conn, selfQueue chan string, pairQueue chan string, finishQueue chan bool) {
		buf := make([]byte, 1024)
		finish := make(chan bool)

		go func() {
			defer func() {
				self.Close()
				finishQueue <- true
			}()
			for {
				select {
				case <-finish:
					return
				case <-finishQueue:
					return
				case str := <-selfQueue:
					if _, err := io.WriteString(self, str); err != nil {
						return
					}
				}
			}
		}()

		for {
			len, err := self.Read(buf)

			if err != nil {
				finish <- true
				return
			} else {
				pairQueue <- string(buf[0:len])
			}
		}
	}

	go f(conn2, queue2, queue1, finished)
	f(conn, queue1, queue2, finished)
}

func checkArgs() {
	if len(os.Args) != 3 {
		fmt.Fprintf(os.Stderr, "usage: relayer local_addr remote_addr\n")
		os.Exit(1)
	}
}

func checkErr(err error) {
	if err != nil {
		fmt.Fprintf(os.Stderr, "Fatal error: %s", err.Error())
	}
}
