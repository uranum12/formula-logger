package database

import (
	"github.com/jmoiron/sqlx"

	"formula-logger/server/model"
)

func initTable(db *sqlx.DB, sqlInit string) {
	db.MustExec(sqlInit)
}

func addData[T any](tx *sqlx.Tx, sqlAdd string, data []T) {
	if _, err := tx.NamedExec(sqlAdd, data); err != nil {
		tx.Rollback()
		panic(err)
	}
}

func getTime(db *sqlx.DB, sqlSelectTime string) []model.Time {
	var result []model.Time
	if err := db.Select(&result, sqlSelectTime); err != nil {
		panic(err)
	}
	return result
}

func getData[T any](db *sqlx.DB, sqlSelectData string, timeMin, timeMax int64) []T {
	var result []T
	if err := db.Select(&result, sqlSelectData, timeMin, timeMax); err != nil {
		panic(err)
	}
	return result
}
