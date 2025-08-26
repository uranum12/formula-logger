package database

import (
	"github.com/jmoiron/sqlx"

	"formula-logger/server/model"
)

const sqlInitEnv = `
	CREATE TABLE IF NOT EXISTS env_data (
		id INTEGER PRIMARY KEY,
		time INTEGER,
		usec INTEGER,
		temp REAL,
		pres REAL,
		hum REAL
	);
`

const sqlAddEnv = `
	INSERT INTO env_data (time, usec, temp, pres, hum)
	VALUES (:time, :usec, :temp, :pres, :hum)
`

const sqlSelectTimeEnv = `
	SELECT time, usec
	FROM env_data
	ORDER BY time ASC
`

const sqlSelectDataEnv = `
	SELECT usec, temp, pres, hum
	FROM env_data
	WHERE time BETWEEN ? AND ?
	ORDER BY time ASC
`

func InitEnv(db *sqlx.DB) {
	initTable(db, sqlInitEnv)
}

func AddEnv(tx *sqlx.Tx, data []model.EnvDB) {
	addData(tx, sqlAddEnv, data)
}

func GetEnvTime(db *sqlx.DB) []model.Time {
	return getTime(db, sqlSelectTimeEnv)
}

func GetEnv(db *sqlx.DB, timeMin, timeMax int64) []model.Env {
	return getData[model.Env](db, sqlSelectDataEnv, timeMin, timeMax)
}
