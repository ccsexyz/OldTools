package main

import (
	"fmt"
	"io"
	"net"
	"os"
	"time"
)

func main() {
	go runEchoServer()
	go runDaytimeServer()
	go runDiscardServer()
	runChatServer()
}

func runEchoServer() {
	service := ":9377"
	tcpAddr, err := net.ResolveTCPAddr("tcp4", service)
	checkErr(err)
	listener, err := net.ListenTCP("tcp", tcpAddr)
	checkErr(err)
	for {
		conn, err := listener.Accept()
		if err != nil {
			continue
		}
		go func(conn net.Conn) {
			defer conn.Close()
			io.Copy(conn, conn)
		}(conn)
	}
}

func runDaytimeServer() {
	service := ":9378"
	tcpAddr, err := net.ResolveTCPAddr("tcp4", service)
	checkErr(err)
	listener, err := net.ListenTCP("tcp", tcpAddr)
	checkErr(err)
	for {
		conn, err := listener.Accept()
		checkErr(err)
		go func(conn net.Conn) {
			defer conn.Close()
			now := time.Now()
			io.WriteString(conn, fmt.Sprintln(now))
		}(conn)
	}
}

func runDiscardServer() {
	service := ":9379"
	tcpAddr, err := net.ResolveTCPAddr("tcp4", service)
	checkErr(err)
	listener, err := net.ListenTCP("tcp", tcpAddr)
	checkErr(err)
	for {
		conn, err := listener.Accept()
		checkErr(err)
		go func(conn net.Conn) {
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
		}(conn)
	}
}

var (
	queues map[string]chan string
)

func runChatServer() {
	queues = make(map[string]chan string)
	service := ":9391"
	tcpAddr, err := net.ResolveTCPAddr("tcp4", service)
	checkErr(err)
	listener, err := net.ListenTCP("tcp", tcpAddr)
	checkErr(err)
	for {
		conn, err := listener.Accept()
		checkErr(err)
		go func(conn net.Conn) {
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
		}(conn)
	}
}

func checkErr(err error) {
	if err != nil {
		fmt.Fprintf(os.Stderr, "Fatal error: %s", err.Error())
	}
}
