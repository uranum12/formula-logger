package handler

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

func SetupAPI[T any, DB any](
	e *echo.Echo,
	topic string,
	buf *buffer.Buf[DB],
	db *sqlx.DB,
	getTime func(*sqlx.DB) []model.Time,
	getHistory func(*sqlx.DB, int64, int64) []T,
	extractDB func(DB) T,
) {
	e.GET(fmt.Sprintf("/%s/recent", topic), func(c echo.Context) error {
		listDB := buf.GetRing()
		limitParam := c.QueryParam("limit")

		var listData []T
		if limitParam == "" {
			listData = make([]T, len(listDB))
			for i, v := range listDB {
				listData[i] = extractDB(v)
			}
		} else {
			limit, err := strconv.Atoi(limitParam)
			if err != nil {
				return jsonError(c, "invalid limit parameter")
			}

			lenListDB := len(listDB)
			count := min(lenListDB, limit)
			start := lenListDB - count
			listData = make([]T, count)
			for i, v := range listDB[start:] {
				listData[i] = extractDB(v)
			}
		}

		return c.JSON(http.StatusOK, ResData[T]{Data: listData})
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

		return c.JSON(http.StatusOK, ResData[T]{Data: listData})
	})
}
