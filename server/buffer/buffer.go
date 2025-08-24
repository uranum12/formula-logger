package buffer

import (
	"container/ring"
	"sync"
)

type Buf[T any] struct {
	mu   sync.Mutex
	ring *ring.Ring
	buf  []T
}

func NewBuf[T any](ringSize, bufCap int) *Buf[T] {
	return &Buf[T]{
		mu:   sync.Mutex{},
		ring: ring.New(ringSize),
		buf:  make([]T, 0, bufCap),
	}
}

func (b *Buf[T]) Add(v T) {
	b.mu.Lock()
	defer b.mu.Unlock()

	b.ring.Value = v
	b.ring = b.ring.Next()
	b.buf = append(b.buf, v)
}

func (b *Buf[T]) GetRing() []T {
	b.mu.Lock()
	defer b.mu.Unlock()

	result := make([]T, 0, b.ring.Len())
	b.ring.Do(func(v any) {
		if v != nil {
			result = append(result, v.(T))
		}
	})
	return result
}

func (b *Buf[T]) PopBuf() []T {
	out := make([]T, len(b.buf))
	copy(out, b.buf)
	b.buf = b.buf[:0]
	return out
}

func (b *Buf[T]) GetBufSize() int {
	return len(b.buf)
}
