package main

import (
	"bufio"
	"encoding/csv"
	"encoding/json"
	"log/slog"
	"net"
	"os"
	"path/filepath"
	"strconv"
	"time"

	"go.bug.st/serial"

	"formula-logger/data-server/config"
)

type MQTTData struct {
	Topic   string `json:"topic"`
	Payload string `json:"payload"`
}

type CSVData struct {
	MQTTData
	Time int64 `json:"time"`
}

// CSVファイル作成
func prepareDataFile() string {
	loc, _ := time.LoadLocation("Asia/Tokyo")
	now := time.Now().In(loc)
	os.MkdirAll("data", 0755)
	return filepath.Join("data", now.Format("20060102_150405")+".csv")
}

// シリアルポートを開く
func openSerialPort(portName string, baud int) serial.Port {
	mode := &serial.Mode{BaudRate: baud}
	port, err := serial.Open(portName, mode)
	if err != nil {
		slog.Error("Failed to open serial port", "port", portName, "baud", baud, "error", err)
		panic(err)
	}
	slog.Info("Serial port opened", "port", portName, "baud", baud)
	return port
}

// CSV保存 goroutine
func logCSV(ch <-chan CSVData, filePath string, flushInterval time.Duration) {
	file, err := os.Create(filePath)
	if err != nil {
		slog.Error("Failed to create CSV file", "path", filePath, "error", err)
		return
	}
	defer file.Close()

	writer := bufio.NewWriter(file)
	csvWriter := csv.NewWriter(writer)
	defer csvWriter.Flush()

	csvWriter.Write([]string{"time", "topic", "payload"})
	csvWriter.Flush()
	writer.Flush()

	ticker := time.NewTicker(flushInterval)
	defer ticker.Stop()

	for {
		select {
		case data := <-ch:
			record := []string{strconv.FormatInt(data.Time, 10), data.Topic, data.Payload}
			if err := csvWriter.Write(record); err != nil {
				slog.Error("CSV write error", "record", record, "error", err)
			}
		case <-ticker.C:
			csvWriter.Flush()
			writer.Flush()
		}
	}
}

// Unixソケット送信 goroutine（トピックごとに10回に1回送信）
func sendLoop(ch <-chan MQTTData, socketPath string) {
	var conn net.Conn
	var err error

	counts := make(map[string]int)

	for {
		if conn == nil {
			conn, err = net.Dial("unix", socketPath)
			if err != nil {
				time.Sleep(10 * time.Millisecond)
				continue
			}
			slog.Info("Connected to MQTT process via unix socket")
		}

		select {
		case data := <-ch:
			counts[data.Topic]++
			if counts[data.Topic]%10 != 0 {
				continue
			}

			msg, err := json.Marshal(data)
			if err != nil {
				slog.Error("JSON marshal error", "data", data, "error", err)
				continue
			}

			_, err = conn.Write(append(msg, '\n'))
			if err != nil {
				slog.Error("Failed to write to unix socket", "error", err)
				conn.Close()
				conn = nil
			} else {
				slog.Info("Published to MQTT process", "topic", data.Topic, "payload", data.Payload)
			}
		default:
			time.Sleep(1 * time.Millisecond)
		}
	}
}

// Serial受信
func readSerial(port serial.Port, timeStart time.Time, chMQTT chan<- MQTTData, chCSV chan<- CSVData) {
	scanner := bufio.NewScanner(port)
	for scanner.Scan() {
		line := scanner.Text()
		var data MQTTData
		if err := json.Unmarshal([]byte(line), &data); err != nil {
			slog.Error("JSON parse error", "line", line, "error", err)
			continue
		}

		chCSV <- CSVData{
			MQTTData: data,
			Time:     time.Since(timeStart).Milliseconds(),
		}

		select {
		case chMQTT <- data:
		default:
		}

		slog.Info("Serial received", "topic", data.Topic, "payload", data.Payload)
	}

	if err := scanner.Err(); err != nil {
		slog.Error("Serial scanner error", "error", err)
	}
}

func main() {
	// stdoutに構造化ログ
	logger := slog.New(slog.NewTextHandler(os.Stdout, nil))
	slog.SetDefault(logger)

	cfg := config.LoadConfig("config.toml")

	filePath := prepareDataFile()
	slog.Info("CSV output path", "path", filePath)

	port := openSerialPort(cfg.Serial.Port, cfg.Serial.Baud)
	defer port.Close()

	chMQTT := make(chan MQTTData, 1000)
	chCSV := make(chan CSVData, 1000)

	go logCSV(chCSV, filePath, time.Duration(cfg.CSV.FlushInterval)*time.Second)
	go sendLoop(chMQTT, cfg.Socket.Serial)

	readSerial(port, time.Now(), chMQTT, chCSV)
}
