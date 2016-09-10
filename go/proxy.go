package main

import (
	"encoding/binary"
	"fmt"
	"io"
	"net"
	"os"
	"strconv"
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
	defer conn.Close()

	buf := make([]byte, 128)
	if len, err := io.ReadAtLeast(conn, buf, 9); len != 9 || err != nil {
		return
	}

	ver := uint8(buf[0])
	command := uint8(buf[1])

	if ver != 4 || command != 1 {
		fmt.Println("bad version or command")
		return
	}

	host := net.IP(buf[4:8]).String()
	port := binary.BigEndian.Uint16(buf[2:4])
	addr := net.JoinHostPort(host, strconv.Itoa(int(port)))

	conn2, err := net.Dial("tcp4", addr)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Connect error: %s\n", err.Error())
		return
	} else {
		_, err := conn.Write([]byte{0x0, 0x5A, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0})
		if err != nil {
			fmt.Fprintf(os.Stderr, "write error: %s\n", err.Error())
			conn2.Close()
			return
		}
	}

	queue1 := make(chan string, 4)
	queue2 := make(chan string, 4)
	finished := make(chan bool, 2)

	f := func(self net.Conn, selfQueue chan string, pairQueue chan string, finishQueue chan bool) {
		buf := make([]byte, 16384)
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
			fmt.Println("try to read sth")
			len, err := self.Read(buf)
			fmt.Println("read sth")

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
	if len(os.Args) != 2 {
		fmt.Fprintf(os.Stderr, "usage: proxy local_addr\n")
		os.Exit(1)
	}
}

func checkErr(err error) {
	if err != nil {
		fmt.Fprintf(os.Stderr, "Fatal error: %s", err.Error())
	}
}
