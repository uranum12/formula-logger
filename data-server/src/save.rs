use std::{
    fs,
    io::Cursor,
    path::{Path, PathBuf},
    thread,
    time::{Duration, Instant},
};

use anyhow::Result;
use nng::options::{Options, protocol::pubsub::Subscribe};
use nng::{Protocol, Socket};
use rmpv::Value;
use rmpv::decode::read_value;
use rmpv::encode::write_value;
use rusqlite::{Connection, params};

#[derive(Debug)]
struct Data {
    time: u64,
    topic: String,
    data: Vec<u8>,
}

fn db_name(db_dir: &Path) -> Result<String> {
    let mut max_index: Option<u64> = None;

    for entry in fs::read_dir(db_dir)? {
        let path = entry?.path();

        if !path.exists() {
            continue;
        }

        if let Some(ext) = path.extension().and_then(|s| s.to_str()) {
            if ext.eq_ignore_ascii_case("db") {
                if let Some(stem) = path.file_stem().and_then(|s| s.to_str()) {
                    if !stem.is_empty() && stem.chars().all(|c| c.is_ascii_digit()) {
                        if let Ok(val) = stem.parse::<u64>() {
                            max_index = Some(max_index.map_or(val, |m| m.max(val)));
                        }
                    }
                }
            }
        }
    }

    let next = match max_index {
        Some(m) => m + 1,
        None => 0,
    };

    let filename = format!("{:03}.db", next);
    Ok(filename)
}

fn db_init(path: &Path) -> Result<Connection> {
    let conn = Connection::open(&path)?;

    conn.execute_batch(
        r#"
        CREATE TABLE data(
            id INTEGER PRIMARY KEY,
            time INTEGER,
            topic TEXT,
            data BLOB
        );
        "#,
    )?;

    Ok(conn)
}

fn db_insert(conn: &mut Connection, records: &[Data]) -> Result<()> {
    let tx = conn.transaction()?;

    {
        let mut stmt = tx.prepare("INSERT INTO data (time, topic, data) VALUES (?1, ?2, ?3)")?;

        for record in records {
            stmt.execute(params![record.time, record.topic, record.data])?;
        }
    }

    tx.commit()?;
    Ok(())
}

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

    let filename = db_name(&db_dir).unwrap();
    let db_path = db_dir.join(filename);
    let mut conn = db_init(&db_path).unwrap();

    let publishers = ["ipc:///tmp/logger_spi.sock", "ipc:///tmp/logger_gps.sock"];

    let socket = Socket::new(Protocol::Sub0).unwrap();
    socket.set_opt::<Subscribe>(b"".to_vec()).unwrap();

    for pub_path in &publishers {
        loop {
            match socket.dial(&pub_path) {
                Ok(_) => {
                    println!("connected: {}", pub_path);
                    break;
                }
                Err(e) => {
                    eprintln!("daial to {} failed: {e}", pub_path);
                    thread::sleep(Duration::from_secs(1));
                }
            }
        }
    }

    let mut buffer: Vec<Data> = Vec::with_capacity(BULK_SIZE);
    let mut last_flush = Instant::now();

    loop {
        let msg = socket.recv().unwrap();
        match extract_msg(&msg) {
            Some((t, p)) => {
                let data = Data {
                    time: start.elapsed().as_micros() as u64,
                    topic: t,
                    data: p,
                };

                db_insert(&mut conn, &[data]).unwrap();
            }
            None => {
                eprintln!("msg decode error");
            }
        }

        if BULK_SIZE <= buffer.len() || Duration::from_secs(BULK_INTERVAL) <= last_flush.elapsed() {
            if !buffer.is_empty() {
                if let Err(e) = db_insert(&mut conn, &buffer) {
                    eprintln!("db insert error: {e}");
                }
                buffer.clear();
                last_flush = Instant::now();
            }
        }
    }
}
