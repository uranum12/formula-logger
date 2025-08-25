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
	database.InitWater(db)

	bufAcc := buffer.NewBuf[model.Acc, model.AccDB](1000, 100)
	chAcc := make(chan model.Acc, 100)

	bufWater := buffer.NewBuf[model.Water, model.WaterDB](1000, 100)
	chWater := make(chan model.Water, 100)

	client, disconnect := mqtt.Setup()
	defer disconnect()

	mqtt.AddHandler(client, "acc", chAcc)
	mqtt.AddHandler(client, "water", chWater)

	e, subscribe := api.Setup()
	go subscribe()

	api.AddApi(e, "acc", bufAcc, db, database.GetAccTime, database.GetAcc)
	api.AddApi(e, "water", bufWater, db, database.GetWaterTime, database.GetWater)

	t := time.NewTicker(1 * time.Second)
	defer t.Stop()

	for {
		select {
		case data := <-chAcc:
			db := model.AccDB{
				Acc:  data,
				Time: time.Now().Unix(),
			}
			bufAcc.Add(data, db)
		case data := <-chWater:
			db := model.WaterDB{
				Water: data,
				Time:  time.Now().Unix(),
			}
			bufWater.Add(data, db)
		case <-t.C:
			tx := db.MustBegin()

			if bufAcc.GetBufSize() > 0 {
				database.AddAcc(tx, bufAcc.PopBuf())
			}

			if bufWater.GetBufSize() > 0 {
				database.AddWater(tx, bufWater.PopBuf())
			}

			if err := tx.Commit(); err != nil {
				panic(err)
			}
		}
	}
}
