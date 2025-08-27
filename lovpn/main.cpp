#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include <thread>
#include <atomic>
#include <chrono>
#include <array>
#include <cstdio>

namespace fs = std::filesystem;

// ANSI color helpers
static const std::string RED    = "\x1b[31m";
static const std::string GREEN  = "\x1b[32m";
static const std::string YELLOW = "\x1b[33m";
static const std::string CYAN   = "\x1b[36m";
static const std::string RESET  = "\x1b[0m";

// Spinner state
std::atomic<bool> g_spinnerRunning{false};

// Shell-escape a single argument for /bin/sh -c
std::string shell_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('\'');
    for (char c : s) {
        if (c == '\'') out += "'\\''"; else out.push_back(c);
    }
    out.push_back('\'');
    return out;
}

std::string join_argv(const std::vector<std::string>& args) {
    std::string cmd;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i) cmd.push_back(' ');
        cmd += shell_escape(args[i]);
    }
    return cmd;
}

// Run a command via system(), optionally silencing output
int run_command(const std::vector<std::string>& args, bool quiet = true) {
    std::string cmd = join_argv(args);
    if (quiet) cmd += " >/dev/null 2>&1";
    return std::system(cmd.c_str());
}

// Run a command and capture stdout (stderr discarded)
std::string run_capture(const std::string& cmd) {
    std::array<char, 4096> buf{};
    std::string out;
    std::string full = cmd + " 2>/dev/null";
    FILE* pipe = popen(full.c_str(), "r");
    if (!pipe) return out;
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe)) {
        out += buf.data();
    }
    pclose(pipe);
    return out;
}

void spinner_task(const std::string& label) {
    static const char* arrows[8] = {"\xE2\x86\x91","\xE2\x86\x97","\xE2\x86\x92","\xE2\x86\x98","\xE2\x86\x93","\xE2\x86\x99","\xE2\x86\x90","\xE2\x86\x96"};
    size_t i = 0;
    while (g_spinnerRunning.load()) {
        std::cout << "\r[" << YELLOW << arrows[i % 8] << RESET << "] " << label << "... " << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ++i;
    }
    std::cout << "\r" << std::flush;
}

std::vector<std::string> get_all_vpn_names() {
    std::string out = run_capture("nmcli -t -f NAME,TYPE connection show");
    std::vector<std::string> names;
    std::istringstream iss(out);
    std::string line;
    while (std::getline(iss, line)) {
        auto pos = line.find(":vpn");
        if (pos != std::string::npos) {
            names.emplace_back(line.substr(0, pos));
        }
    }
    return names;
}

std::string import_vpn(const std::string& filePath, const std::string& username, const std::string& password) {
    fs::path p(filePath);
    std::string name = p.stem().string();

    run_command({"nmcli", "connection", "import", "type", "openvpn", "file", filePath});
    run_command({"nmcli", "connection", "modify", name, "vpn.user-name", username});
    run_command({"nmcli", "connection", "modify", name, "+vpn.data", "password-flags=0"});
    run_command({"nmcli", "connection", "modify", name, "vpn.secrets", std::string("password=") + password});

    return name;
}

void delete_vpn(const std::string& name) {
    run_command({"nmcli", "connection", "delete", name});
}

void delete_all_vpns() {
    for (const auto& n : get_all_vpn_names()) delete_vpn(n);
}

void connect_vpn(const std::string& name) {
    run_command({"nmcli", "connection", "up", name}, /*quiet=*/false);
}

void disconnect_vpn(const std::string& name) {
    run_command({"nmcli", "connection", "down", name}, /*quiet=*/false);
}

void set_autoconnect(const std::string& name, bool enabled) {
    std::string flag = enabled ? "yes" : "no";
    run_command({"nmcli", "connection", "modify", name, "connection.autoconnect", flag});
}

void print_help() {
    std::cout << CYAN << R"([OVPN-BULKER] - Manage OpenVPN Profiles via NetworkManager

Usage:
  ovpn-bulker <directory> <username> <password>  Import all .ovpn files
  ovpn-bulker list                               Show all imported VPNs
  ovpn-bulker connect <vpn-name>                 Connect to a VPN
  ovpn-bulker disconnect <vpn-name>              Disconnect a VPN
  ovpn-bulker delete-all                         Remove all VPNs
  ovpn-bulker help                               Show this message
)" << RESET;
}

void install_all(const std::string& directory, const std::string& username, const std::string& password) {
    std::vector<fs::path> ovpn_files;
    try {
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".ovpn") {
                ovpn_files.push_back(entry.path());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << RED << "[!] Failed to read directory: " << e.what() << RESET << "\n";
        return;
    }

    if (ovpn_files.empty()) {
        std::cerr << RED << "[!] No .ovpn files found in " << directory << RESET << "\n";
        return;
    }

    std::cout << "\n";
    g_spinnerRunning = true;
    std::thread spinner(spinner_task, "Importing OVPN configurations");

    for (const auto& file : ovpn_files) {
        std::string name = import_vpn(file.string(), username, password);
        set_autoconnect(name, true);
    }

    g_spinnerRunning = false;
    if (spinner.joinable()) spinner.join();

    std::cout << "[" << GREEN << "\xE2\x9C\x93" << RESET << "] Imported " << ovpn_files.size() << " VPN(s) successfully.\n\n";
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);

    auto starts_with_dash = [](const std::string& s){ return !s.empty() && s[0] == '-'; };

    if (args.size() == 3 && !starts_with_dash(args[0])) {
        const std::string& directory = args[0];
        const std::string& username  = args[1];
        const std::string& password  = args[2];
        install_all(directory, username, password);
        return 0;
    }

    if (args.empty() || args[0] == "help" || args[0] == "--help" || args[0] == "-h") {
        print_help();
        return 0;
    }

    const std::string command = args[0];

    if (command == "list") {
        auto names = get_all_vpn_names();
        std::cout << CYAN << "\n[=] VPN Profiles:" << RESET << "\n";
        for (const auto& n : names) {
            std::cout << "  - " << GREEN << n << RESET << "\n";
        }
        std::cout << "\n";
        return 0;
    }

    if (command == "delete-all") {
        std::cout << YELLOW << "Are you sure you want to delete all VPNs? (y/N): " << RESET;
        std::string resp; std::getline(std::cin, resp);
        if (!resp.empty() && (resp == "y" || resp == "Y")) {
            g_spinnerRunning = true;
            std::thread spinner(spinner_task, "Deleting OVPN configurations");
            delete_all_vpns();
            g_spinnerRunning = false;
            if (spinner.joinable()) spinner.join();
            std::cout << "[" << GREEN << "\xE2\x9C\x93" << RESET << "] All VPNs deleted.\n\n";
        } else {
            std::cout << "Aborted.\n";
        }
        return 0;
    }

    if (command == "connect" && args.size() == 2) {
        connect_vpn(args[1]);
        return 0;
    }

    if (command == "disconnect" && args.size() == 2) {
        disconnect_vpn(args[1]);
        return 0;
    }

    std::cerr << RED << "[!] Invalid arguments." << RESET << "\n\n";
    print_help();
    return 1;
}
