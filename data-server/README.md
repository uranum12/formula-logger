# 車載データ保存用サーバー

Serialで受け取ったデータをCSVで保存し、一部をMQTTに送信します

## build

Raspberry Pi OSのgoはバージョンが古いため、クロスビルドを推奨します

```sh
GOOS=linux GOARCH=arm64 go build
```

## install

systemdを用いた自動起動を設定できます  
以下のファイル構造を元に設定が書かれています

```
~/logger
├── api
├── mqtt
├── serial
├── config.toml
└── service
    ├── logger-api.service
    ├── logger-mqtt.service
    └── logger-serial.service
```

以下の手順で設定が可能です

```sh
# set config file
sudo cp service/logger-serial.service /etc/systemd/system/
sudo cp service/logger-mqtt.service /etc/systemd/system/
sudo cp service/logger-api.service /etc/systemd/system/
# reload daemon
sudo systemctl daemon-reload
# enable serial
sudo systemctl enable --now logger-serial.service
sudo systemctl status logger-serial.service
# enable mqtt
sudo systemctl enable --now logger-mqtt.service
sudo systemctl status logger-mqtt.service
# enable api
sudo systemctl enable --now logger-api.service
sudo systemctl status logger-api.service
```

ログは以下のコマンドで確認できます

```sh
journalctl -f -u logger-serial.service
journalctl -f -u logger-mqtt.service
journalctl -f -u logger-api.service
```

