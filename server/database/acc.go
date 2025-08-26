package database

import (
	"github.com/jmoiron/sqlx"

	"formula-logger/server/model"
)

const sqlInitAcc = `
	CREATE TABLE IF NOT EXISTS acc_data (
		id INTEGER PRIMARY KEY,
		time INTEGER,
		usec INTEGER,
		x REAL,
		y REAL,
		z REAL
	);
`

const sqlAddAcc = `
	INSERT INTO acc_data (time, usec, x, y, z)
	VALUES (:time, :usec, :x, :y, :z)
`

const sqlSelectTimeAcc = `
	SELECT time, usec
	FROM acc_data
	ORDER BY time ASC
`

const sqlSelectDataAcc = `
	SELECT usec, x, y, z
	FROM acc_data
	WHERE time BETWEEN ? AND ?
	ORDER BY time ASC
`

func InitAcc(db *sqlx.DB) {
	initTable(db, sqlInitAcc)
}

func AddAcc(tx *sqlx.Tx, data []model.AccDB) {
	addData(tx, sqlAddAcc, data)
}

func GetAccTime(db *sqlx.DB) []model.Time {
	return getTime(db, sqlSelectTimeAcc)
}

func GetAcc(db *sqlx.DB, timeMin, timeMax int64) []model.Acc {
	return getData[model.Acc](db, sqlSelectDataAcc, timeMin, timeMax)
}
