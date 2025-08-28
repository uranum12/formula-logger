module formula-logger/data-server/mqtt

go 1.25.0

require (
	formula-logger/data-server/config v0.0.0-00010101000000-000000000000
	github.com/eclipse/paho.mqtt.golang v1.5.0
)

require (
	github.com/BurntSushi/toml v1.5.0 // indirect
	github.com/gorilla/websocket v1.5.3 // indirect
	golang.org/x/net v0.27.0 // indirect
	golang.org/x/sync v0.7.0 // indirect
)

replace formula-logger/data-server/config => ../config
