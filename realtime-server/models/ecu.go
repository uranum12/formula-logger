package models

type ECUData struct {
	DataTime
	ECT float64 `json:"ect"`
	TPS float64 `json:"tps"`
	IAP float64 `json:"iap"`
	GP  float64 `json:"gp"`
}

type ECUDB struct {
	DBTime
	ECT float64 `db:"ect"`
	TPS float64 `db:"tps"`
	IAP float64 `db:"iap"`
	GP  float64 `db:"gp"`
}

func MapECUData(d ECUData, t DBTime) ECUDB {
	return ECUDB{
		DBTime: t,
		ECT:    d.ECT,
		TPS:    d.TPS,
		IAP:    d.IAP,
		GP:     d.GP,
	}
}
