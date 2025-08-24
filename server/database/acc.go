package database

import (
	"github.com/jmoiron/sqlx"

	"formula-logger/server/model"
)

const sql_init = `
	CREATE TABLE IF NOT EXISTS acc_data (
		id INTEGER PRIMARY KEY,
		time INTEGER,
		usec INTEGER,
		x REAL,
		y REAL,
		z REAL
	);
`

const sql_add = `
	INSERT INTO acc_data (time, usec, x, y, z)
	VALUES (:time, :usec, :x, :y, :z)
`

const sql_select_time = `
	SELECT time, usec
	FROM acc_data
	ORDER BY time ASC
`

const sql_select_data = `
	SELECT usec, x, y, z
	FROM acc_data
	WHERE time BETWEEN ? AND ?
	ORDER BY time ASC
`

func InitAcc(db *sqlx.DB) {
	db.MustExec(sql_init)
}

func AddAcc(tx *sqlx.Tx, data []model.AccDB) {
	_, err := tx.NamedExec(sql_add, data)
	if err != nil {
		tx.Rollback()
		panic(err)
	}
}

func GetAccTime(db *sqlx.DB) []model.Time {
	var result []model.Time

	err := db.Select(&result, sql_select_time)
	if err != nil {
		panic(err)
	}

	return result
}

func GetAcc(db *sqlx.DB, timeMin, timeMax int64) []model.Acc {
	var result []model.Acc

	err := db.Select(&result, sql_select_data, timeMin, timeMax)
	if err != nil {
		panic(err)
	}

	return result
}
