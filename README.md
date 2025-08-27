# lovpn (C++ Version)

A direct C++ port of the original Python **OVPN-Bulker** utility now called lovpn (Linux Open VPN). This tool automates the import, management, and removal of multiple OpenVPN profiles using **NetworkManager** (`nmcli`). 

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
./lovpn <directory> <username> <password>   # Import all .ovpn files
./lovpn list                                # Show imported VPNs
./lovpn connect <vpn-name>                  # Connect to a VPN
./lovpn disconnect <vpn-name>               # Disconnect a VPN
./lovpn delete-all                          # Delete all VPNs
./lovpn help                                # Show help message
```

## Example
```bash
./lovpn ~/VPNs myuser mypass
./lovpn list
./lovpn connect work-vpn
```

## Notes
- Ensure your terminal is set to **UTF-8** to properly display the spinner.
- Root or `sudo` may be required for some commands depending on your system configuration.
