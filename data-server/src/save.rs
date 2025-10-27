use std::{
    fs,
    io::Cursor,
    path::PathBuf,
    time::{Duration, Instant},
};

use rmpv::Value;
use rmpv::decode::read_value;
use rmpv::encode::write_value;

use crate::util::database::{self, Data};
use crate::util::socket;

fn extract_msg(msg: &[u8]) -> Option<(String, Vec<u8>)> {
    let mut cur = Cursor::new(&msg);
    let val = read_value(&mut cur).unwrap();

    if let Value::Map(map) = val {
        let mut topic: Option<String> = None;
        let mut payload: Option<Vec<u8>> = None;

        for (k, v) in map {
            if let Some(key) = k.as_str() {
                if key == "topic" {
                    topic = v.as_str().map(|s| s.to_string());
                } else if key == "payload" {
                    let mut buf = Vec::new();
                    write_value(&mut buf, &v).ok().unwrap();
                    payload = Some(buf);
                }
            }
        }

        topic.zip(payload)
    } else {
        None
    }
}

pub fn save() {
    const DB_DIR: &str = "data";
    const BULK_SIZE: usize = 100;
    const BULK_INTERVAL: u64 = 1;

    let start = Instant::now();

    let db_dir = PathBuf::from(DB_DIR);
    if !db_dir.exists() {
        fs::create_dir_all(&db_dir).unwrap();
    }

    let filename = database::db_name(&db_dir).unwrap();
    let db_path = db_dir.join(filename);
    let mut conn = database::db_init(&db_path).unwrap();

    let publishers = ["ipc:///tmp/logger_spi.sock", "ipc:///tmp/logger_gps.sock"];

    let socket = socket::init_sub(&publishers).unwrap();

    let mut buffer: Vec<Data> = Vec::with_capacity(BULK_SIZE);
    let mut last_flush = Instant::now();

    loop {
        let msg = socket.recv().unwrap();
        match extract_msg(&msg) {
            Some((topic, payload)) => {
                let time = start.elapsed().as_micros() as u64;
                let data = Data::new(time, topic, payload);

                database::db_insert(&mut conn, &[data]).unwrap();
            }
            None => {
                eprintln!("msg decode error");
            }
        }

        if BULK_SIZE <= buffer.len() || Duration::from_secs(BULK_INTERVAL) <= last_flush.elapsed() {
            if !buffer.is_empty() {
                if let Err(e) = database::db_insert(&mut conn, &buffer) {
                    eprintln!("db insert error: {e}");
                }
                buffer.clear();
                last_flush = Instant::now();
            }
        }
    }
}
