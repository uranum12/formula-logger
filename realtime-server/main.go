package main

import (
	"database/sql"
	"encoding/json"
	"log"
	"net/http"
	"strconv"
	"strings"
	"time"

	"github.com/doug-martin/goqu/v9"
	_ "github.com/doug-martin/goqu/v9/dialect/sqlite3"
	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/labstack/echo/v4"
	"github.com/labstack/echo/v4/middleware"
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
	db *goqu.Database,
	topic string,
	mapData func(TData, models.DBTime) TDB,
	tableName string,
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
		ds := db.Insert(tableName).Rows(row)
		if _, err := ds.Executor().Exec(); err != nil {
			log.Println(topic, "DB insert error:", err)
		}
	})
}

// ---------------- メイン ----------------
func main() {
	sqlDB, err := sql.Open("sqlite3", ":memory:")
	if err != nil {
		log.Fatal(err)
	}
	defer sqlDB.Close()

	db := goqu.New("sqlite3", sqlDB)

	// ---------------- テーブル作成 ----------------
	db.Exec(`CREATE TABLE water (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		usec INTEGER,
		time INTEGER,
		inlet_temp REAL,
		outlet_temp REAL
	)`)
	db.Exec(`CREATE TABLE stroke_front (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		usec INTEGER,
		time INTEGER,
		left REAL,
		right REAL
	)`)
	db.Exec(`CREATE TABLE stroke_rear (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		usec INTEGER,
		time INTEGER,
		left REAL,
		right REAL
	)`)
	db.Exec(`CREATE TABLE ecu (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		usec INTEGER,
		time INTEGER,
		ect REAL,
		tps REAL,
		iap REAL,
		gp INTEGER
	)`)
	db.Exec(`CREATE TABLE rpm (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		usec INTEGER,
		time INTEGER,
		rpm REAL,
		god REAL
	)`)
	db.Exec(`CREATE TABLE acc (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		usec INTEGER,
		time INTEGER,
		accel_x REAL,
		accel_y REAL,
		accel_z REAL,
		gyro_x REAL,
		gyro_y REAL,
		gyro_z REAL,
		mag_x REAL,
		mag_y REAL,
		mag_z REAL,
		euler_heading REAL,
		euler_roll REAL,
		euler_pitch REAL,
		quaternion_w REAL,
		quaternion_x REAL,
		quaternion_y REAL,
		quaternion_z REAL,
		linear_accel_x REAL,
		linear_accel_y REAL,
		linear_accel_z REAL,
		gravity_x REAL,
		gravity_y REAL,
		gravity_z REAL,
		status_sys INTEGER,
		status_gyro INTEGER,
		status_accel INTEGER,
		status_mag INTEGER
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
		"water",
	)
	subscribeMQTT(
		client,
		db,
		"stroke/front",
		models.MapStrokeFrontData,
		"stroke_front",
	)
	subscribeMQTT(
		client,
		db,
		"stroke/rear",
		models.MapStrokeRearData,
		"stroke_rear",
	)
	subscribeMQTT(
		client,
		db,
		"ecu",
		models.MapECUData,
		"ecu",
	)
	subscribeMQTT(
		client,
		db,
		"rpm",
		models.MapRPMData,
		"rpm",
	)
	subscribeMQTT(
		client,
		db,
		"acc",
		models.MapAccData,
		"acc",
	)

	// ---------------- フィールドマップ ----------------
	fieldMap := map[string]FieldInfo{
		"water/inlet_temp":   {"water", "inlet_temp"},
		"water/outlet_temp":  {"water", "outlet_temp"},
		"stroke/front/left":  {"stroke_front", "left"},
		"stroke/front/right": {"stroke_front", "right"},
		"stroke/rear/left":   {"stroke_rear", "left"},
		"stroke/rear/right":  {"stroke_rear", "right"},
		"ecu/ect":            {"ecu", "ect"},
		"ecu/tps":            {"ecu", "tps"},
		"ecu/iap":            {"ecu", "iap"},
		"ecu/gp":             {"ecu", "gp"},
		"rpm/rpm":            {"rpm", "rpm"},
		"rpm/god":            {"rpm", "god"},
		"acc/accel_x":        {"acc", "accel_x"},
		"acc/accel_y":        {"acc", "accel_y"},
		"acc/accel_z":        {"acc", "accel_z"},
		"acc/gyro_x":         {"acc", "gyro_x"},
		"acc/gyro_y":         {"acc", "gyro_y"},
		"acc/gyro_z":         {"acc", "gyro_z"},
		"acc/mag_x":          {"acc", "mag_x"},
		"acc/mag_y":          {"acc", "mag_y"},
		"acc/mag_z":          {"acc", "mag_z"},
		"acc/euler_heading":  {"acc", "euler_heading"},
		"acc/euler_roll":     {"acc", "euler_roll"},
		"acc/euler_pitch":    {"acc", "euler_pitch"},
		"acc/quaternion_w":   {"acc", "quaternion_w"},
		"acc/quaternion_x":   {"acc", "quaternion_x"},
		"acc/quaternion_y":   {"acc", "quaternion_y"},
		"acc/quaternion_z":   {"acc", "quaternion_z"},
		"acc/linear_accel_x": {"acc", "linear_accel_x"},
		"acc/linear_accel_y": {"acc", "linear_accel_y"},
		"acc/linear_accel_z": {"acc", "linear_accel_z"},
		"acc/gravity_x":      {"acc", "gravity_x"},
		"acc/gravity_y":      {"acc", "gravity_y"},
		"acc/gravity_z":      {"acc", "gravity_z"},
		"acc/status_sys":     {"acc", "status_sys"},
		"acc/status_gyro":    {"acc", "status_gyro"},
		"acc/status_accel":   {"acc", "status_accel"},
		"acc/status_mag":     {"acc", "status_mag"},
	}

	e := echo.New()

	e.Use(middleware.Recover())
	e.Use(middleware.Logger())

	// ---------------- 最新N件取得 ----------------
	getFieldsLatest := func(fields []string, n int) map[string][]FieldValue {
		result := make(map[string][]FieldValue)
		for _, f := range fields {
			info, ok := fieldMap[f]
			if !ok {
				continue
			}
			sub := db.From(goqu.I(info.Table)).
				Select(
					goqu.C("usec"),
					goqu.C(info.Column),
				).
				Order(goqu.C("usec").Desc()).
				Limit(uint(n))
			ds := db.From(sub.As("sub")).
				Select(
					goqu.C("usec"),
					goqu.C(info.Column).As("value"),
				).
				Order(goqu.C("usec").Asc())
			var arr []FieldValue
			if err := ds.ScanStructs(&arr); err != nil {
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
			ds := db.From(info.Table).
				Select(
					goqu.C("usec"),
					goqu.C(info.Column).As("value"),
				).
				Where(goqu.C("time").Between(goqu.Range(from, to))).
				Order(goqu.C("usec").Asc())
			var arr []FieldValue
			if err := ds.ScanStructs(&arr); err != nil {
				log.Println("query error for", f, ":", err)
				continue
			}
			result[f] = arr
		}
		return result
	}

	e.GET("/", func(c echo.Context) error {
		return c.File("dist/index.html")
	})

	// ---------------- 最新N件 API ----------------
	e.GET("/latest/:n", func(c echo.Context) error {
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
	e.GET("/range", func(c echo.Context) error {
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
