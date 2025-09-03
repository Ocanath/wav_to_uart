#pragma once

#include <cstdint>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

class Serial 
{
private:
#ifdef _WIN32
    HANDLE serial_handle;
#else
    int serial_fd;
#endif
    bool is_connected;
    unsigned long baud_rate;

    // Platform-specific connection methods
    bool connect_to_port(const char* port_name);
    void configure_port();

public:
    Serial();
    ~Serial();
    
    // Main interface methods
    bool autoconnect(unsigned long baud_rate);
    bool connect(const char* port_name, unsigned long baud_rate);
    void disconnect();
    
    // Read/Write methods
    int write(uint8_t* data, int size);
    int read(uint8_t* buffer, int buffer_size);
    
    // Status methods
    bool connected();
    unsigned long get_baud_rate();
    
    // Platform-specific port enumeration (optional)
    static const char* find_first_available_port();
}; 