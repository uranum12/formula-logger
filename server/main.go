package main

import (
	"time"

	api "formula-logger/server/api"
	"formula-logger/server/buffer"
	"formula-logger/server/database"
	"formula-logger/server/model"
	mqtt "formula-logger/server/mqtt"
)

func main() {
	db := database.Setup()
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
