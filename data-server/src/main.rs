use std::env;

mod gps;
mod save;
mod spi;

fn main() {
    match env::args().nth(1).as_deref() {
        Some("gps") => gps::gps(),
        Some("spi") => spi::spi(),
        Some("save") => save::save(),
        Some("mqtt") => println!("MQTT"),
        Some("api") => println!("API"),
        _ => println!("other"),
    }
}
