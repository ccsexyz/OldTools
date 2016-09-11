package main

import (
	"container/list"
	"encoding/binary"
	"fmt"
	"io"
	"net"
	"os"
	"strconv"
	"time"
)

type rateInfo struct {
	bytes int
	ch    chan bool
}

var (
	readLimitChan chan rateInfo
	writLimitChan chan rateInfo
)

func main() {
	checkArgs()
	tcpAddr, err := net.ResolveTCPAddr("tcp4", os.Args[1])
	checkErr(err)
	listener, err := net.ListenTCP("tcp", tcpAddr)
	checkErr(err)

	go limitRateRoutine("read", readLimitChan)
	go limitRateRoutine("write", writLimitChan)

	for {
		conn, err := listener.Accept()
		checkErr(err)
		go handleClient(conn)
	}
}

func init() {
	readLimitChan = make(chan rateInfo, 16)
	writLimitChan = make(chan rateInfo, 16)
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

	f := func(self net.Conn, selfQueue chan string, pairQueue chan string, finishQueue chan bool, limit chan rateInfo) {
		buf := make([]byte, 16384)
		finish := make(chan bool)
		pending := make(chan bool)

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
				limit <- rateInfo{len, pending}
				<-pending
				pairQueue <- string(buf[0:len])
			}
		}
	}

	go f(conn2, queue2, queue1, finished, writLimitChan)
	f(conn, queue1, queue2, finished, readLimitChan)
}

func limitRateRoutine(prefix string, ch chan rateInfo) {
	t := time.NewTicker(time.Second)
	totalBytes := 0
	bytesPerSecond := 0
	waitedSeconds := 0
	pendingInfoList := list.New()

	var bytesLimit int
	if len(os.Args) == 3 {
		bytesLimit, _ = strconv.Atoi(os.Args[2])
	} else {
		bytesLimit = 0
	}

	do := func(info rateInfo) {
		bytes := info.bytes
		ch := info.ch
		totalBytes += bytes
		bytesPerSecond += bytes
		ch <- true
	}

	for {
		select {
		case <-t.C:
			fmt.Printf("%v speed: %v\n", prefix, bytesPerSecond)
			// fmt.Printf("%v waited seconds: %v\n", prefix, waitedSeconds)
			bytesPerSecond = 0
			for {
				e := pendingInfoList.Front()
				if e != nil {
					info := e.Value.(rateInfo)

					if info.bytes+bytesPerSecond > waitedSeconds*bytesLimit+bytesLimit {
						waitedSeconds++
						break
					} else {
						waitedSeconds = 0
					}

					do(info)
					pendingInfoList.Remove(e)
					if bytesPerSecond > bytesLimit {
						break
					}
				} else {
					break
				}
			}
		case info := <-ch:
			if bytesLimit == 0 || bytesPerSecond+info.bytes < bytesLimit {
				do(info)
			} else {
				// fmt.Println("pending ", info.bytes, " bytes")
				pendingInfoList.PushBack(info)
			}
		}
	}
}

func checkArgs() {
	if len(os.Args) != 2 && len(os.Args) != 3 {
		fmt.Fprintf(os.Stderr, "usage: proxy local_addr limit_rate\n")
		os.Exit(1)
	}
}

func checkErr(err error) {
	if err != nil {
		fmt.Fprintf(os.Stderr, "Fatal error: %s", err.Error())
	}
}
