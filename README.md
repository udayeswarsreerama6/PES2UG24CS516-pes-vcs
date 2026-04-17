# PES-VCS — Version Control System from Scratch

## Objective

This project implements a simplified version control system similar to Git.  
It tracks file changes, stores snapshots efficiently, and maintains commit history.

This system demonstrates Operating System and File System concepts including:

- Content-addressable storage
- Directory tree structures
- Atomic file operations
- Index (staging area)
- Commit history chains

**Platform Used:** Ubuntu 22.04  
**Programming Language:** C  
**Compiler:** gcc  
**Library Used:** OpenSSL (SHA-256)

---

## Commands Implemented

```bash
./pes init
./pes add <file>
./pes status
./pes commit -m "<message>"
./pes log
```

---

## Project Directory Structure

```
pes-vcs/
├── object.c
├── tree.c
├── index.c
├── commit.c
├── pes.c
├── pes.h
├── tree.h
├── index.h
├── commit.h
├── Makefile
├── test_objects.c
├── test_tree.c
├── test_sequence.sh
└── README.md
```

---

## Phase 1 — Object Storage

### Implemented Functions

- `object_write()`
- `object_read()`

### Description

Phase 1 implements content-addressable storage.

Each file is:

- Hashed using SHA-256
- Stored using hash value
- Saved inside sharded directories

**Example:**

```
.pes/objects/ab/cdef123456...
```

### Concepts Used

- SHA-256 hashing
- Directory sharding
- File integrity validation
- Atomic writes

### Commands Used

```bash
make test_objects
./test_objects
```

---

## Phase 2 — Tree Objects

### Implemented Function

- `tree_from_index()`

### Description

Phase 2 creates directory tree objects from staged files.

Tree objects store:

- File name
- File mode
- Hash reference

### Concepts Used

- Tree structures
- Serialization
- Directory hierarchy

### Commands Used

```bash
make test_tree
./test_tree
```

---

## Phase 3 — Index (Staging Area)

### Implemented Functions

- `index_load()`
- `index_save()`
- `index_add()`

### Description

Phase 3 implements the staging area.

Files added using:

```bash
./pes add <file>
```

are stored inside:

```
.pes/index
```

### Concepts Used

- File metadata tracking
- Index management
- Atomic file saving

### Commands Used

```bash
./pes init

echo "hello" > file1.txt
echo "world" > file2.txt

./pes add file1.txt
./pes add file2.txt

./pes status
```

---

## Phase 4 — Commit System

### Implemented Function

- `commit_create()`

### Description

Phase 4 creates commits and maintains history.

Each commit stores:

- Tree reference
- Parent commit
- Author information
- Message
- Timestamp

### Concepts Used

- Commit chaining
- Snapshot storage
- Reference updates

### Commands Used

```bash
./pes init

echo "Hello" > hello.txt
./pes add hello.txt
./pes commit -m "Initial commit"

echo "World" >> hello.txt
./pes add hello.txt
./pes commit -m "Second commit"

echo "Bye" > bye.txt
./pes add bye.txt
./pes commit -m "Third commit"

./pes log
```

---

## Repository Structure After Initialization

```
.pes/
├── objects/
├── refs/
│   └── heads/
│       └── main
├── index
└── HEAD
```

---

## Final Integration Test

**Command:**

```bash
make test-integration
```

---

## Build Instructions

Install dependencies:

```bash
sudo apt update
sudo apt install gcc build-essential libssl-dev
```

Build project:

```bash
make
```

Clean build:

```bash
make clean
```

---

## Usage Instructions

Initialize repository:

```bash
./pes init
```

Add files:

```bash
./pes add filename.txt
```

Check status:

```bash
./pes status
```

Create commit:

```bash
./pes commit -m "message"
```

View history:

```bash
./pes log
```

---

## Filesystem Concepts Demonstrated

This project demonstrates:

- Content-addressable storage
- SHA-256 hashing
- Directory sharding
- Tree-based directory structures
- File staging
- Commit history chains
- Atomic file operations

---

## Author Information

**Name:** Your Name  
**SRN:** Your SRN

Set author:

```bash
export PES_AUTHOR="Your Name <Your SRN>"
```

---

## Conclusion

This project successfully implements a simplified version control system inspired by Git.  
All four phases were completed and verified using test programs and integration tests.
