package models

type StrokeRearData struct {
	DataTime
	Left  float64 `json:"left"`
	Right float64 `json:"right"`
}

type StrokeRearDB struct {
	DBTime
	Left  float64 `db:"left"`
	Right float64 `db:"right"`
}

func MapStrokeRearData(d StrokeRearData, t DBTime) StrokeRearDB {
	return StrokeRearDB{
		DBTime: t,
		Left:   d.Left,
		Right:  d.Right,
	}
}
