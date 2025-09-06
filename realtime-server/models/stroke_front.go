package models

type StrokeFrontData struct {
	DataTime
	Left  float64 `json:"left"`
	Right float64 `json:"right"`
}

type StrokeFrontDB struct {
	DBTime
	Left  float64 `db:"left"`
	Right float64 `db:"right"`
}

func MapStrokeFrontData(d StrokeFrontData, t DBTime) StrokeFrontDB {
	return StrokeFrontDB{
		DBTime: t,
		Left:   d.Left,
		Right:  d.Right,
	}
}
