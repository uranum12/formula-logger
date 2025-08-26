package database

import (
	"os"
	"path/filepath"
	"time"

	"github.com/jmoiron/sqlx"
	_ "github.com/mattn/go-sqlite3"
)

func Setup() *sqlx.DB {
	loc, err := time.LoadLocation("Asia/Tokyo")
	if err != nil {
		panic(err)
	}
	now := time.Now().In(loc)

	if err := os.MkdirAll("data", 0755); err != nil {
		panic(err)
	}
	db_path := filepath.Join("data", now.Format("20060102_150405")+".db")

	db, err := sqlx.Open("sqlite3", db_path)
	if err != nil {
		panic(err)
	}

	return db
}
