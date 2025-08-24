package handler

import (
	"github.com/jmoiron/sqlx"
	"github.com/labstack/echo/v4"

	"formula-logger/server/buffer"
	"formula-logger/server/database"
	"formula-logger/server/model"
)

func SetupAcc(e *echo.Echo, buf *buffer.Buf[model.AccDB], db *sqlx.DB) {
	extractData := func(db model.AccDB) model.Acc {
		return db.Acc
	}
	SetupAPI(e, "acc", buf, db, database.GetAccTime, database.GetAcc, extractData)
}
