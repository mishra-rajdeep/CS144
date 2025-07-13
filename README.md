# CS144 Self-Study: Networking Systems

This repository contains my solutions for the assignments from **Stanford CS144: Computer Networking** (Winter 2025) which I self-studied during Summer 2025. The course gradually builds a simplified TCP/IP stack from scratch, providing a deep understanding of how networking works under the hood.

---

## 📌 Overview of Assignments

### Assignment 0: ByteStream

Implemented a finite, flow-controlled byte stream abstraction:

- Supports writing from an input end and reading from an output end.
- Flow-controlled: enforces a strict `capacity` limit (number of bytes held in memory).
- Handles end-of-input (EOF) signaling.
- Can carry streams of arbitrary length even with tiny memory capacity.


---

### Assignment 1: Stream Reassembler

Constructed a `Reassembler`:

- Accepts out-of-order substrings with indices.
- Reassembles them into the original byte stream in order.
- Writes to the same `ByteStream` built in Assignment 0.
  

---

### Assignment 2: TCP Receiver

Built the `TCPReceiver`:

- Translates received segments into reassembled streams.
- Tracks:
  - **acknowledgment number (ackno)**: first missing byte index.
  - **window size**: how much more data the receiver can accept.
- Integrates the TCP view of sequencing with our previous byte stream and reassembler.


---

### Assignment 3: TCP Sender

Implemented the `TCPSender`:

- Reads from the `ByteStream` and generates outgoing segments.
- Manages:
  - Receiver window (acknowledgment and size).
  - Sending new data (including SYN/FIN).
  - Retransmitting unacknowledged ("outstanding") segments.


---

### Assignment 5: Network Interface (ARP Layer)

Built the `NetworkInterface` to support Ethernet + ARP:

- Maintains a cache from IP → MAC (ARP cache).
- Sends ARP requests when the MAC of the next hop is unknown.
- Queues outgoing datagrams until ARP resolution completes.
- Responds to ARP requests and updates cache with ARP replies.
- Cleans up expired ARP entries.


---

### Assignment 6: IP Router

Implemented a multi-interface IP router:

- Maintains a **routing table**: each rule defines a `prefix`, `prefix_length`, optional `next_hop`, and `interface_num`.
- Forwards datagrams based on **longest-prefix matching**.
- Drops packets if TTL reaches 0.
- Uses `NetworkInterface` to send datagrams to next hop or destination.


---

## 🛠️ Tech Stack

- **Language**: C++20
- **Build System**: CMake

---

## 📚 Reference

Course: [CS144: Computer Networking (Stanford University)](https://cs144.github.io/)  
Quarter: **Winter 2025**

---


