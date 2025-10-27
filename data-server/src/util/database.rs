use std::fs;
use std::path::Path;

use anyhow::{Context, Result};
use rusqlite::{Connection, params};

#[derive(Debug)]
pub struct Data {
    time: u64,
    topic: String,
    data: Vec<u8>,
}

impl Data {
    pub fn new(time: u64, topic: String, data: Vec<u8>) -> Self {
        Data { time, topic, data }
    }
}

pub fn db_name(db_dir: &Path) -> Result<String> {
    let max_index = fs::read_dir(db_dir)
        .with_context(|| format!("Failed to read directory: {}", db_dir.display()))?
        .filter_map(|entry| {
            let path = entry.ok()?.path();
            if path.extension()?.to_str()?.eq_ignore_ascii_case("db") {
                path.file_stem()?.to_str()?.parse::<u64>().ok()
            } else {
                None
            }
        })
        .max();

    Ok(format!("{:03}.db", max_index.map_or(0, |m| m + 1)))
}

pub fn db_init(path: &Path) -> Result<Connection> {
    let conn = Connection::open(&path)
        .with_context(|| format!("Failed to open database at {}", path.display()))?;

    conn.execute_batch(
        r#"
        CREATE TABLE data(
            id INTEGER PRIMARY KEY,
            time INTEGER,
            topic TEXT,
            data BLOB
        );
        "#,
    )
    .context("Failed to create table")?;

    Ok(conn)
}

pub fn db_insert(conn: &mut Connection, records: &[Data]) -> Result<()> {
    let tx = conn.transaction().context("Failed to start transaction")?;

    {
        let mut stmt = tx
            .prepare("INSERT INTO data (time, topic, data) VALUES (?1, ?2, ?3)")
            .context("Failed to prepare insert statement")?;

        for record in records {
            stmt.execute(params![record.time, record.topic, record.data])
                .with_context(|| format!("Failed to insert record: {:?}", record))?;
        }
    }

    tx.commit().context("Failed to commit transaction")?;
    Ok(())
}
