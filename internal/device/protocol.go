package device

import (
	"errors"
	"fmt"
	"slices"
)

const (
	FlashSize       = 0x200000 // FIXME
	FlashPageSize   = 0x000100
	FlashSectorSize = 0x001000
	FlashBlockSize  = 0x010000
)

type operation = byte

const (
	opPowerUp operation = iota + 1
	opPowerDown
	opJedecId
	opRead
	opWrite
	opEraseSector
	opEraseBlock
	opEraseChip
)

type data = byte

const (
	dataPowerUp data = iota + 1
	dataPowerDown
	dataJedecId
	dataRead
	dataEraseSector
	dataEraseBlock
	dataEraseChip
)

type report = byte

const (
	reportFlashPage report = iota + 1
	reportData
)

type status = byte

const (
	statusOk status = iota
	statusUnpowered
	statusInvalidRequest
	statusInvalidCommandId
	statusInvalidFlashPageRead
	statusInvalidFlashPageWrite
	statusLocked
)

var (
	ErrUnpowered             = errors.New("iceflashprog: protocol: flash chip is unpowered")
	ErrInvalidRequest        = errors.New("iceflashprog: protocol: invalid request")
	ErrInvalidCommandId      = errors.New("iceflashprog: protocol: invalid command id")
	ErrInvalidFlashPageRead  = errors.New("iceflashprog: protocol: invalid flash page read")
	ErrInvalidFlashPageWrite = errors.New("iceflashprog: protocol: invalid flash page write, failed to verify")
	ErrLocked                = errors.New("iceflashprog: protocol: device is locked")

	errorMap = map[status]error{
		statusOk:                    nil,
		statusUnpowered:             ErrUnpowered,
		statusInvalidRequest:        ErrInvalidRequest,
		statusInvalidCommandId:      ErrInvalidCommandId,
		statusInvalidFlashPageRead:  ErrInvalidFlashPageRead,
		statusInvalidFlashPageWrite: ErrInvalidFlashPageWrite,
		statusLocked:                ErrLocked,
	}

	operationMap = map[operation]struct {
		data       data
		requestId  report
		responseId report
	}{
		opPowerUp:     {dataPowerUp, 2, 2},
		opPowerDown:   {dataPowerDown, 2, 2},
		opJedecId:     {dataJedecId, 2, 2},
		opRead:        {dataRead, 2, 1},
		opWrite:       {0, 1, 2},
		opEraseSector: {dataEraseSector, 2, 2},
		opEraseBlock:  {dataEraseBlock, 2, 2},
		opEraseChip:   {dataEraseChip, 2, 2},
	}
)

func (d *Device) opCall(op operation, data []byte) ([]byte, error) {
	obj, ok := operationMap[op]
	if !ok {
		return nil, fmt.Errorf("iceflashprog: protocol: invalid operation: %d", op)
	}

	dataToSend := []byte{}

	switch obj.requestId {
	case reportFlashPage:
		if l := len(data); l != 259 {
			return nil, fmt.Errorf("iceflashprog: protocol: invalid request data length for report %d: %d", obj.requestId, l)
		}
		dataToSend = data

	case reportData:
		if l := len(data); l != 3 {
			return nil, fmt.Errorf("iceflashprog: protocol: invalid request data length for report %d: %d", obj.requestId, l)
		}
		dataToSend = append([]byte{obj.data}, data...)
	}

	id, data, err := d.call(obj.requestId, dataToSend)
	if err != nil {
		return nil, err
	}
	if id != obj.responseId {
		return nil, fmt.Errorf("iceflashprog: protocol: invalid report id for response: %d", obj.responseId)
	}

	switch obj.responseId {
	case reportFlashPage:
		if l := len(data); l != FlashPageSize {
			return nil, fmt.Errorf("iceflashprog: protocol: invalid response data length for report %d: %d", obj.responseId, l)
		}
		return data, nil

	case reportData:
		if l := len(data); l != 4 {
			return nil, fmt.Errorf("iceflashprog: protocol: invalid response data length for report %d: %d", obj.responseId, l)
		}

		err, ok := errorMap[data[0]]
		if !ok {
			return nil, fmt.Errorf("iceflashprog: protocol: unknown error: %d", data[0])
		}
		if err != nil {
			return nil, err
		}

		return data[1:], nil
	}

	return nil, fmt.Errorf("iceflashprog: protocol: unknown error")
}

func (d *Device) PowerUp() error {
	_, err := d.opCall(opPowerUp, []byte{0, 0, 0})
	return err
}

func (d *Device) PowerDown() error {
	_, err := d.opCall(opPowerDown, []byte{0, 0, 0})
	if errors.Is(err, ErrUnpowered) {
		return nil
	}
	return fmt.Errorf("iceflashprog: protocol: failed to power down")
}

func (d *Device) GetJedecId() (byte, uint16, error) {
	data, err := d.opCall(opJedecId, []byte{0, 0, 0})
	if err != nil {
		return 0, 0, err
	}
	return data[0], uint16(data[1])<<8 | uint16(data[2]), nil
}

func (d *Device) ReadFlashPage(addr uint32) ([]byte, error) {
	data, err := d.opCall(opRead, []byte{byte(addr >> 16), byte(addr >> 8), byte(addr)})
	if err != nil {
		return nil, err
	}
	return data, nil
}

func (d *Device) WriteFlashPage(addr uint32, data []byte) error {
	if l := len(data); l > FlashPageSize {
		return fmt.Errorf("iceflashprog: protocol: got more data to write to flash than one page: %d", l)
	}

	data2 := slices.Clone(data)
	for range FlashPageSize - len(data) {
		data2 = append(data2, 0xff)
	}
	_, err := d.opCall(opWrite, append([]byte{byte(addr >> 16), byte(addr >> 8), byte(addr)}, data2...))
	return err
}

func (d *Device) EraseFlashSector(addr uint32) error {
	_, err := d.opCall(opEraseSector, []byte{byte(addr >> 16), byte(addr >> 8), byte(addr)})
	return err
}

func (d *Device) EraseFlashBlock(addr uint32) error {
	_, err := d.opCall(opEraseBlock, []byte{byte(addr >> 16), byte(addr >> 8), byte(addr)})
	return err
}

func (d *Device) EraseChip() error {
	_, err := d.opCall(opEraseChip, []byte{0, 0, 0})
	return err
}
