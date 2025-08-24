package model

type Acc struct {
	Usec uint64  `db:"usec" json:"usec"`
	X    float64 `db:"x" json:"x"`
	Y    float64 `db:"y" json:"y"`
	Z    float64 `db:"z" json:"z"`
}

type AccDB struct {
	Acc
	Time int64 `db:"time"`
}
