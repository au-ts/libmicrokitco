A simple Redis-like example system that implements an integer-string map as an in-memory cache and uses `libmicrokitco` (hence "Codis") on the server PD to manage multiple client connections and on the clients PDs for blocking I/O while only using 1 seL4 kernel thread per PD.

In it's current state, the cache have 255 "buckets" that are able to store a 4K string per bucket. Communications are done asynchronously by signalling the server channel and arguments are passed by a shared buffer between the client and server. The clients then blocks waiting for data to come back from the server to demo the library.

Modify the SDK paths in `run_qemu.sh` as appropriate for your system. Then execute the script for running options.
