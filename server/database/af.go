package database

import (
	"github.com/jmoiron/sqlx"

	"formula-logger/server/model"
)

const sqlInitAF = `
	CREATE TABLE IF NOT EXISTS af_data (
		id INTEGER PRIMARY KEY,
		time INTEGER,
		usec INTEGER,
		af REAL
	);
`

const sqlAddAF = `
	INSERT INTO af_data (time, usec, af)
	VALUES (:time, :usec, :af)
`

const sqlSelectTimeAF = `
	SELECT time, usec
	FROM af_data
	ORDER BY time ASC
`

const sqlSelectDataAF = `
	SELECT usec, af
	FROM af_data
	WHERE time BETWEEN ? AND ?
	ORDER BY time ASC
`

func InitAF(db *sqlx.DB) {
	initTable(db, sqlInitAF)
}

func AddAF(tx *sqlx.Tx, data []model.AFDB) {
	addData(tx, sqlAddAF, data)
}

func GetAFTime(db *sqlx.DB) []model.Time {
	return getTime(db, sqlSelectTimeAF)
}

func GetAF(db *sqlx.DB, timeMin, timeMax int64) []model.AF {
	return getData[model.AF](db, sqlSelectDataAF, timeMin, timeMax)
}
