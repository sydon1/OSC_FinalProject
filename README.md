This is the final project made for my operating systems course. It was made after several introductory labs introducing C.
Grade: 17/20
## **Project Overview**
A multi-threaded sensor monitoring system that collects temperature data from multiple sensor nodes via TCP. The system includes a central gateway that processes incoming sensor data through three main components:
  - Connection Manager: Handles TCP connections from sensor nodes
  - Data Manager: Processes and validates sensor data, monitors temperature thresholds  
  - Storage Manager: Persists sensor data to CSV files

## Features
- Multi-threaded architecture with thread-safe shared buffer
- TCP connection management with timeout handling
- Logging system using process forking and pipes
- Uses custom circular buffer & mutex locks with condition variables for thread communication and synchronization

## Issues 
  - Issues at times when testing with many clients & nodes. I think this relates to the implementation of the shared buffer, but couldn't solve the problem.
