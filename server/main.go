package main

import (
	"time"

	"formula-logger/server/api"
	"formula-logger/server/buffer"
	"formula-logger/server/database"
	"formula-logger/server/model"
	"formula-logger/server/mqtt"
)

func main() {
	db := database.Setup()
	defer db.Close()

	database.InitAcc(db)
	database.InitWater(db)
	database.InitEnv(db)
	database.InitECU(db)

	bufAcc := buffer.NewBuf[model.Acc, model.AccDB](1000, 100)
	chAcc := make(chan model.Acc, 100)

	bufWater := buffer.NewBuf[model.Water, model.WaterDB](1000, 100)
	chWater := make(chan model.Water, 100)

	bufEnv := buffer.NewBuf[model.Env, model.EnvDB](1000, 100)
	chEnv := make(chan model.Env, 100)

	bufECU := buffer.NewBuf[model.ECU, model.ECUDB](1000, 100)
	chECU := make(chan model.ECU, 100)

	bufAF := buffer.NewBuf[model.AF, model.AFDB](1000, 100)
	chAF := make(chan model.AF, 100)

	client, disconnect := mqtt.Setup()
	defer disconnect()

	mqtt.AddHandler(client, "acc", chAcc)
	mqtt.AddHandler(client, "water", chWater)
	mqtt.AddHandler(client, "env", chEnv)
	mqtt.AddHandler(client, "ecu", chECU)
	mqtt.AddHandler(client, "af", chAF)

	e, subscribe := api.Setup()
	go subscribe()

	api.AddApi(e, "acc", bufAcc, db, database.GetAccTime, database.GetAcc)
	api.AddApi(e, "water", bufWater, db, database.GetWaterTime, database.GetWater)
	api.AddApi(e, "env", bufEnv, db, database.GetEnvTime, database.GetEnv)
	api.AddApi(e, "ecu", bufECU, db, database.GetECUTime, database.GetECU)
	api.AddApi(e, "af", bufAF, db, database.GetAFTime, database.GetAF)

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
		case data := <-chEnv:
			db := model.EnvDB{
				Env:  data,
				Time: time.Now().Unix(),
			}
			bufEnv.Add(data, db)
		case data := <-chECU:
			db := model.ECUDB{
				ECU:  data,
				Time: time.Now().Unix(),
			}
			bufECU.Add(data, db)
		case data := <-chAF:
			db := model.AFDB{
				AF:   data,
				Time: time.Now().Unix(),
			}
			bufAF.Add(data, db)
		case <-t.C:
			tx := db.MustBegin()

			if bufAcc.BufSize() > 0 {
				database.AddAcc(tx, bufAcc.FlashBuf())
			}

			if bufWater.BufSize() > 0 {
				database.AddWater(tx, bufWater.FlashBuf())
			}

			if bufEnv.BufSize() > 0 {
				database.AddEnv(tx, bufEnv.FlashBuf())
			}

			if bufECU.BufSize() > 0 {
				database.AddECU(tx, bufECU.FlashBuf())
			}

			if bufAF.BufSize() > 0 {
				database.AddAF(tx, bufAF.FlashBuf())
			}

			if err := tx.Commit(); err != nil {
				panic(err)
			}
		}
	}
}
