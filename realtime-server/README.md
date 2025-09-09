# リアルタイム監視用サーバー

MQTTで受け取ったデータを加工し、インメモリDBに保存します。

## build

Raspberry Pi OSのGoはバージョンが古いため、クロスビルドを推奨します。  
index.htmlを埋め込むため、このディレクトリにコピーしてください。  
インメモリDBにSQLiteを採用したため、CGoによるコンパイルが挟まります。  
そのため、Goだけでなく、Cのクロスコンパイラも必要です。  
gccやclangはめんどくさいのでzigのCコンパイラ機能を利用しました。

```sh
cp ../webapp/dist/index.html .
CC='zig cc -target aarch64-linux-gnu' GOOS=linux GOARCH=arm64 CGO_ENABLED=1 go build
```

## install

systemdを用いた自動起動を設定できます。  
以下のコマンドで設定可能です。

```sh
sudo cp infra/logger-realtime.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now logger-realtime.service
```

ログは以下のコマンドで確認可能です。

```sh
journalctl -f -u logger-realtime.service
```

リアルタイム用にMQTTブローカーの設定が必要です。  
`infra/mosquitto.conf` を参考に設定してください。  
また、固定IPも必須です。

