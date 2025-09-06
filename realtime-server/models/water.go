package models

type WaterData struct {
	DataTime
	InletTemp  float64 `json:"inlet_temp"`
	OutletTemp float64 `json:"outlet_temp"`
}

type WaterDB struct {
	DBTime
	InletTemp  float64 `db:"inlet_temp"`
	OutletTemp float64 `db:"outlet_temp"`
}

func MapWaterData(d WaterData, t DBTime) WaterDB {
	return WaterDB{
		DBTime:     t,
		InletTemp:  d.InletTemp - 273.15,
		OutletTemp: d.OutletTemp - 273.15,
	}
}
