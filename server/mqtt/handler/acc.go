package handler

import (
	"encoding/json"
	"log/slog"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"

	"formula-logger/server/model"
)

func HandlerAcc(logger *slog.Logger, ch chan model.AccDB) mqtt.MessageHandler {
	return func(c mqtt.Client, m mqtt.Message) {
		var jsonData model.Acc
		err := json.Unmarshal(m.Payload(), &jsonData)
		if err != nil {
			logger.Error("failed to unmarshal", slog.String("error", err.Error()))
			return
		}

		now := time.Now().UTC().Unix()
		data := model.AccDB{
			Acc:  jsonData,
			Time: now,
		}

		ch <- data
	}
}
