package main

import (
	"os"
	"path/filepath"
	"time"

	"github.com/jmoiron/sqlx"
	_ "github.com/mattn/go-sqlite3"

	api "formula-logger/server/api"
	"formula-logger/server/buffer"
	"formula-logger/server/database"
	"formula-logger/server/model"
	mqtt "formula-logger/server/mqtt"
)

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

	buf := buffer.NewBuf[model.Acc, model.AccDB](1000, 100)
	ch := make(chan model.Acc, 100)

	client, disconnect := mqtt.Setup()
	defer disconnect()

	mqtt.AddHandler(client, "acc", ch)

	e, subscribe := api.Setup()
	go subscribe()

	api.AddApi(e, "acc", buf, db, database.GetAccTime, database.GetAcc)

	t := time.NewTicker(1 * time.Second)
	defer t.Stop()

	for {
		select {
		case data := <-ch:
			db := model.AccDB{
				Acc:  data,
				Time: time.Now().Unix(),
			}
			buf.Add(data, db)
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
