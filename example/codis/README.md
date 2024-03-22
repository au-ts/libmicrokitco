A simple "Redis" example system that implements a subset of Redis' features and uses co-routines (hence "Codis") to manage multiple client connections while only using 1 seL4 kernel thread.

libc.a came from musllibc in LionsOS.