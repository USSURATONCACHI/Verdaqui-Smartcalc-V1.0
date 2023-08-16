extern crate wild;

use std::path::Path;

fn main() {
    let args = wild::args()
        .skip(1)
        .filter(|x| !x.contains("*"));

    let mut success_count = 0;
    let mut fails_count = 0;
    let mut quiet = false;

    for a in args {
        if a == "--quiet" {
            quiet = true;
        }

        let path = Path::new(&a);

        if path.is_file() {
            match std::fs::remove_file(path) {
                Ok(()) => success_count += 1,
                Err(e) => {
                    fails_count += 1;
                    if !quiet {
                        println!("Failed: {:?}", e);
                    }
                }
            }
        } else if path.is_dir() {
            match std::fs::remove_dir_all(path) {
                Ok(()) => success_count += 1,
                Err(e) => {
                    fails_count += 1;
                    if !quiet {
                        println!("Failed: {:?}", e);
                    }
                }
            }
        }
    }

    if !quiet {
        println!("Successfully deleted: {success_count}. Failed: {fails_count}.");
    }
}
