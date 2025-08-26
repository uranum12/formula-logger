package model

type AF struct {
	Usec     uint64  `db:"usec" json:"usec"`
	AF       float64 `db:"af" json:"af"`
	Throttle float64 `db:"throttle" json:"throttle"`
}

type AFDB struct {
	AF
	Time int64 `db:"time"`
}
