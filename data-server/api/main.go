package main

import (
	"fmt"
	"log/slog"
	"net/http"
	"os"
	"path/filepath"
	"sort"
	"strings"

	"github.com/labstack/echo/v4"
	"github.com/labstack/echo/v4/middleware"

	"formula-logger/data-server/config"
)

func main() {
	logger := slog.New(slog.NewTextHandler(os.Stdout, nil))
	slog.SetDefault(logger)

	cfg := config.LoadConfig("config.toml")

	e := echo.New()

	// ミドルウェア: ログ、リカバリ、gzip圧縮
	e.Use(middleware.Logger())
	e.Use(middleware.Recover())
	if cfg.API.GzipEnabled {
		e.Use(middleware.GzipWithConfig(middleware.GzipConfig{Level: cfg.API.GzipLevel}))
	}

	e.HideBanner = true

	// ハンドラー登録
	e.GET("/", listCSVHandler)
	e.GET("/:filename", serveCSVHandler)

	// ポート設定
	e.Logger.Fatal(e.Start(fmt.Sprintf(":%d", cfg.API.Port)))
}

// "/" の GET: CSVファイル名リストを返す
func listCSVHandler(c echo.Context) error {
	files := []string{}

	err := filepath.Walk("data", func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}
		if !info.IsDir() && strings.HasSuffix(info.Name(), ".csv") {
			name := strings.TrimSuffix(info.Name(), ".csv")
			files = append(files, name)
		}
		return nil
	})

	if err != nil {
		return c.JSON(http.StatusInternalServerError, map[string]string{"error": err.Error()})
	}

	sort.Strings(files)
	return c.JSON(http.StatusOK, map[string][]string{"files": files})
}

// "/:filename" の GET: CSVファイルをストリーム配信
func serveCSVHandler(c echo.Context) error {
	filename := c.Param("filename")

	// パスサニタイズ
	cleanFilename := filepath.Clean(filename)
	if strings.Contains(cleanFilename, "..") || cleanFilename == "" {
		return c.JSON(http.StatusBadRequest, map[string]string{"error": "invalid filename"})
	}

	filePath := filepath.Join("data", cleanFilename+".csv")

	// ファイル存在チェック
	if _, err := os.Stat(filePath); os.IsNotExist(err) {
		return c.JSON(http.StatusNotFound, map[string]string{"error": "file not found"})
	}

	// CSV をストリーム配信（gzipはミドルウェアが自動対応）
	c.Response().Header().Set(echo.HeaderContentType, "text/csv")
	return c.File(filePath)
}
