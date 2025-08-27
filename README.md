# OVPN-Bulker (C++ Version)

A direct C++ port of the original Python **OVPN-Bulker** utility. This tool automates the import, management, and removal of multiple OpenVPN profiles using **NetworkManager** (`nmcli`). 

## Features
- Bulk import all `.ovpn` configuration files from a directory
- Automatically apply username and password to imported VPNs
- Enable autoconnect on imported profiles
- List all configured VPN profiles
- Connect and disconnect VPNs by name
- Delete all VPN profiles with confirmation
- Colored CLI output for better readability

## Requirements
- Linux system with **NetworkManager** and `nmcli`

## Build
```bash
g++ -std=c++17 -pthread ovpn_bulker.cpp -o ovpn-bulker
```

## Usage
```bash
./ovpn-bulker <directory> <username> <password>   # Import all .ovpn files
./ovpn-bulker list                                # Show imported VPNs
./ovpn-bulker connect <vpn-name>                  # Connect to a VPN
./ovpn-bulker disconnect <vpn-name>               # Disconnect a VPN
./ovpn-bulker delete-all                          # Delete all VPNs
./ovpn-bulker help                                # Show help message
```

## Example
```bash
./ovpn-bulker ~/VPNs myuser mypass
./ovpn-bulker list
./ovpn-bulker connect work-vpn
```

## Notes
- Ensure your terminal is set to **UTF-8** to properly display the spinner.
- Root or `sudo` may be required for some commands depending on your system configuration.
