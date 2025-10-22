use std::env;

fn main() {
    match env::args().nth(1).as_deref() {
        Some("gps") => println!("GPS"),
        Some("spi") => println!("SPI"),
        Some("save") => println!("Save"),
        Some("mqtt") => println!("MQTT"),
        Some("api") => println!("API"),
        _ => println!("other"),
    }
}
