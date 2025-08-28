module formula-logger/data-server/serial

go 1.25.0

require (
	formula-logger/data-server/config v0.0.0-00010101000000-000000000000
	go.bug.st/serial v1.6.4
)

require (
	github.com/BurntSushi/toml v1.5.0 // indirect
	github.com/creack/goselect v0.1.2 // indirect
	golang.org/x/sys v0.19.0 // indirect
)

replace formula-logger/data-server/config => ../config
