package main

import (
	"bufio"
	"encoding/json"
	"log/slog"
	"net"
	"os"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

type MQTTData struct {
	Topic   string `json:"topic"`
	Payload string `json:"payload"`
}

func main() {
	// stdoutに構造化ログ
	logger := slog.New(slog.NewTextHandler(os.Stdout, nil))
	slog.SetDefault(logger)
	slog.Info("MQTT process started")

	opts := mqtt.NewClientOptions().
		AddBroker("tcp://192.168.11.10:1883").
		SetClientID("go-mqtt-publisher").
		SetKeepAlive(30 * time.Second).
		SetPingTimeout(10 * time.Second).
		SetAutoReconnect(true)

	opts.OnConnect = func(c mqtt.Client) {
		slog.Info("MQTT connected")
	}
	opts.OnConnectionLost = func(c mqtt.Client, err error) {
		slog.Error("MQTT connection lost", "error", err)
	}

	client := mqtt.NewClient(opts)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		slog.Error("MQTT connect failed", "error", token.Error())
		panic(token.Error())
	}

	socketPath := "/tmp/serial.sock"
	os.Remove(socketPath)
	ln, err := net.Listen("unix", socketPath)
	if err != nil {
		slog.Error("Failed to listen unix socket", "path", socketPath, "error", err)
		panic(err)
	}
	defer ln.Close()
	slog.Info("Listening on unix socket", "path", socketPath)

	for {
		conn, err := ln.Accept()
		if err != nil {
			slog.Error("Accept error", "error", err)
			continue
		}

		go func(c net.Conn) {
			defer c.Close()
			scanner := bufio.NewScanner(c)
			for scanner.Scan() {
				var data MQTTData
				if err := json.Unmarshal(scanner.Bytes(), &data); err != nil {
					slog.Error("JSON parse error", "line", string(scanner.Bytes()), "error", err)
					continue
				}

				go func(d MQTTData) {
					token := client.Publish(d.Topic, 0, false, d.Payload)
					token.Wait()
					slog.Info("Published MQTT", "topic", d.Topic, "payload", d.Payload)
				}(data)
			}
			if err := scanner.Err(); err != nil {
				slog.Error("Socket scanner error", "error", err)
			}
		}(conn)
	}
}
