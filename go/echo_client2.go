package main

import (
	"fmt"
	"net"
	"os"
	"strconv"
	"time"
)

var (
	msg_num  int
	msg_len  int
	msg_str  []byte
	num_chan chan int
)

func main() {
	if len(os.Args) != 7 {
		print_usage()
		return
	}
	msg_len, _ = strconv.Atoi(os.Args[6])
	msg_num, _ = strconv.Atoi(os.Args[5])
	concurrency_, _ := strconv.Atoi(os.Args[3])
	active_conns, _ := strconv.Atoi(os.Args[4])
	num_chan = make(chan int)
	generate_string()
	ticker := time.NewTicker(time.Second * 1)
	start := time.Now()
	bytes := 0

	i := 0
	for {
		conn, err := net.Dial("tcp4", os.Args[1]+":"+os.Args[2])
		if err != nil {
			fmt.Printf("connect error!")
		} else {
			i++
			if i <= active_conns {
				ch := make(chan int)
				go write_message(conn, ch)
				go read_message(conn, ch)
			}
			if concurrency_ == i {
				break
			}
		}
	}

	go func() {
		lastBytes := bytes
		for _ = range ticker.C {
			fmt.Printf("read %d bytes\n", bytes-lastBytes)
			lastBytes = bytes
		}
	}()

	i = 0
	for {
		bytes += <-num_chan
		i++
		if active_conns == i {
			break
		}
	}

	end := time.Now()
	delta := end.Sub(start)
	seconds := delta.Seconds()
	bytes_per_second := (float64(bytes) / seconds)
	speed := bytes_per_second / (1024 * 1024)

	fmt.Printf("%f MB/s in %fs\n", speed, delta.Seconds())
}

func print_usage() {
	fmt.Printf("usage: echo_client server_path server_port concurrency active_conns msg_num msg_len\n")
}

func generate_string() {
	msg_str = make([]byte, msg_len)
	i := 0
	for {
		msg_str[i] = 'A'
		i += 1
		if i == msg_len {
			return
		}
	}
}

func write_message(conn net.Conn, ch chan int) {
	bytes := 0
	total := msg_len * msg_num
	for {
		byte, err := conn.Write(msg_str)
		if err != nil {
			break
		}
		bytes += byte
		if bytes >= total {
			break
		}
	}
	ch <- 1
}

func read_message(conn net.Conn, ch chan int) {
	defer conn.Close()
	bytes := 0
	total := msg_len * msg_num
	b := make([]byte, msg_len)
	for {
		byte, err := conn.Read(b[0:])
		if err != nil {
			fmt.Println(err.Error())
			break
		}
		bytes += byte
		if bytes >= total {
			break
		}
	}
	<-ch
	num_chan <- bytes
}
