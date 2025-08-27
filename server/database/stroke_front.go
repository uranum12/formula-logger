package database

import (
	"github.com/jmoiron/sqlx"

	"formula-logger/server/model"
)

const sqlInitStrokeFront = `
	CREATE TABLE IF NOT EXISTS stroke_front_data (
		id INTEGER PRIMARY KEY,
		time INTEGER,
		usec INTEGER,
		left REAL,
		right REAL
	);
`

const sqlAddStrokeFront = `
	INSERT INTO stroke_front_data (time, usec, left, right)
	VALUES (:time, :usec, :left, :right)
`

const sqlSelectTimeStrokeFront = `
	SELECT time, usec
	FROM stroke_front_data
	ORDER BY time ASC
`

const sqlSelectDataStrokeFront = `
	SELECT usec, left, right
	FROM stroke_front_data
	WHERE time BETWEEN ? AND ?
	ORDER BY time ASC
`

func InitStrokeFront(db *sqlx.DB) {
	initTable(db, sqlInitStrokeFront)
}

func AddStrokeFront(tx *sqlx.Tx, data []model.StrokeFrontDB) {
	addData(tx, sqlAddStrokeFront, data)
}

func GetStrokeFrontTime(db *sqlx.DB) []model.Time {
	return getTime(db, sqlSelectTimeStrokeFront)
}

func GetStrokeFront(db *sqlx.DB, timeMin, timeMax int64) []model.StrokeFront {
	return getData[model.StrokeFront](db, sqlSelectDataStrokeFront, timeMin, timeMax)
}
