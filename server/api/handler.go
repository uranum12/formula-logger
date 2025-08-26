package api

import (
	"net/http"
	"strconv"
	"time"

	"github.com/jmoiron/sqlx"
	"github.com/labstack/echo/v4"

	"formula-logger/server/buffer"
	"formula-logger/server/model"
)

type resData[T any] struct {
	Data []T `json:"data"`
}

type resTime struct {
	Time []model.Time `json:"time"`
}

func jsonError(c echo.Context, msg string) error {
	return c.JSON(http.StatusBadRequest, map[string]string{
		"error": msg,
	})
}

func handlerRecent[TData any, TDB any](buf *buffer.Buf[TData, TDB]) echo.HandlerFunc {
	return func(c echo.Context) error {
		listData := buf.RingSnapshot()

		if limitParam := c.QueryParam("limit"); limitParam != "" {
			limit, err := strconv.Atoi(limitParam)
			if err != nil {
				return jsonError(c, "invalid limit parameter")
			}
			if limit < len(listData) {
				listData = listData[len(listData)-limit:]
			}
		}

		return c.JSON(http.StatusOK, resData[TData]{Data: listData})
	}
}

func handlerTime(db *sqlx.DB, getTime func(*sqlx.DB) []model.Time) echo.HandlerFunc {
	return func(c echo.Context) error {
		return c.JSON(http.StatusOK, resTime{Time: getTime(db)})
	}
}

func handlerHistory[TData any](db *sqlx.DB,
	getHistory func(*sqlx.DB, int64, int64) []TData) echo.HandlerFunc {
	return func(c echo.Context) error {
		minParam := c.QueryParam("min")
		maxParam := c.QueryParam("max")

		if minParam == "" || maxParam == "" {
			return jsonError(c, "min and max are required")
		}

		minTime, err := time.Parse(time.RFC3339, minParam)
		if err != nil {
			return jsonError(c, "invalid min format")
		}

		maxTime, err := time.Parse(time.RFC3339, maxParam)
		if err != nil {
			return jsonError(c, "invalid max format")
		}

		data := getHistory(db, minTime.Unix(), maxTime.Unix())
		return c.JSON(http.StatusOK, resData[TData]{Data: data})
	}
}
