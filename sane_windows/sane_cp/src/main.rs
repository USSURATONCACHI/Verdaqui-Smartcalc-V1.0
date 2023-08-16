use std::path::{PathBuf, Path};
use wildmatch::WildMatch;

fn read_args() -> Result<(Vec<PathBuf>, String, bool), String> {
    let mut files: Vec<String> = vec![];
    let mut forced = false;

    for arg in std::env::args().skip(1) {
        if arg == "-f" && !forced {
            forced = true
        } else {
            files.push(arg);
        }
    }

    if files.len() < 2 {
        return Err("Not enough args specified".to_string());
    }

    let last_index = files.len() - 1;
    let into = files.swap_remove(last_index);

    let mut result = vec![];
    for mut from in files {
        let start_dir = if is_absolute_path(&from) {
            let mut parts = from.split(['/',  '\\']);
            let result = PathBuf::from(format!("{}/", parts.next().unwrap()));
            from = parts.collect::<Vec<_>>().join("/");
            result
        } else {
            std::env::current_dir().unwrap()
        };
        result.extend(wildcard_to_pathes(start_dir, &from));
    }

    Ok((result, into, forced))
}

fn main() -> Result<(), ()> {
    let (pathes, to, forced) = match read_args() {
        Ok(args) => args,
        Err(error) => {
            eprintln!("Error: {}", error);
            eprintln!("Usage: sane_cp.exe [-f] <source> <destination>");
            eprintln!("\t\t Copy files from source to destination.");
            eprintln!("\t\t -f : ignore copying errors");
            eprintln!("\t\t Put '/' at the end of destination to copy into a directory, or remove it to copy into a file");
            return Err(());
        }
    };

    let to_path = PathBuf::from(&to);
    let copy_to_dir = to.ends_with("/") || to.ends_with("\\");
    // Multiple files cannot be copied into a file

    if copy_to_dir {
        let mut errors_count = 0;
        if to_path.is_file() {
            let display = to_path.display();
            panic!("Destination path {display} is a FILE, but attempting to copy to directory. Remove '/' from the end of path to copy to file. Add/remain '/' to copy to directory");
        }
        if !to_path.is_dir() {
            std::fs::create_dir_all(&to_path).expect("Failed to create directory");
        }
        for entry in pathes {
            let mut dest = to_path.clone();
            dest.push(entry.file_name().unwrap());
            if entry.is_dir() {
                let _ = copy(copy_dir_all, &entry, &dest, || errors_count += 1);
            } else if entry.is_file() {
                let _ = copy(std::fs::copy, &entry, &dest, || errors_count += 1);
            } else {
                panic!("File not found (right after being found): {}", entry.display());
            }
        }

        if errors_count == 0 || forced {
            return Ok(());
        } else {
            eprintln!("Errors: {errors_count}");
            return Err(());
        }
    } else {
        if pathes.len() == 1 {
            let only_path = pathes.into_iter().next().unwrap();
            match std::fs::copy(&only_path, &to_path) {
                Ok(_) => println!("Copied {} -> {}", only_path.display(), to_path.display()),
                Err(error) => { 
                    eprintln!("Failed to copy {} -> {} : {}", only_path.display(), to_path.display(), error.to_string());
                    return Err(()); 
                }
            }
        } else {
            eprintln!("Cannot copy multiple files into one file:");
            for path in pathes {
                eprintln!("{}", path.display());
            }
            eprintln!("Add a '/' to the end of destination to copy to DIRECTORY");
            return if forced { Ok(()) } else { Err(()) };
        }
    }

    Ok(())
}

fn copy<'a, T, E, F>(fun: F, entry: &'a PathBuf, dest: &'a PathBuf, on_err: impl FnOnce()) -> Result<(),()> 
    where F: Fn(&'a PathBuf, &'a PathBuf) -> Result<T, E>, 
        E: ToString
{
    match fun(entry, dest) {
        Ok(_) => {
            println!("Copied {} -> {}", entry.display(), dest.display());
            return Ok(());
        },
        Err(error) => { 
            on_err();
            eprintln!("Failed to copy {} -> {} : {}", entry.display(), dest.display(), error.to_string()); 
            return Err(());
        }
    }
}

fn is_absolute_path(path: &str) -> bool {
    match PathBuf::try_from(path) {
        Ok(path) => path.is_absolute(),
        Err(_) => false,
    }
}

fn wildcard_to_pathes(start_dir: PathBuf, path: &str) -> Vec<PathBuf> {
    let mut directories = vec![start_dir];
    let mut files = vec![];

    for element in path.split(['/', '\\']) {
        files.clear();
        let mut cur_dirs = vec![];
        std::mem::swap(&mut cur_dirs, &mut directories);

        'dir: for mut dir in cur_dirs {
            if !dir.is_dir() { continue; }

            match element {
                "." => directories.push(dir),
                ".." => {
                    let _ = dir.pop();
                    directories.push(dir);
                }
                element => {
                    let matchh = WildMatch::new(element);
                    let display_name = dir.display().to_string();

                    let readdir = match std::fs::read_dir(dir) {
                        Ok(r) => r,
                        Err(e) => {
                            eprintln!("Failed to read dir {display_name}: {}", e.to_string());
                            continue 'dir;
                        }
                    };

                    'entry: for entry in readdir {
                        let entry = match entry {
                            Ok(e) => e,
                            Err(error) => {
                                eprintln!("Failed to read entry: {}", error.to_string());
                                continue 'entry;
                            }
                        };
                        let path = entry.path();
                        let last_element = path.components().last().unwrap();
                        if matchh.matches(last_element.as_os_str().to_str().unwrap()) {
                            if path.is_dir() {
                                directories.push(path);
                            } else if path.is_file() {
                                files.push(path);
                            }
                        }
                        
                    }
                }
            }
        }
    }
    
    directories.extend(files);
    directories
}

fn copy_dir_all(src: impl AsRef<Path>, dst: impl AsRef<Path>) -> std::io::Result<()> {
    std::fs::create_dir_all(&dst)?;
    for entry in std::fs::read_dir(src)? {
        let entry = entry?;
        let ty = entry.file_type()?;
        if ty.is_dir() {
            copy_dir_all(entry.path(), dst.as_ref().join(entry.file_name()))?;
        } else {
            std::fs::copy(entry.path(), dst.as_ref().join(entry.file_name()))?;
        }
    }
    Ok(())
}