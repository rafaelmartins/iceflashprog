package main

import (
	"fmt"

	"rafaelmartins.com/p/iceflashprog/internal/cleanup"
	"rafaelmartins.com/p/iceflashprog/internal/device"
)

func main() {
	defer cleanup.Cleanup()

	dev, err := device.New("")
	if err != nil {
		cleanup.Check(err)
	}
	cleanup.Check(err)

	cleanup.Check(dev.Open())
	cleanup.Register(dev)

	go func() {
		cleanup.Check(dev.Listen())
	}()

	cleanup.Check(dev.PowerUp())

	mfr, devid, err := dev.GetJedecId()
	cleanup.Check(err)
	fmt.Printf("manufacturer=%#02x; device_id=%#04x\n", mfr, devid)

	cleanup.Check(dev.PowerDown())
}
