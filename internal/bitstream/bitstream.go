package bitstream

import (
	"io"
	"os"

	"rafaelmartins.com/p/iceflashprog/internal/device"
)

type Bitstream struct {
	file string
	fp   *os.File
	size int64
	buf  []byte
}

func New(file string) (*Bitstream, error) {
	fp, err := os.Open(file)
	if err != nil {
		return nil, err
	}

	size, err := fp.Seek(0, 2)
	if err != nil {
		fp.Close()
		return nil, err
	}
	if _, err := fp.Seek(0, 0); err != nil {
		fp.Close()
		return nil, err
	}

	return &Bitstream{
		file: file,
		fp:   fp,
		size: size,
		buf:  make([]byte, device.FlashPageSize),
	}, nil
}

func (bs *Bitstream) Size() uint32 {
	return uint32(bs.size)
}

func (bs *Bitstream) resetFp() error {
	if bs.fp == nil {
		return nil
	}

	_, err := bs.fp.Seek(0, 0)
	return err
}

func (bs *Bitstream) ForEachFlashPage(f func(addr uint32, data []byte) error) error {
	if f == nil {
		return nil
	}

	if err := bs.resetFp(); err != nil {
		return err
	}

	addr := 0
	for {
		r, err := bs.fp.Read(bs.buf)
		if err != nil {
			if err == io.EOF {
				return nil
			}
			return err
		}

		if err := f(uint32(addr), bs.buf[:r]); err != nil {
			return err
		}
		addr += r
	}
}

func (bs *Bitstream) listFlash(sz uint32) []uint32 {
	rv := []uint32{}
	if bs.size == 0 {
		return rv
	}

	addr := uint32(0)
	for {
		rv = append(rv, addr)
		addr += sz
		if addr > uint32(bs.size) {
			return rv
		}
	}
}

func (bs *Bitstream) ListFlashPages() []uint32 {
	return bs.listFlash(device.FlashPageSize)
}

func (bs *Bitstream) ListFlashSectors() []uint32 {
	return bs.listFlash(device.FlashSectorSize)
}

func (bs *Bitstream) ListFlashBlocks() []uint32 {
	return bs.listFlash(device.FlashBlockSize)
}

func (bs *Bitstream) Close() error {
	if bs.fp == nil {
		return nil
	}
	err := bs.fp.Close()
	bs.fp = nil
	return err
}
