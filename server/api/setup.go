package api

import (
	"fmt"

	"github.com/jmoiron/sqlx"
	"github.com/labstack/echo/v4"
	"github.com/labstack/echo/v4/middleware"

	"formula-logger/server/buffer"
	"formula-logger/server/model"
)

func Setup() (e *echo.Echo, subscribe func()) {
	const port = 1234

	e = echo.New()

	e.HideBanner = true

	e.Use(middleware.Logger())
	e.Use(middleware.Recover())

	e.GET("/", func(c echo.Context) error {
		return c.File("dist/index.html")
	})

	subscribe = func() {
		e.Logger.Fatal(e.Start(fmt.Sprintf(":%d", port)))
	}

	return
}

func AddApi[TData any, TDB any](
	e *echo.Echo,
	topic string,
	buf *buffer.Buf[TData, TDB],
	db *sqlx.DB,
	getTime func(*sqlx.DB) []model.Time,
	getHistory func(*sqlx.DB, int64, int64) []TData,
) {
	g := e.Group("/" + topic)
	g.GET("/recent", handlerRecent(buf))
	g.GET("/time", handlerTime(db, getTime))
	g.GET("/history", handlerHistory(db, getHistory))
}
