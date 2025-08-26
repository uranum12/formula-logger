package model

type ECU struct {
	Usec uint64  `db:"usec" json:"usec"`
	ECT  float64 `db:"ect" json:"ect"`
	TPS  float64 `db:"tps" json:"tps"`
	IAP  float64 `db:"iap" json:"iap"`
	GP   int8    `db:"gp" json:"gp"`
	RPM  int16   `db:"rpm" json:"rpm"`
}

type ECUDB struct {
	ECU
	Time int64 `db:"time"`
}
