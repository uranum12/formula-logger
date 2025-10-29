use std::env;

mod gps;
mod save;
mod spi;
mod util;

fn main() {
    match env::args().nth(1).as_deref() {
        Some("gps") => gps::gps().unwrap(),
        Some("spi") => spi::spi().unwrap(),
        Some("save") => save::save().unwrap(),
        Some("mqtt") => println!("MQTT"),
        Some("api") => println!("API"),
        _ => println!("other"),
    }
}
