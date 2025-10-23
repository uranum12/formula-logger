use std::env;

mod spi;

fn main() {
    match env::args().nth(1).as_deref() {
        Some("gps") => println!("GPS"),
        Some("spi") => spi::spi(),
        Some("save") => println!("Save"),
        Some("mqtt") => println!("MQTT"),
        Some("api") => println!("API"),
        _ => println!("other"),
    }
}
