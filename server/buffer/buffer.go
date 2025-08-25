package buffer

import (
	"container/ring"
	"sync"
)

type Buf[TData any, TDB any] struct {
	mu   sync.Mutex
	ring *ring.Ring
	buf  []TDB
}

func NewBuf[TData any, TDB any](ringSize, bufCap int) *Buf[TData, TDB] {
	return &Buf[TData, TDB]{
		mu:   sync.Mutex{},
		ring: ring.New(ringSize),
		buf:  make([]TDB, 0, bufCap),
	}
}

func (b *Buf[TData, TDB]) Add(data TData, db TDB) {
	b.mu.Lock()
	defer b.mu.Unlock()

	b.ring.Value = data
	b.ring = b.ring.Next()
	b.buf = append(b.buf, db)
}

func (b *Buf[TData, TDB]) GetRing() []TData {
	b.mu.Lock()
	defer b.mu.Unlock()

	result := make([]TData, 0, b.ring.Len())
	b.ring.Do(func(v any) {
		if v != nil {
			result = append(result, v.(TData))
		}
	})
	return result
}

func (b *Buf[TData, TDB]) PopBuf() []TDB {
	b.mu.Lock()
	defer b.mu.Unlock()

	out := make([]TDB, len(b.buf))
	copy(out, b.buf)
	b.buf = b.buf[:0]
	return out
}

func (b *Buf[TData, TDB]) GetBufSize() int {
	return len(b.buf)
}
