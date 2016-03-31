package main

import (
	"fmt"
	"io"
	"net"
	"os"
	"strconv"
)

var (
	ch chan int
)

func main() {
	if len(os.Args) != 4 {
		fmt.Printf("usage: ./pingpong name port conns\n")
		return
	}
	concurrency, _ := strconv.Atoi(os.Args[3])
	ch = make(chan int)
	i := 0
	for {
		conn, err := net.Dial("tcp4", os.Args[1]+":"+os.Args[2])
		if err != nil {
			fmt.Println(err.Error())
			return
		} else {
			go handleMessage(conn)
			i++
			if i == concurrency {
				break
			}
		}
	}
	i = 0
	for {
		<-ch
		i++
		if i == concurrency {
			break
		}
	}
}

func handleMessage(conn net.Conn) {
	defer conn.Close()
	msg_str := make([]byte, 40960)
	_, err := conn.Write(msg_str)
	if err != nil {
		fmt.Println(err.Error())
	} else {
		io.Copy(conn, conn)
	}
	ch <- 1
}
