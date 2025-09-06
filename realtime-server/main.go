package main

import (
	"encoding/json"
	"log"
	"net/http"
	"strconv"
	"strings"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/jmoiron/sqlx"
	"github.com/labstack/echo/v4"
	_ "github.com/mattn/go-sqlite3"

	"formula-logger/realtime-server/models"
)

// ---------------- データ構造 ----------------

type FieldValue struct {
	Usec  int64   `db:"usec" json:"usec"`
	Value float64 `db:"value" json:"value"`
}

type FieldInfo struct {
	Table  string
	Column string
}

// ---------------- MQTT購読関数 ----------------
func subscribeMQTT[TData models.TimeProvider, TDB any](
	client mqtt.Client,
	db *sqlx.DB,
	topic string,
	mapData func(TData, models.DBTime) TDB,
	sqlInsert string,
) {
	client.Subscribe(topic, 0, func(c mqtt.Client, m mqtt.Message) {
		var data TData
		if err := json.Unmarshal(m.Payload(), &data); err != nil {
			log.Println(topic, "JSON parse error:", err)
			return
		}

		dbt := models.DBTime{
			Usec: data.GetUsec(),
			Time: time.Now().Unix(),
		}

		row := mapData(data, dbt)

		if _, err := db.NamedExec(sqlInsert, row); err != nil {
			log.Println(topic, "DB insert error:", err)
		}
	})
}

// ---------------- メイン ----------------
func main() {
	db, err := sqlx.Open("sqlite3", ":memory:")
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()

	// ---------------- テーブル作成 ----------------
	db.MustExec(`CREATE TABLE water (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		usec INTEGER,
		time INTEGER,
		inlet_temp REAL,
		outlet_temp REAL
	)`)
	db.MustExec(`CREATE TABLE stroke_front (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		usec INTEGER,
		time INTEGER,
		left REAL,
		right REAL
	)`)

	// ---------------- MQTT設定 ----------------
	opts := mqtt.NewClientOptions().AddBroker("tcp://localhost:1883").SetClientID("go_mqtt_client")
	client := mqtt.NewClient(opts)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		log.Fatal(token.Error())
	}

	// ---------------- MQTT購読 ----------------
	subscribeMQTT(
		client,
		db,
		"water",
		models.MapWaterData,
		`INSERT INTO water (usec, time, inlet_temp, outlet_temp)
		 VALUES (:usec, :time, :inlet_temp, :outlet_temp)`,
	)
	subscribeMQTT(
		client,
		db,
		"stroke/front",
		models.MapStrokeFrontData,
		`INSERT INTO stroke_front (usec, time, left, right)
		VALUES (:usec, :time, :left, :right)`,
	)

	// ---------------- フィールドマップ ----------------
	fieldMap := map[string]FieldInfo{
		"water/inlet_temp":   {"water", "inlet_temp"},
		"water/outlet_temp":  {"water", "outlet_temp"},
		"stroke/front/left":  {"stroke_front", "left"},
		"stroke/front/right": {"stroke_front", "right"},
	}

	e := echo.New()

	// ---------------- 最新N件取得 ----------------
	getFieldsLatest := func(fields []string, n int) map[string][]FieldValue {
		result := make(map[string][]FieldValue)
		for _, f := range fields {
			info, ok := fieldMap[f]
			if !ok {
				continue
			}
			query := `
				SELECT usec, ` + info.Column + ` AS value
				FROM (
					SELECT usec, ` + info.Column + `
					FROM ` + info.Table + `
					ORDER BY usec DESC
					LIMIT ?
				) sub
				ORDER BY usec ASC
			`
			var arr []FieldValue
			if err := db.Select(&arr, query, n); err != nil {
				log.Println("query error for", f, ":", err)
				continue
			}
			result[f] = arr
		}
		return result
	}

	// ---------------- 期間指定取得 ----------------
	getFieldsRange := func(fields []string, from, to int64) map[string][]FieldValue {
		result := make(map[string][]FieldValue)
		for _, f := range fields {
			info, ok := fieldMap[f]
			if !ok {
				continue
			}
			query := `
				SELECT usec, ` + info.Column + ` AS value
				FROM ` + info.Table + `
				WHERE time BETWEEN ? AND ?
				ORDER BY usec ASC
			`
			var arr []FieldValue
			if err := db.Select(&arr, query, from, to); err != nil {
				log.Println("query error for", f, ":", err)
				continue
			}
			result[f] = arr
		}
		return result
	}

	// ---------------- 最新N件 API ----------------
	e.GET("/fields/latest/:n", func(c echo.Context) error {
		n, err := strconv.Atoi(c.Param("n"))
		if err != nil || n <= 0 {
			return c.JSON(http.StatusBadRequest, map[string]string{"error": "invalid n"})
		}
		fieldsParam := c.QueryParam("fields")
		if fieldsParam == "" {
			return c.JSON(http.StatusBadRequest, map[string]string{"error": "fields param required"})
		}
		requested := strings.Split(fieldsParam, ",")
		res := getFieldsLatest(requested, n)
		return c.JSON(http.StatusOK, res)
	})

	// ---------------- 期間指定 API ----------------
	e.GET("/fields/range", func(c echo.Context) error {
		fromStr := c.QueryParam("from")
		toStr := c.QueryParam("to")
		if fromStr == "" || toStr == "" {
			return c.JSON(http.StatusBadRequest, map[string]string{"error": "from and to required"})
		}
		from, err1 := strconv.ParseInt(fromStr, 10, 64)
		to, err2 := strconv.ParseInt(toStr, 10, 64)
		if err1 != nil || err2 != nil {
			return c.JSON(http.StatusBadRequest, map[string]string{"error": "invalid from/to"})
		}
		fieldsParam := c.QueryParam("fields")
		if fieldsParam == "" {
			return c.JSON(http.StatusBadRequest, map[string]string{"error": "fields param required"})
		}
		requested := strings.Split(fieldsParam, ",")
		res := getFieldsRange(requested, from, to)
		return c.JSON(http.StatusOK, res)
	})

	go func() {
		if err := e.Start(":8080"); err != nil {
			log.Fatal(err)
		}
	}()

	select {}
}
