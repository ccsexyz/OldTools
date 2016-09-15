package main

import (
	"container/list"
	"encoding/binary"
	"flag"
	"fmt"
	"io"
	"net"
	"os"
	"strconv"
	"time"
)

var (
	daytimeService string
	echoService    string
	discardService string
	chatService    string
	socksService   string
	bytesLimit     int
	printSpeed     bool
)

func main() {
	flag.StringVar(&daytimeService, "t", ":9378", "daytime server path and port")
	flag.StringVar(&echoService, "e", ":9377", "echo server path and port")
	flag.StringVar(&discardService, "d", ":9379", "discard server path and port")
	flag.StringVar(&chatService, "c", ":9391", "chat server path and port")
	flag.StringVar(&socksService, "s", ":8888", "socks proxy server path and port")
	flag.IntVar(&bytesLimit, "b", 0, "bytesLimit for socks proxy")
	flag.BoolVar(&printSpeed, "pb", false, "print speed of proxy")

	flag.Parse()

	go runEchoServer()
	go runDaytimeServer()
	go runDiscardServer()
	go runChatServer()
	runSocksProxy()
}

func runEchoServer() {
	service := echoService
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
	service := daytimeService
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
	service := discardService
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
	service := chatService
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
				return
			}
		}(conn)
	}
}

type rateInfo struct {
	bytes int
	ch    chan bool
}

var (
	readLimitChan chan rateInfo
	writLimitChan chan rateInfo
)

func init() {
	readLimitChan = make(chan rateInfo)
	writLimitChan = make(chan rateInfo)
}

func runSocksProxy() {
	runLimitRateRoutine(bytesLimit)

	service := socksService
	tcpAddr, err := net.ResolveTCPAddr("tcp4", service)
	checkErr(err)
	listener, err := net.ListenTCP("tcp", tcpAddr)
	checkErr(err)
	for {
		conn, err := listener.Accept()
		checkErr(err)
		go func(conn net.Conn) {
			defer conn.Close()

			ver := make([]byte, 1)
			if len, err := io.ReadAtLeast(conn, ver[0:1], 1); len != 1 || err != nil {
				return
			}

			var ok bool
			var conn2 net.Conn

			if ver[0] == 4 {
				conn2, ok = socks4HandShake(conn)
			} else if ver[0] == 5 {
				conn2, ok = socks5HandShake(conn)
			} else {
				return
			}

			if ok == false {
				return
			}

			defer conn2.Close()

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
						break
					} else {
						limit <- rateInfo{len, pending}
						<-pending
						pairQueue <- string(buf[0:len])
					}
				}
			}

			go f(conn2, queue2, queue1, finished, writLimitChan)
			f(conn, queue1, queue2, finished, readLimitChan)
		}(conn)
	}
}

func socks4HandShake(conn net.Conn) (conn2 net.Conn, ok bool) {
	ok = false
	buf := make([]byte, 128)
	if len, err := io.ReadAtLeast(conn, buf, 8); len != 8 || err != nil {
		return
	}

	command := buf[0]
	if command != 1 {
		return
	}

	host := net.IP(buf[3:7]).String()
	port := binary.BigEndian.Uint16(buf[1:3])
	addr := net.JoinHostPort(host, strconv.Itoa(int(port)))

	conn2, err := net.Dial("tcp4", addr)
	if err == nil {
		_, err := conn.Write([]byte{0x0, 0x5A, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0})
		if err != nil {
			conn2.Close()
		} else {
			ok = true
		}
	}

	return
}

func socks5HandShake(conn net.Conn) (conn2 net.Conn, ok bool) {
	ok = false
	buf := make([]byte, 512)
	if len, err := io.ReadAtLeast(conn, buf[0:1], 1); len != 1 || err != nil {
		return
	}

	nmethods := int(buf[0])

	if len, err := io.ReadAtLeast(conn, buf[0:nmethods], nmethods); len != nmethods || err != nil {
		return
	}

	if method := buf[0]; method != 0 {
		return
	}

	if _, err := conn.Write([]byte{0x5, 0x0}); err != nil {
		return
	}

	if len, err := io.ReadAtLeast(conn, buf[0:6], 6); len != 6 || err != nil {
		return
	}

	ver := buf[0]
	cmd := buf[1]
	atyp := buf[3]

	if ver != 0x5 || cmd != 0x1 || (atyp != 0x1 && atyp != 0x3) {
		return
	}

	var host string
	var port uint16
	var addr string
	var portSlice []byte

	if atyp == 0x1 {
		if _, err := io.ReadAtLeast(conn, buf[6:10], 4); err != nil {
			return
		}

		host = net.IP(buf[4:8]).String()
		port = binary.BigEndian.Uint16(buf[8:10])
		addr = net.JoinHostPort(host, strconv.Itoa(int(port)))

		portSlice = buf[8:10]
	} else if atyp == 0x3 {
		len := int(buf[4])
		if l, err := io.ReadAtLeast(conn, buf[6:len+7], len+1); l != len+1 || err != nil {
			return
		}

		host = string(buf[5 : len+5])
		port = binary.BigEndian.Uint16(buf[len+5 : len+7])
		addr = net.JoinHostPort(host, strconv.Itoa(int(port)))

		portSlice = buf[len+5 : len+7]
	}

	conn2, err := net.Dial("tcp4", addr)
	if err != nil {
		return
	}

	_, err = conn.Write([]byte{0x5, 0x0, 0x0, 0x3, byte(len(host))})
	if err != nil {
		conn2.Close()
		return
	}

	if _, err = conn.Write([]byte(host)); err != nil {
		conn2.Close()
		return
	}

	if _, err = conn.Write(portSlice); err != nil {
		conn2.Close()
		return
	}

	ok = true
	return
}

func runLimitRateRoutine(bytesLimit int) {
	go limitRateRoutine("read", readLimitChan, bytesLimit)
	go limitRateRoutine("write", writLimitChan, bytesLimit)
}

func limitRateRoutine(prefix string, ch chan rateInfo, bytesLimit int) {
	t := time.NewTicker(time.Second)
	// totalBytes := 0
	bytesPerSecond := 0
	waitedSeconds := 0
	pendingInfoList := list.New()

	do := func(info rateInfo) {
		bytes := info.bytes
		ch := info.ch
		// totalBytes += bytes
		bytesPerSecond += bytes
		ch <- true
	}

	for {
		select {
		case <-t.C:
			if printSpeed {
				fmt.Printf("%v speed: %v\n", prefix, bytesPerSecond)
			}
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

func checkErr(err error) {
	if err != nil {
		fmt.Fprintf(os.Stderr, "Fatal error: %s", err.Error())
	}
}
