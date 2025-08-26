package database

import (
	"github.com/jmoiron/sqlx"

	"formula-logger/server/model"
)

const sqlInitECU = `
	CREATE TABLE IF NOT EXISTS ecu_data (
		id INTEGER PRIMARY KEY,
		time INTEGER,
		usec INTEGER,
		ect REAL,
		tps REAL,
		iap REAL,
		gp INTEGER,
		rpm INTEGER
	);
`

const sqlAddECU = `
	INSERT INTO ecu_data (time, usec, ect, tps, iap, gp, rpm)
	VALUES (:time, :usec, :ect, :tps, :iap, :gp, :rpm)
`

const sqlSelectTimeECU = `
	SELECT time, usec
	FROM ecu_data
	ORDER BY time ASC
`

const sqlSelectDataECU = `
	SELECT usec, ect, tps, iap, gp, rpm
	FROM ecu_data
	WHERE time BETWEEN ? AND ?
	ORDER BY time ASC
`

func InitECU(db *sqlx.DB) {
	initTable(db, sqlInitECU)
}

func AddECU(tx *sqlx.Tx, data []model.ECUDB) {
	addData(tx, sqlAddECU, data)
}

func GetECUTime(db *sqlx.DB) []model.Time {
	return getTime(db, sqlSelectTimeECU)
}

func GetECU(db *sqlx.DB, timeMin, timeMax int64) []model.ECU {
	return getData[model.ECU](db, sqlSelectDataECU, timeMin, timeMax)
}
