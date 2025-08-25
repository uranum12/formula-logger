package model

type Water struct {
	Usec       uint64  `db:"usec" json:"usec"`
	InletTemp  float64 `db:"inlet_temp" json:"inlet_temp"`
	OutletTemp float64 `db:"outlet_temp" json:"outlet_temp"`
}

type WaterDB struct {
	Water
	Time int64 `db:"time"`
}
