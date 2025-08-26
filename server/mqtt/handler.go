package mqtt

import (
	"encoding/json"
	"log/slog"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

func messageHandler[T any](logger *slog.Logger, ch chan T) mqtt.MessageHandler {
	return func(c mqtt.Client, m mqtt.Message) {
		var data T
		err := json.Unmarshal(m.Payload(), &data)
		if err != nil {
			logger.Error("failed to unmarshal", slog.String("error", err.Error()))
			return
		}
		ch <- data
		logger.Info("Received message", slog.String("topic", m.Topic()), slog.Any("data", data))
	}
}
