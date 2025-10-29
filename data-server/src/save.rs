use std::{
    fs,
    io::Cursor,
    path::PathBuf,
    time::{Duration, Instant},
};

use anyhow::{Context, Result, bail};
use rmpv::Value;
use rmpv::decode::read_value;
use rmpv::encode::write_value;

use crate::util::database::{self, Data};
use crate::util::socket;

fn extract_msg(msg: &[u8]) -> Result<(String, Vec<u8>)> {
    let mut cur = Cursor::new(&msg);
    let val = read_value(&mut cur).context("Failed to decode message")?;

    let map = match val {
        Value::Map(m) => m,
        _ => bail!("Expected a map but found {:?}", val),
    };

    let mut topic_opt: Option<String> = None;
    let mut payload_opt: Option<Vec<u8>> = None;

    for (k, v) in map {
        match k.as_str() {
            Some("topic") => {
                topic_opt = v.as_str().map(String::from);
            }
            Some("payload") => {
                let mut buf = Vec::new();
                write_value(&mut buf, &v).context("Failed to encode payload")?;
                payload_opt = Some(buf);
            }
            _ => {}
        }
    }

    let topic = topic_opt.context("Missing 'topic' field")?;
    let payload = payload_opt.context("Missing 'payload' field")?;

    Ok((topic, payload))
}

pub fn save() -> Result<()> {
    const DB_DIR: &str = "data";
    const BULK_SIZE: usize = 100;
    const BULK_INTERVAL: u64 = 1;

    let start = Instant::now();

    let db_dir = PathBuf::from(DB_DIR);
    if !db_dir.exists() {
        fs::create_dir_all(&db_dir)
            .with_context(|| format!("Failed to create directory: {}", db_dir.display()))?;
    }

    let filename = database::db_name(&db_dir)?;
    let db_path = db_dir.join(filename);
    let mut conn = database::db_init(&db_path)?;

    let publishers = ["ipc:///tmp/logger_spi.sock", "ipc:///tmp/logger_gps.sock"];

    let socket = socket::init_sub(&publishers)?;

    let mut buffer: Vec<Data> = Vec::with_capacity(BULK_SIZE);
    let mut last_flush = Instant::now();

    loop {
        let msg = match socket.recv() {
            Ok(m) => m,
            Err(e) => {
                eprintln!("msg recieve error {e}");
                continue;
            }
        };
        match extract_msg(&msg) {
            Ok((topic, payload)) => {
                let time = start.elapsed().as_micros() as u64;
                let data = Data::new(time, topic, payload);
                buffer.push(data);
            }
            Err(e) => {
                eprintln!("msg decode error {e}");
            }
        }

        if BULK_SIZE <= buffer.len() || Duration::from_secs(BULK_INTERVAL) <= last_flush.elapsed() {
            if !buffer.is_empty() {
                if let Err(e) = database::db_insert(&mut conn, &buffer) {
                    eprintln!("db insert error: {e}");
                    continue;
                }
                buffer.clear();
                last_flush = Instant::now();
            }
        }
    }
}
