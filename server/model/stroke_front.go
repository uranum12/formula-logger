package model

type StrokeFront struct {
	Usec  uint64  `db:"usec" json:"usec"`
	Left  float64 `db:"left" json:"left"`
	Right float64 `db:"right" json:"right"`
}

type StrokeFrontDB struct {
	StrokeFront
	Time int64 `db:"time"`
}
