package database

import (
	"github.com/jmoiron/sqlx"

	"formula-logger/server/model"
)

const sqlInitWater = `
	CREATE TABLE IF NOT EXISTS water_data (
		id INTEGER PRIMARY KEY,
		time INTEGER,
		usec INTEGER,
		inlet_temp REAL,
		outlet_temp REAL
	);
`

const sqlAddWater = `
	INSERT INTO water_data (time, usec, inlet_temp, outlet_temp)
	VALUES (:time, :usec, :inlet_temp, :outlet_temp)
`

const sqlSelectTimeWater = `
	SELECT time, usec
	FROM water_data
	ORDER BY time ASC
`

const sqlSelectDataWater = `
	SELECT usec, inlet_temp, outlet_temp
	FROM water_data
	WHERE time BETWEEN ? AND ?
	ORDER BY time ASC
`

func InitWater(db *sqlx.DB) {
	db.MustExec(sqlInitWater)
}

func AddWater(tx *sqlx.Tx, data []model.WaterDB) {
	_, err := tx.NamedExec(sqlAddWater, data)
	if err != nil {
		tx.Rollback()
		panic(err)
	}
}

func GetWaterTime(db *sqlx.DB) []model.Time {
	var result []model.Time

	err := db.Select(&result, sqlSelectTimeWater)
	if err != nil {
		panic(err)
	}

	return result
}

func GetWater(db *sqlx.DB, timeMin, timeMax int64) []model.Water {
	var result []model.Water

	err := db.Select(&result, sqlSelectDataWater, timeMin, timeMax)
	if err != nil {
		panic(err)
	}

	return result
}
