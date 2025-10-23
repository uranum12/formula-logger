use std::{io, thread, time::Duration};

use crc::{CRC_16_IBM_3740, Crc};
use nng::{Protocol, Socket};
use spidev::{SpiModeFlags, Spidev, SpidevOptions, SpidevTransfer};

fn spi_init(spi_dev: &str, spi_baud: u32) -> io::Result<Spidev> {
    let mut spi = Spidev::open(spi_dev)?;

    let options = SpidevOptions::new()
        .bits_per_word(8)
        .max_speed_hz(spi_baud)
        .mode(SpiModeFlags::SPI_MODE_0)
        .build();
    spi.configure(&options)?;

    Ok(spi)
}

fn read_length(spi: &mut Spidev) -> io::Result<u16> {
    let mut rx_buf = [0u8; 2];

    let mut transfer = SpidevTransfer::read(&mut rx_buf);
    spi.transfer(&mut transfer)?;

    let len = u16::from_be_bytes(rx_buf);

    Ok(len)
}

fn read_data(spi: &mut Spidev, len: u16) -> io::Result<(u16, Vec<u8>)> {
    let buf_size: usize = len as usize + 4;
    let mut rx_buf = vec![0u8; buf_size];

    let mut transfer = SpidevTransfer::read(&mut rx_buf);
    spi.transfer(&mut transfer)?;

    let crc = u16::from_be_bytes([rx_buf[2], rx_buf[3]]);
    let data = rx_buf[4..].to_vec();

    Ok((crc, data))
}

fn write_next(spi: &mut Spidev) -> io::Result<()> {
    let tx_buf = [0x01u8];

    let mut transfer = SpidevTransfer::write(&tx_buf);
    spi.transfer(&mut transfer)?;

    Ok(())
}

pub fn spi() {
    const SPI_DEV: &str = "/dev/spidev0.0";
    const SPI_BAUD: u32 = 1_000_000;
    const SOCKET_ADDR: &str = "ipc:///tmp/logger_spi.sock";

    let mut spi = match spi_init(SPI_DEV, SPI_BAUD) {
        Ok(s) => s,
        Err(e) => {
            eprintln!("SPI init failed: {e}");
            return;
        }
    };

    let socket = match Socket::new(Protocol::Pub0).and_then(|s| {
        s.listen(SOCKET_ADDR)?;
        Ok(s)
    }) {
        Ok(s) => s,
        Err(e) => {
            eprintln!("Socket init failed: {e}");
            return;
        }
    };

    let crc16 = Crc::<u16>::new(&CRC_16_IBM_3740);

    thread::sleep(Duration::from_secs(1));

    loop {
        let len = match read_length(&mut spi) {
            Ok(l) => l,
            Err(e) => {
                eprintln!("read_length error: {e}");
                continue;
            }
        };

        if len == 0 {
            if let Err(e) = write_next(&mut spi) {
                eprintln!("write_next error: {e}")
            };
            continue;
        }

        let (crc, data) = match read_data(&mut spi, len) {
            Ok(v) => v,
            Err(e) => {
                eprintln!("read_data error: {e}");
                continue;
            }
        };

        let result = crc16.checksum(&data);
        if crc != result {
            continue;
        }

        if let Err((_, e)) = socket.send(&data) {
            eprintln!("socket.send error: {e}");
        }

        if let Err(e) = write_next(&mut spi) {
            eprintln!("write_next error: {e}");
        }
    }
}
