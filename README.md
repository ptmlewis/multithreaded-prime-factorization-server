# ⚙️ Multithreaded Prime Factoring Server (C)

## 🧠 Overview
Concurrent **TCP client-server** system written in C.  
Each client sends an integer; the server performs 32-bit bitwise rotations and prime factorization across threads.

## ⚙️ Features
- Multi-threaded design using `pthread` and mutex synchronization  
- Handles up to 12 clients concurrently  
- Streams real-time progress and results to clients  
- Built-in test mode for concurrency stress testing

## 🚀 Compile & Run
```bash
gcc server.c -lpthread -lm -o server
gcc client.c -lpthread -o client

./server
./client
```

## 🧩 Example Output
**Client:**
```
Enter a number: 60
Rotation 1: 60 → Factors: 2 2 3 5
Rotation 2: 120 → Factors: 2 2 2 3 5
```

**Server:**
```
Client 1 connected
Slot 0 progress: 25%
Slot 0 complete in 1.83s
```

## 🧠 Skills Demonstrated
C · Multithreading · TCP/IP Networking · Synchronization · Systems Programming
