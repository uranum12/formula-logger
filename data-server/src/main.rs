use std::{env, path::PathBuf};

mod config;
mod gps;
mod save;
mod spi;
mod util;

fn main() {
    let config_path = PathBuf::from("config.toml");

    let config = config::load(&config_path).unwrap();

    match env::args().nth(1).as_deref() {
        Some("gps") => gps::gps(config).unwrap(),
        Some("spi") => spi::spi(config).unwrap(),
        Some("save") => save::save(config).unwrap(),
        Some("mqtt") => println!("MQTT"),
        Some("api") => println!("API"),
        _ => println!("other"),
    }
}
