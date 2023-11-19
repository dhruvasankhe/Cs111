# Hash Hash Hash
This lab presents a thread-safe implementation of a hash table in C, ensuring safe concurrent operations. It consists of two versions: `hash_table_v1` and `hash_table_v2`, each employing different mutex locking strategies to manage concurrent access.

## Building
```shell
make
```

## Running
```shell
First, I ran `python -m unittest` multiple times to ensure correctness.

To run the tester:
```
./hash-table-tester -t [number_of_threads] -s [number_of_items]
```
This will test and compare the base implementation, V1, and V2, reporting missing entries and time taken for insertion.
```

## First Implementation
In the `hash_table_v1_add_entry` function, I added a global locking mechanism. A single mutex (`pthread_mutex_t mutex`) is used for the entire hash table, which is initialized in `hash_table_v1_create` and destroyed in `hash_table_v1_destroy`. This approach ensures that only one thread can add an entry at any time, maintaining data integrity during concurrent operations.

### Performance
```shell
#### Output1
- Generation: 37,707 usec
- Hash table base: 356,629 usec
  - 0 missing
- Hash table v1: 1,263,134 usec
  - 0 missing
- Hash table v2: 110,136 usec
  - 0 missing

#### Output2
- Generation: 37,257 usec
- Hash table base: 387,182 usec
  - 0 missing
- Hash table v1: 1,238,489 usec
  - 0 missing
- Hash table v2: 113,810 usec
  - 0 missing

#### Output3
- Generation: 37,998 usec
- Hash table base: 364,427 usec
  - 0 missing
- Hash table v1: 1,120,921 usec
  - 0 missing
- Hash table v2: 118,776 usec
  - 0 missing

```
### Performance Comparison Table

| Implementation | Average Time (usec) |
|----------------|---------------------|
| base           | 372,412             |
| V1             | 1,207,514           |
| V2             | 114,240             |

Version 1 is 3.24 times slower than the base version, which is due to all threads requiring access to the single mutex, which serializes the insert operations and adds the overhead of lock management.

## Second Implementation
In the `hash_table_v2_add_entry` function, I TODO

### Performance
```shell
#### Output1
- Generation: 37,707 usec
- Hash table base: 356,629 usec
  - 0 missing
- Hash table v1: 1,263,134 usec
  - 0 missing
- Hash table v2: 110,136 usec
  - 0 missing

#### Output2
- Generation: 37,257 usec
- Hash table base: 387,182 usec
  - 0 missing
- Hash table v1: 1,238,489 usec
  - 0 missing
- Hash table v2: 113,810 usec
  - 0 missing

#### Output3
- Generation: 37,998 usec
- Hash table base: 364,427 usec
  - 0 missing
- Hash table v1: 1,120,921 usec
  - 0 missing
- Hash table v2: 118,776 usec
  - 0 missing


```
### Performance Comparison Table

| Implementation | Average Time (usec) |
|----------------|---------------------|
| base           | 372,412             |
| V1             | 1,207,514           |
| V2             | 114,240             |

V2 is 3.23 times faster than the base implementation when 4 cores are used. By reducing lock contention and allowing parallel access to the hash table, V2 achieves better throughput and efficiency.

In `hash_table_v2`, I adopted a fine-grained locking strategy, where each bucket in the hash table is protected by its own mutex. This means that multiple threads can add, retrieve, or update entries in different buckets simultaneously, greatly reducing the likelihood of a bottleneck compared to the global lock approach in `hash_table_v1`. 

This approach is kind of like having a large office with multiple filing cabinets (buckets) and giving each cabinet its own lock and key (mutex). In such a scenario, multiple employees (threads) can work with different cabinets at the same time without interfering with each other, leading to a more efficient workplace.

## Cleaning up
```shell
make clean
```
