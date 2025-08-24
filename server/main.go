package main

import (
	"fmt"
	"log/slog"
	"os"
	"path/filepath"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/jmoiron/sqlx"
	"github.com/labstack/echo/v4"
	"github.com/labstack/echo/v4/middleware"
	_ "github.com/mattn/go-sqlite3"

	apiHandler "formula-logger/server/api/handler"
	"formula-logger/server/buffer"
	"formula-logger/server/database"
	"formula-logger/server/model"
	mqttHandler "formula-logger/server/mqtt/handler"
)

func mqtt_server(ch chan model.AccDB) {
	const broker = "localhost"
	const port = 1883
	const client_id = "mqtt-server"

	const topic = "acc"
	const qos = 1

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
	defer client.Disconnect(250)

	token := client.Subscribe(topic, qos, mqttHandler.HandlerAcc(logger, ch))
	if token.Wait() && token.Error() != nil {
		panic(token.Error())
	} else {
		logger.Info("Subscribed", "topic", topic)
	}

	select {}
}

func api_server(db *sqlx.DB, buf *buffer.Buf[model.AccDB]) {
	e := echo.New()

	e.Use(middleware.Logger())
	e.Use(middleware.Recover())

	e.GET("/", func(c echo.Context) error {
		return c.File("dist/index.html")
	})

	apiHandler.SetupAcc(e, buf, db)

	e.Logger.Fatal(e.Start(":1234"))
}

func main() {
	loc, err := time.LoadLocation("Asia/Tokyo")
	if err != nil {
		panic(err)
	}
	now := time.Now().In(loc)

	if err := os.Mkdir("data", 0755); err != nil && !os.IsExist(err) {
		panic(err)
	}
	db_path := filepath.Join("data", now.Format(time.RFC3339)+".db")

	db, err := sqlx.Open("sqlite3", db_path)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	database.InitAcc(db)

	buf := buffer.NewBuf[model.AccDB](1000, 100)
	ch := make(chan model.AccDB, 100)

	go mqtt_server(ch)
	go api_server(db, buf)

	t := time.NewTicker(1 * time.Second)
	defer t.Stop()

	for {
		select {
		case d := <-ch:
			buf.Add(d)
		case <-t.C:
			tx := db.MustBegin()

			if buf.GetBufSize() > 0 {
				database.AddAcc(tx, buf.PopBuf())
			}

			if err := tx.Commit(); err != nil {
				panic(err)
			}
		}
	}
}
