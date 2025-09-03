#include "serial.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#include <errno.h>
#else
#include <fcntl.h>
#include <errno.h>
#include <asm/ioctls.h>
#include <asm/termbits.h>
#include <sys/ioctl.h>
#endif

Serial::Serial() 
{
    is_connected = false;
    baud_rate = 115200;
#ifdef _WIN32
    serial_handle = INVALID_HANDLE_VALUE;
#else
    serial_fd = -1;
#endif
}

Serial::~Serial() 
{
    disconnect();
}

bool Serial::autoconnect(unsigned long baud_rate) 
{
    this->baud_rate = baud_rate;
    
#ifdef _WIN32
    // Windows: Try COM1 through COM255
    for (int i = 1; i <= 255; i++) 
    {
        char port_name[16];
        sprintf_s(port_name, "\\\\.\\COM%d", i);
        
        if (connect_to_port(port_name)) 
        {
            printf("Connected to %s successfully\n", port_name);
            return true;
        }
    }
#else
    // Linux: Try /dev/ttyUSB0 through /dev/ttyUSB255
    for (int i = 0; i < 256; i++) 
    {
        char port_name[32];
        sprintf(port_name, "/dev/ttyUSB%d", i);
        
        if (connect_to_port(port_name)) 
        {
            printf("Connected to %s successfully\n", port_name);
            return true;
        }
    }
#endif

    printf("No serial ports found\n");
    return false;
}

bool Serial::connect(const char* port_name, unsigned long baud_rate) 
{
    this->baud_rate = baud_rate;
    return connect_to_port(port_name);
}

void Serial::disconnect() 
{
    if (is_connected) 
    {
#ifdef _WIN32
        if (serial_handle != INVALID_HANDLE_VALUE) 
        {
            CloseHandle(serial_handle);
            serial_handle = INVALID_HANDLE_VALUE;
        }
#else
        if (serial_fd >= 0) 
        {
            close(serial_fd);
            serial_fd = -1;
        }
#endif
        is_connected = false;
    }
}

int Serial::write(uint8_t* data, int size) 
{
    if (!is_connected) 
    {
        return -1;
    }

#ifdef _WIN32
    DWORD bytes_written = 0;
    if (WriteFile(serial_handle, data, size, &bytes_written, NULL)) 
    {
        return (int)bytes_written;
    }
    return -1;
#else
    return ::write(serial_fd, data, size);
#endif
}

int Serial::read(uint8_t* buffer, int buffer_size) 
{
    if (!is_connected) 
    {
        return -1;
    }

#ifdef _WIN32
    DWORD bytes_read = 0;
    if (ReadFile(serial_handle, buffer, buffer_size, &bytes_read, NULL)) 
    {
        return (int)bytes_read;
    }
    return -1;
#else
    return ::read(serial_fd, buffer, buffer_size);
#endif
}

bool Serial::connected() 
{
    return is_connected;
}

unsigned long Serial::get_baud_rate() 
{
    return baud_rate;
}

bool Serial::connect_to_port(const char* port_name) 
{
#ifdef _WIN32
    serial_handle = CreateFileA(port_name, 
                               GENERIC_READ | GENERIC_WRITE, 
                               0, 0, OPEN_EXISTING, 
                               FILE_ATTRIBUTE_NORMAL, 0);
    
    if (serial_handle == INVALID_HANDLE_VALUE) 
    {
        return false;
    }
    
    configure_port();
    is_connected = true;
    return true;
#else
    serial_fd = open(port_name, O_RDWR);
    if (serial_fd < 0) 
    {
        return false;
    }
    
    configure_port();
    is_connected = true;
    return true;
#endif
}

void Serial::configure_port() 
{
#ifdef _WIN32
    DCB serial_params = { 0 };
    serial_params.DCBlength = sizeof(serial_params);
    serial_params.BaudRate = baud_rate;
    serial_params.ByteSize = DATABITS_8;
    serial_params.StopBits = ONESTOPBIT;
    serial_params.Parity = PARITY_NONE;
    
    if (SetCommState(serial_handle, &serial_params)) 
    {
        COMMTIMEOUTS timeouts = { 0 };
        timeouts.ReadIntervalTimeout = MAXDWORD;
        timeouts.ReadTotalTimeoutConstant = 0;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 0;
        timeouts.WriteTotalTimeoutMultiplier = 0;
        SetCommTimeouts(serial_handle, &timeouts);
    }
#else
    struct termios2 tty;
    ioctl(serial_fd, TCGETS2, &tty);
    
    // Configure the port
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    
    tty.c_oflag = 0;
    tty.c_oflag &= ~OPOST;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;
    
    // Custom baud rate
    tty.c_cflag &= ~CBAUD;
    tty.c_cflag |= CBAUDEX;
    tty.c_ispeed = baud_rate;
    tty.c_ospeed = baud_rate;
    
    // Timeout = 0
    tty.c_cc[VTIME] = 0;
    tty.c_cc[VMIN] = 0;
    
    ioctl(serial_fd, TCSETS2, &tty);
#endif
}

const char* Serial::find_first_available_port() 
{
    static char port_name[32];
    
#ifdef _WIN32
    for (int i = 1; i <= 255; i++) 
    {
        sprintf_s(port_name, "\\\\.\\COM%d", i);
        
        HANDLE test_handle = CreateFileA(port_name, 
                                        GENERIC_READ | GENERIC_WRITE, 
                                        0, 0, OPEN_EXISTING, 
                                        FILE_ATTRIBUTE_NORMAL, 0);
        if (test_handle != INVALID_HANDLE_VALUE) 
        {
            CloseHandle(test_handle);
            return port_name;
        }
    }
#else
    for (int i = 0; i < 256; i++) 
    {
        sprintf(port_name, "/dev/ttyUSB%d", i);
        
        int test_fd = open(port_name, O_RDWR);
        if (test_fd >= 0) 
        {
            close(test_fd);
            return port_name;
        }
    }
#endif
    return "";
} 