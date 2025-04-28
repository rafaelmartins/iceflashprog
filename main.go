package main

import (
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"runtime/debug"
	"slices"

	"github.com/schollz/progressbar/v3"
	"rafaelmartins.com/p/iceflashprog/internal/bitstream"
	"rafaelmartins.com/p/iceflashprog/internal/cleanup"
	"rafaelmartins.com/p/iceflashprog/internal/device"
)

var (
	check        = flag.Bool("c", false, "compare file content against flash memory")
	detect       = flag.Bool("d", false, "detect flash memory and exit")
	chipErase    = flag.Bool("e", false, "erase whole flash memory and exit")
	skipErase    = flag.Bool("n", false, "do not erase flash before writing")
	read         = flag.Bool("r", false, "read flash memory to file")
	serialNumber = flag.String("s", "", "device serial number")
	version      = flag.Bool("V", false, "show version and exit")
)

func readToFile(dev *device.Device, f string) error {
	if err := os.MkdirAll(filepath.Dir(f), 0777); err != nil {
		return err
	}

	fp, err := os.Create(f)
	if err != nil {
		return err
	}
	defer fp.Close()

	bar := progressbar.DefaultBytes(device.FlashSize, "Reading")

	addr := uint32(0)
	for addr <= device.FlashSize {
		data, err := dev.ReadFlashPage(addr)
		if err != nil {
			return err
		}

		bar.Add(len(data))

		if _, err := fp.Write(data); err != nil {
			return err
		}

		addr += device.FlashPageSize
	}
	return nil
}

func writeToChip(dev *device.Device, bs *bitstream.Bitstream) error {
	if !*skipErase {
		blocks := bs.ListFlashBlocks()
		bar := progressbar.DefaultBytes(int64(len(blocks)*device.FlashBlockSize), "Erasing")

		for _, addr := range blocks {
			if err := dev.EraseFlashBlock(addr); err != nil {
				return err
			}

			bar.Add(device.FlashBlockSize)
		}
	}

	bar := progressbar.DefaultBytes(int64(bs.Size()), "Writing")

	return bs.ForEachFlashPage(func(addr uint32, data []byte) error {
		if err := dev.WriteFlashPage(addr, data); err != nil {
			return err
		}

		bar.Add(len(data))
		return nil
	})
}

func checkFile(dev *device.Device, bs *bitstream.Bitstream) error {
	bar := progressbar.DefaultBytes(int64(bs.Size()), "Checking")

	return bs.ForEachFlashPage(func(addr uint32, data []byte) error {
		mdata, err := dev.ReadFlashPage(addr)
		if err != nil {
			return err
		}

		if slices.Compare(data, mdata[:len(data)]) != 0 {
			return fmt.Errorf("mismatch: %+v != %+v", data, mdata[:len(data)])
		}

		bar.Add(len(data))
		return nil
	})
}

func main() {
	defer cleanup.Cleanup()

	flag.Parse()

	if *version {
		if bi, ok := debug.ReadBuildInfo(); ok {
			fmt.Println(bi.Main.Version)
			return
		}

		fmt.Println("UNKNOWN")
		cleanup.Exit(1)
	}

	dev, err := device.New(*serialNumber)
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

	fmt.Printf("Manufacturer: %#02x\nDevice ID: %#04x\n", mfr, devid)

	if *detect {
		return
	}

	fmt.Println()

	if *chipErase {
		fmt.Println("Erasing chip ...")
		cleanup.Check(dev.EraseChip())
		fmt.Println("Done!")
		return
	}

	if len(flag.Args()) != 1 {
		cleanup.Check("invalid arguments")
	}

	if *read {
		cleanup.Check(readToFile(dev, flag.Arg(0)))
		return
	}

	bs, err := bitstream.New(flag.Arg(0))
	cleanup.Check(err)
	cleanup.Register(bs)

	if *check {
		cleanup.Check(checkFile(dev, bs))
		return
	}

	cleanup.Check(writeToChip(dev, bs))
}
