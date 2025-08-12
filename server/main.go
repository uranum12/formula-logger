package main

import (
	"database/sql"
	"fmt"
	"io"
	"log/slog"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/labstack/echo/v4"
	"github.com/labstack/echo/v4/middleware"
	_ "github.com/mattn/go-sqlite3"
	slogecho "github.com/samber/slog-echo"
)

func MessageHandler(logger *slog.Logger, db *sql.DB) func(c mqtt.Client, m mqtt.Message) {
	return func(c mqtt.Client, m mqtt.Message) {
		topic := m.Topic()
		payload := string(m.Payload())
		logger.Info("Recieve message", slog.String("topic", topic), slog.String("payload", payload))

		const sql_insert = `
			INSERT INTO mqtt_data (time, topic, payload)
			VALUES (?, ?, ?)
		`

		now := time.Now().Format(time.RFC3339Nano)

		_, err := db.Exec(sql_insert, now, topic, payload)
		if err != nil {
			panic(err)
		}
	}
}

func mqtt_server(logger *slog.Logger, db *sql.DB) {
	const broker = "localhost"
	const port = 1883
	const client_id = "mqtt-server"

	const topic = "+"
	const qos = 1

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

	token := client.Subscribe(topic, qos, MessageHandler(logger, db))
	if token.Wait() && token.Error() != nil {
		panic(token.Error())
	} else {
		logger.Info("Subscribed", "topic", topic)
	}

	for {
		time.Sleep(1 * time.Second)
	}
}

type Item struct {
	Time    string `json:"time"`
	Payload string `json:"payload"`
}

type Res struct {
	Data []Item `json:"data"`
}

func api_server(logger *slog.Logger, db *sql.DB) {
	e := echo.New()

	e.Use(slogecho.New(logger))
	e.Use(middleware.Recover())

	e.GET("/", func(c echo.Context) error {
		return c.File("dist/index.html")
	})

	e.GET("/:topic", func(c echo.Context) error {
		topic := c.Param("topic")
		limit_param := c.QueryParam("limit")

		const sql_select = `
			SELECT time, payload FROM mqtt_data
			WHERE topic = ?
		`

		const sql_select_limit = `
			SELECT time, payload FROM (
				SELECT time, payload FROM mqtt_data
				WHERE topic = ?
				ORDER BY time DESC
				LIMIT ?
			)
			ORDER BY time ASC
		`

		var data []Item

		if limit_param == "" {
			rows, err := db.Query(sql_select, topic)
			if err != nil {
				panic(err)
			}
			defer rows.Close()

			for rows.Next() {
				var item Item

				err = rows.Scan(&item.Time, &item.Payload)
				if err != nil {
					panic(err)
				}

				data = append(data, item)
			}

			return c.JSON(http.StatusOK, Res{Data: data})
		}

		limit, err := strconv.Atoi(limit_param)
		if err != nil {
			return c.JSON(http.StatusBadRequest, map[string]string{
				"error": "invalid limit parameter",
			})
		}

		rows, err := db.Query(sql_select_limit, topic, limit)
		if err != nil {
			panic(err)
		}
		defer rows.Close()

		for rows.Next() {
			var item Item

			err = rows.Scan(&item.Time, &item.Payload)
			if err != nil {
				panic(err)
			}

			data = append(data, item)
		}

		return c.JSON(http.StatusOK, Res{Data: data})
	})

	e.Logger.Fatal(e.Start(":1234"))
}

func main() {
	loc, err := time.LoadLocation("Asia/Tokyo")
	if err != nil {
		panic(err)
	}
	now := time.Now().In(loc)

	if err := os.Mkdir("log", 0755); err != nil && !os.IsExist(err) {
		panic(err)
	}
	log_path := filepath.Join("log", now.Format(time.RFC3339)+".log")

	f, err := os.Create(log_path)
	if err != nil {
		panic(err)
	}
	defer f.Close()

	multiWriter := io.MultiWriter(os.Stdout, f)

	logger := slog.New(slog.NewJSONHandler(multiWriter, nil))

	if err := os.Mkdir("data", 0755); err != nil && !os.IsExist(err) {
		panic(err)
	}
	db_path := filepath.Join("data", now.Format(time.RFC3339)+".db")

	db, err := sql.Open("sqlite3", db_path)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	const sql_create = `
		CREATE TABLE IF NOT EXISTS mqtt_data (
			id INTEGER PRIMARY KEY,
			time TEXT,
			topic TEXT,
			payload TEXT
		)
	`
	_, err = db.Exec(sql_create)
	if err != nil {
		panic(err)
	}

	go mqtt_server(logger, db)
	go api_server(logger, db)

	for {
		time.Sleep(1 * time.Second)
	}
}
