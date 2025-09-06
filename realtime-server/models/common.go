package models

type DataTime struct {
	Sec  int64 `json:"sec"`
	Usec int64 `json:"usec"`
}

func (dt DataTime) GetUsec() int64 {
	return dt.Sec*1_000_000 + dt.Usec
}

type TimeProvider interface {
	GetUsec() int64
}

type DBTime struct {
	Usec int64 `db:"usec"`
	Time int64 `db:"time"`
}
