package api

import (
	"fmt"

	"github.com/labstack/echo/v4"
	"github.com/labstack/echo/v4/middleware"
)

func Setup() (e *echo.Echo, subscribe func()) {
	const port = 1234

	e = echo.New()

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
