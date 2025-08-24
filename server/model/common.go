package model

type Time struct {
	Time uint64 `db:"time" json:"time"`
	Usec uint64 `db:"usec" json:"usec"`
}
