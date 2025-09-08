package models

type RPMData struct {
	DataTime
	RPM float64 `json:"rpm"`
}

type RPMDB struct {
	DBTime
	RPM float64 `db:"rpm"`
}

func MapRPMData(d RPMData, t DBTime) RPMDB {
	return RPMDB{
		DBTime: t,
		RPM:    d.RPM,
	}
}
