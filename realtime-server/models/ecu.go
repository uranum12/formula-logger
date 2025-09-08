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
	GP  int     `db:"gp"`
}

func calcGear(v float64) int {
	vRef := []float64{
		0.0, 0.88, 1.10, 1.46, 1.77, 2.09, 2.38, 3.0,
	}

	for i := range 6 {
		low := (vRef[i] + vRef[i+1]) / 2.0
		high := (vRef[i+1] + vRef[i+2]) / 2.0
		if low <= v && v < high {
			return i + 1
		}
	}
	return 0
}

func MapECUData(d ECUData, t DBTime) ECUDB {
	return ECUDB{
		DBTime: t,
		ECT:    d.ECT,
		TPS:    d.TPS,
		IAP:    d.IAP,
		GP:     calcGear(d.GP),
	}
}
