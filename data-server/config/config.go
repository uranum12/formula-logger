package config

import (
	"log/slog"
	"os"

	"github.com/BurntSushi/toml"
)

type Config struct {
	Serial struct {
		Port string `toml:"port"`
		Baud int    `toml:"baud"`
	} `toml:"serial"`
	MQTT struct {
		Host string `toml:"host"`
		Port int    `toml:"port"`
	} `toml:"mqtt"`
	API struct {
		Port        int  `toml:"port"`
		GzipEnabled bool `toml:"gzip_enabled"`
		GzipLevel   int  `toml:"gzip_level"`
	} `toml:"api"`
	Socket struct {
		Serial string `toml:"serial"`
	} `toml:"socket"`
	CSV struct {
		FlushInterval int `toml:"flush_interval"`
	} `toml:"csv"`
}

// LoadConfig TOML を読み込む（エラー時は slog で出力して終了）
func LoadConfig(path string) Config {
	var cfg Config

	if _, err := os.Stat(path); err != nil {
		slog.Error("Config file not found", "path", path, "error", err)
		os.Exit(1)
	}

	if _, err := toml.DecodeFile(path, &cfg); err != nil {
		slog.Error("Failed to parse config", "path", path, "error", err)
		os.Exit(1)
	}

	slog.Info("Load config file", "config", cfg)

	return cfg
}
