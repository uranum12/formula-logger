# 車載データ保存用サーバー

Serialで受け取ったデータをCSVで保存し、一部をMQTTに送信します

## build

Raspberry Pi OSのgoはバージョンが古いため、クロスビルドを推奨します

```sh
GOOS=linux GOARCH=arm64 go build
```

