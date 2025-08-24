package handler

import (
	"net/http"
	"strconv"
	"time"

	"github.com/jmoiron/sqlx"
	"github.com/labstack/echo/v4"

	"formula-logger/server/buffer"
	"formula-logger/server/database"
	"formula-logger/server/model"
)

type resData struct {
	Data []model.Acc `json:"data"`
}

type resTime struct {
	Time []model.Time `json:"time"`
}

func SetupAcc(e *echo.Echo, buf *buffer.Buf[model.AccDB], db *sqlx.DB) {
	e.GET("/acc/recent", func(c echo.Context) error {
		listDB := buf.GetRing()
		var listData []model.Acc

		limitParam := c.QueryParam("limit")

		if limitParam == "" {
			listData = make([]model.Acc, len(listDB))
			for i, v := range listDB {
				listData[i] = v.Acc
			}
		} else {
			limit, err := strconv.Atoi(limitParam)
			if err != nil {
				return c.JSON(http.StatusBadRequest, map[string]string{
					"error": "invalid limit parameter",
				})
			}

			listData = make([]model.Acc, min(limit, len(listDB)))
			for i, v := range listDB[:limit] {
				listData[i] = v.Acc
			}
		}

		return c.JSON(http.StatusOK, resData{Data: listData})

	})

	e.GET("/acc/time", func(c echo.Context) error {
		listTime := database.GetAccTime(db)
		return c.JSON(http.StatusOK, resTime{Time: listTime})
	})

	e.GET("/acc/history", func(c echo.Context) error {
		minParam := c.QueryParam("min")
		maxParam := c.QueryParam("max")

		if minParam == "" || maxParam == "" {
			return c.JSON(http.StatusBadRequest, map[string]string{
				"error": "min and max are required",
			})
		}

		minTime, err := time.Parse(time.RFC3339, minParam)
		if err != nil {
			return c.JSON(http.StatusBadRequest, map[string]string{
				"error": "invalid min format",
			})
		}

		maxTime, err := time.Parse(time.RFC3339, maxParam)
		if err != nil {
			return c.JSON(http.StatusBadRequest, map[string]string{
				"error": "invalid max format",
			})
		}

		listData := database.GetAcc(db, minTime.Unix(), maxTime.Unix())

		return c.JSON(http.StatusOK, resData{Data: listData})
	})
}
