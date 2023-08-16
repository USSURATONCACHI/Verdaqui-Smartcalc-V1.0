use std::path::PathBuf;

fn main() {
    for arg in std::env::args().skip(1) {
        let path = PathBuf::from(&arg);
        if path.is_file() {
            eprintln!("Error: {arg} is an already existing FILE.");
        } else if path.is_dir() {
            println!("Directory {arg} already exists, nothing to do.");
        } else {
            match std::fs::create_dir_all(path) {
                Ok(()) => println!("Created directory: {arg}."),
                Err(e) => println!("Failed to create directory {arg}: {}.", e.to_string())
            }
        }
    }
}
