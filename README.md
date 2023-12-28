# FileMailman
This is a prototype project for file transferring based on UDP.
# Build
This project is built with CMake(I use MinGW Makefiles and ninja to build), and is designed for Windows.
# Usage
Only command line is supported. The Sender should be launched first, then the Receiver.
- Sender
    ```
    .\filemailman.exe send [port] [file_path]
    ```
- Receiver
    ```
    .\filemailman.exe receive [ip] [port]
    ```
# To Do
- [ ] refine log system
- [ ] make it multithreaded
- [ ] add gui
- [ ] refine rudp implementation