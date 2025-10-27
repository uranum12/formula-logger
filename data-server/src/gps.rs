use std::io::{self, BufRead, BufReader};
use std::thread;
use std::time::Duration;

use anyhow::{Context, Result};
use chrono::{NaiveTime, Timelike};
use nmea::ParseResult;
use nmea::sentences::{FixType, GgaData, VtgData};
use nng::{Protocol, Socket};
use serde::Serialize;
use serialport::SerialPort;

use crate::util::socket;

#[derive(Debug, Serialize)]
struct GGAMsg {
    hour: u32,
    min: u32,
    sec: u32,
    msec: u32,

    lat: f64,
    lon: f64,

    hdop: f32,
    fix_type: u8,
}

#[derive(Debug, Serialize)]
struct VTGMsg {
    course: f32,
    speed: f32,
}

#[derive(Debug, Serialize)]
struct Msg<T> {
    topic: &'static str,
    payload: T,
}

fn serial_init(serial_dev: &str, serial_baud: u32) -> Result<BufReader<Box<dyn SerialPort>>> {
    let port = serialport::new(serial_dev, serial_baud)
        .timeout(Duration::from_millis(1000))
        .open()
        .with_context(|| format!("serial {serial_dev} open failed. baudrate: {serial_baud}"))?;
    let reader = BufReader::new(port);
    Ok(reader)
}

fn fix_type_to_u8(fix_type: FixType) -> u8 {
    use FixType::*;
    match fix_type {
        Invalid => 0,
        Gps => 1,
        DGps => 2,
        Pps => 3,
        Rtk => 4,
        FloatRtk => 5,
        _ => 6,
    }
}

fn make_gga_msg(gga: GgaData) -> GGAMsg {
    let time = gga.fix_time.unwrap_or(NaiveTime::MIN);
    let fix_type = fix_type_to_u8(gga.fix_type.unwrap_or(FixType::Invalid));

    GGAMsg {
        hour: time.hour(),
        min: time.minute(),
        sec: time.second(),
        msec: time.nanosecond() / 1_000_000,

        lat: gga.latitude.unwrap_or_default(),
        lon: gga.longitude.unwrap_or_default(),

        hdop: gga.hdop.unwrap_or(-1.0),
        fix_type: fix_type,
    }
}

fn make_vtg_msg(vtg: VtgData) -> VTGMsg {
    VTGMsg {
        course: vtg.true_course.unwrap_or_default(),
        speed: vtg.speed_over_ground.unwrap_or_default(),
    }
}

fn send_msg<T: Serialize>(socket: &Socket, topic: &'static str, payload: T) {
    let msg = Msg { topic, payload };
    match rmp_serde::to_vec_named(&msg) {
        Ok(bytes) => {
            if let Err((_, e)) = socket.send(&bytes) {
                eprintln!("socket.send failed: {e}");
            }
        }
        Err(e) => eprintln!("MsgPack serialize failed: {e}"),
    }
}

pub fn gps() {
    const SERIAL_DEV: &str = "/dev/serial0";
    const SERIAL_BAUD: u32 = 9600;
    const SOCKET_ADDR: &str = "ipc:///tmp/logger_gps.sock";

    let reader = match serial_init(SERIAL_DEV, SERIAL_BAUD) {
        Ok(r) => r,
        Err(e) => {
            eprintln!("Serial init failed: {e}");
            return;
        }
    };

    let socket = socket::init_pub(SOCKET_ADDR).unwrap();

    thread::sleep(Duration::from_secs(1));

    for line_result in reader.lines() {
        let line = match line_result {
            Ok(l) => l,
            Err(ref e) if e.kind() == io::ErrorKind::TimedOut => continue,
            Err(e) => {
                eprintln!("read line error: {e}");
                continue;
            }
        };

        let parsed = match nmea::parse_str(&line) {
            Ok(v) => v,
            Err(e) => {
                eprintln!("NMEA parse failed: {e}");
                continue;
            }
        };

        match parsed {
            ParseResult::GGA(gga) => send_msg(&socket, "gps/gga", make_gga_msg(gga)),
            ParseResult::VTG(vtg) => send_msg(&socket, "gps/vtg", make_vtg_msg(vtg)),
            _ => (),
        }
    }
}
