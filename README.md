# os-socket-chat

Simple chat server and client programs using Linux tcp sockets.

## build

```bash
cmake -S. -Bbuild
cmake --build build
```

or, build with debug logs

```bash
cmake -S. -Bbuild -DCMAKE_C_FLAGS="-DUSE_DEBUG_LOGGING"
cmake --build build
```

## run

First, run server

```bash
./build/server
```

Then, run client on the another terminal, or another device.

```bash
./build/client <server-host>
# ./build/client localhost       same device, diffrent terminal
# ./build/client 123.45.6.7      different device
```

Type `!` if you want to exit the client program.

```bash
...
>>> !
[exited]
```

Send `SIGINT` by typing `ctrl+c` to stop the server program.
