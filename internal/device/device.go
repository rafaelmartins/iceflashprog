package device

import (
	"fmt"
	"sync"

	"rafaelmartins.com/p/usbhid"
)

type result struct {
	id   byte
	data []byte
}

type Device struct {
	dev    *usbhid.Device
	listen chan bool

	m      sync.Mutex
	result chan result
}

func New(serialNumber string) (*Device, error) {
	devices, err := usbhid.Enumerate(func(d *usbhid.Device) bool {
		if d.VendorId() != 0x16c0 {
			return false
		}
		if d.ProductId() != 0x05df {
			return false
		}
		if d.Product() != "iceflashprog" {
			return false
		}
		return true
	})
	if err != nil {
		return nil, err
	}

	if len(devices) == 0 {
		if serialNumber != "" {
			return nil, fmt.Errorf("iceflashprog: %w [%q]", usbhid.ErrNoDeviceFound, serialNumber)
		}
		return nil, fmt.Errorf("iceflashprog: %w", usbhid.ErrNoDeviceFound)
	}

	if serialNumber == "" {
		if len(devices) == 1 {
			return &Device{
				dev: devices[0],
			}, nil
		}

		sn := []string{}
		for _, dev := range devices {
			sn = append(sn, dev.SerialNumber())
		}
		return nil, fmt.Errorf("iceflashprog: %w %q", usbhid.ErrMoreThanOneDeviceFound, sn)
	}

	for _, dev := range devices {
		if dev.SerialNumber() == serialNumber {
			return &Device{
				dev: devices[0],
			}, nil
		}
	}

	return nil, fmt.Errorf("iceflashprog: %w [%q]", usbhid.ErrNoDeviceFound, serialNumber)
}

func (d *Device) Open() error {
	if err := d.dev.Open(true); err != nil {
		return err
	}

	d.listen = make(chan bool)
	return nil
}

func (d *Device) Listen() error {
	for {
		select {
		case <-d.listen:
			return nil
		default:
			if d.listen == nil {
				return nil
			}
		}

		id, buf, err := d.dev.GetInputReport()
		if err != nil {
			return err
		}
		if id != 1 && id != 2 {
			continue
		}

		if d.result == nil {
			return fmt.Errorf("iceflashprog: got result without any request pending: %+v", buf)
		}

		d.result <- result{
			id:   id,
			data: buf,
		}
	}
}

func (d *Device) call(id byte, data []byte) (byte, []byte, error) {
	d.m.Lock()
	defer d.m.Unlock()

	d.result = make(chan result)

	if err := d.dev.SetOutputReport(id, data); err != nil {
		close(d.result)
		d.result = nil
		return 0, nil, err
	}

	r := <-d.result

	close(d.result)
	d.result = nil

	return r.id, r.data, nil
}

func (d *Device) Close() error {
	if err := d.PowerDown(); err != nil {
		return err
	}

	close(d.listen)
	return d.dev.Close()
}
