use std::{thread, time::Duration};

use anyhow::{Context, Result};
use crc::{CRC_16_IBM_3740, Crc};
use spidev::{SpiModeFlags, Spidev, SpidevOptions, SpidevTransfer};

use crate::config::Config;
use crate::util::socket;

fn spi_init(spi_dev: &str, spi_baud: u32) -> Result<Spidev> {
    let mut spi =
        Spidev::open(spi_dev).with_context(|| format!("spidev {} open failed.", spi_dev))?;

    let options = SpidevOptions::new()
        .bits_per_word(8)
        .max_speed_hz(spi_baud)
        .mode(SpiModeFlags::SPI_MODE_0)
        .build();
    spi.configure(&options).with_context(|| {
        format!(
            "spidev {} configure failed. options: {:?}",
            spi_dev, options
        )
    })?;

    Ok(spi)
}

fn read_length(spi: &mut Spidev) -> Result<u16> {
    let mut rx_buf = [0u8; 2];

    let mut transfer = SpidevTransfer::read(&mut rx_buf);
    spi.transfer(&mut transfer)
        .context("spi transfer failed.")?;

    let len = u16::from_be_bytes(rx_buf);

    Ok(len)
}

fn read_data(spi: &mut Spidev, len: u16) -> Result<(u16, Vec<u8>)> {
    let buf_size: usize = len as usize + 4;
    let mut rx_buf = vec![0u8; buf_size];

    let mut transfer = SpidevTransfer::read(&mut rx_buf);
    spi.transfer(&mut transfer)
        .context("spi transfer failed.")?;

    let crc = u16::from_be_bytes([rx_buf[2], rx_buf[3]]);
    let data = rx_buf[4..].to_vec();

    Ok((crc, data))
}

fn write_next(spi: &mut Spidev) -> Result<()> {
    let tx_buf = [0x01u8];

    let mut transfer = SpidevTransfer::write(&tx_buf);
    spi.transfer(&mut transfer)
        .context("spi transfer failed.")?;

    Ok(())
}

pub fn spi(config: Config) -> Result<()> {
    let mut spi = spi_init(config.spi.dev.as_str(), config.spi.baud)?;

    let socket = socket::init_pub(config.socket.spi.as_str())?;

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
