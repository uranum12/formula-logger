use std::{fs, path::Path};

use anyhow::{Context, Result};
use serde::Deserialize;

#[derive(Deserialize)]
pub struct SpiConfig {
    pub dev: String,
    pub baud: u32,
}

#[derive(Deserialize)]
pub struct SerialConfig {
    pub dev: String,
    pub baud: u32,
}

#[derive(Deserialize)]
pub struct SaveConfig {
    pub db_dir: String,
    pub bulk_size: u32,
    pub bulk_interval: u32,
}

#[derive(Deserialize)]
pub struct SocketConfig {
    pub spi: String,
    pub gps: String,
}

#[derive(Deserialize)]
pub struct Config {
    pub spi: SpiConfig,
    pub serial: SerialConfig,
    pub save: SaveConfig,
    pub socket: SocketConfig,
}

pub fn load(path: &Path) -> Result<Config> {
    let config_str = fs::read_to_string(&path)
        .with_context(|| format!("Failed to read config file: {}", path.display()))?;
    let config: Config = toml::from_str(&config_str)
        .with_context(|| format!("Failed to deserialize config file: {}", path.display()))?;

    Ok(config)
}
