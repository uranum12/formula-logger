package mqtt

import (
	"fmt"
	"log/slog"
	"os"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

type Client struct {
	client mqtt.Client
	logger *slog.Logger
}

func Setup() (c *Client, disconnect func()) {
	const broker = "localhost"
	const port = 1883
	const client_id = "mqtt-server"

	logger := slog.New(slog.NewJSONHandler(os.Stdout, nil))

	opts := mqtt.NewClientOptions()
	opts.AddBroker(fmt.Sprintf("tcp://%s:%d", broker, port))
	opts.SetClientID(client_id)
	client := mqtt.NewClient(opts)

	if token := client.Connect(); token.Wait() && token.Error() != nil {
		panic(token.Error())
	} else {
		logger.Info("Connected to MQTT")
	}

	c = &Client{
		client: client,
		logger: logger,
	}

	disconnect = func() {
		client.Disconnect(250)
	}

	return
}

func AddHandler[T any](c *Client, topic string, ch chan T) {
	const qos = 1

	token := c.client.Subscribe(topic, qos, messageHandler(c.logger, ch))
	if token.Wait() && token.Error() != nil {
		panic(token.Error())
	}
	c.logger.Info("Subscribed", "topic", topic)
}
