package handler

import (
	"github.com/jmoiron/sqlx"
	"github.com/labstack/echo/v4"

	"formula-logger/server/buffer"
	"formula-logger/server/database"
	"formula-logger/server/model"
)

func SetupAcc(e *echo.Echo, buf *buffer.Buf[model.Acc, model.AccDB], db *sqlx.DB) {
	SetupAPI(e, "acc", buf, db, database.GetAccTime, database.GetAcc)
}
