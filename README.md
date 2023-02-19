# bangkiddOS

## Made by

| Name                           |   Nim    |
| ------------------------------ | :------: |
| Muhammad Bangkit Dwi Cahyono   | 13521055 |
| Louis Caesa Kesuma             | 13521069 |
| Bagas Aryo Seto                | 13521081 |
| Addin Munawwar Yusuf           | 13521085 |
| Aulia Mey Diva Annandya        | 13521103 |

## Features
1. Print `Hai!` to screen
2. Enter protected mode

## Technologies Used
1. [Window Subsytem for Linux](https://docs.microsoft.com/en-us/windows/wsl/install)
2. [Ubuntu 20.04 LTS](https://releases.ubuntu.com/20.04/)
3. [Nasm](https://www.nasm.us/)
4. [Qemu](https://www.qemu.org/docs/master/system/target-i386.html)
5. [genisoimage](https://linux.die.net/man/1/genisoimage)
6. [GNU Make](https://www.gnu.org/software/make/)
7. C

## Setup
1. Install all of the requirements

```
sudo apt update
sudo apt install gcc nasm make qemu-system-x86 genisoimage
```

2. Run using WSL2
3. Clone this repository
4. run `make build`
5. `cd bin/` and type `qemu-system-i386 -s -cdrom OS2023.iso`

## References
1. [Intel Manual](https://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-vol-3a-part-1-manual.html.html)
2. [littleosbook](https://littleosbook.github.io)
3. [wikiosdev](https://wiki.osdev.org/)

## Milestone
- [x] Milestone1
- [ ] MilestoneX
