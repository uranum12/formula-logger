package model

type Env struct {
	Usec uint64  `db:"usec" json:"usec"`
	Temp float64 `db:"temp" json:"temp"`
	Pres float64 `db:"pres" json:"pres"`
	Hum  float64 `db:"hum" json:"hum"`
}

type EnvDB struct {
	Env
	Time int64 `db:"time"`
}
