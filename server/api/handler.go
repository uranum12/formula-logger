package api

import (
	"fmt"
	"net/http"
	"strconv"
	"time"

	"github.com/jmoiron/sqlx"
	"github.com/labstack/echo/v4"

	"formula-logger/server/buffer"
	"formula-logger/server/model"
)

type ResData[T any] struct {
	Data []T `json:"data"`
}

type ResTime struct {
	Time []model.Time `json:"time"`
}

func jsonError(c echo.Context, msg string) error {
	return c.JSON(http.StatusBadRequest, map[string]string{
		"error": msg,
	})
}

func AddApi[TData any, TDB any](
	e *echo.Echo,
	topic string,
	buf *buffer.Buf[TData, TDB],
	db *sqlx.DB,
	getTime func(*sqlx.DB) []model.Time,
	getHistory func(*sqlx.DB, int64, int64) []TData,
) {
	e.GET(fmt.Sprintf("/%s/recent", topic), func(c echo.Context) error {
		listData := buf.GetRing()
		limitParam := c.QueryParam("limit")

		if limitParam != "" {
			limit, err := strconv.Atoi(limitParam)
			if err != nil {
				return jsonError(c, "invalid limit parameter")
			}

			lenListDB := len(listData)
			count := min(lenListDB, limit)
			start := lenListDB - count
			listData = listData[start:]
		}

		return c.JSON(http.StatusOK, ResData[TData]{Data: listData})
	})

	e.GET(fmt.Sprintf("/%s/time", topic), func(c echo.Context) error {
		listTime := getTime(db)
		return c.JSON(http.StatusOK, ResTime{Time: listTime})
	})

	e.GET(fmt.Sprintf("/%s/history", topic), func(c echo.Context) error {
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

		listData := getHistory(db, minTime.Unix(), maxTime.Unix())

		return c.JSON(http.StatusOK, ResData[TData]{Data: listData})
	})
}
