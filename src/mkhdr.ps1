param (
    [string]$path
)

$path_h = $path + ".h"
$path_c = $path + ".c"

$define = $path.Replace('.', '_').Replace('/', '_').ToUpper()
$define += "_"

# Create the directory if it doesn't exist
$directory = Split-Path -Path $path -Parent
if ($directory -ne "" -and !(Test-Path -Path $directory)) {
    New-Item -ItemType Directory -Force -Path $directory
}

# Create the file and write to it
New-Item -ItemType File -Force -Path $path_h
New-Item -ItemType File -Force -Path $path_c
echo "#ifndef $define`n#define $define`n`n`n`n#endif //$define" > $path_h

$h_name = Split-Path $path_h -leaf
echo "#include `"$h_name`"" > $path_c