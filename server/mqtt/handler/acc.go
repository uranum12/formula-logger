package handler

import (
	"encoding/json"
	"log/slog"

	mqtt "github.com/eclipse/paho.mqtt.golang"

	"formula-logger/server/model"
)

func HandlerAcc(logger *slog.Logger, ch chan model.Acc) mqtt.MessageHandler {
	return func(c mqtt.Client, m mqtt.Message) {
		var data model.Acc
		err := json.Unmarshal(m.Payload(), &data)
		if err != nil {
			logger.Error("failed to unmarshal", slog.String("error", err.Error()))
			return
		}
		ch <- data
	}
}
