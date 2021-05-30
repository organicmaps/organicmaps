# Sandbox

This project can show how airmaps works on your computer.

## Setup

You must have [docker](https://docs.docker.com/get-docker/) and [docker-compose](https://docs.docker.com/compose/install/).

0. Change working directory:

```sh
$ cd omim/tools/python/airmaps/sandbox
```

1. Build airmaps service:

```sh
sandbox$ ./build.sh
```

2. Create storage(sandbox/storage directory):

```sh
sandbox$ ./create_storage.sh
```

Note: May be you need `sudo`, because `./create_storage.sh` to try change an owner of `sandbox/storage/tests` directory.

## Usage

### Starting

0. Change working directory:

```sh
$ cd omim/tools/python/airmaps/sandbox
```

1. Run all services:

```sh
sandbox$ docker-compose up
```

2. Open http://localhost in your browser.

Note: You can see the results of airmaps working in `sandbox/storage/tests`.

### Stopping

0. Change working directory:

```sh
$ cd omim/tools/python/airmaps/sandbox
```

1. Stop all services:
   Push Ctrl+C and

```sh
sandbox$ docker-compose down
```

### Clean

#### Clean storage and intermediate files:

0. Change working directory:

```sh
$ cd omim/tools/python/airmaps/sandbox
```

1. Clean storage and intermediate files:

```sh
sandbox$ ./clean.sh
```

#### Remove images:

0. Change working directory:

```sh
$ cd omim/tools/python/airmaps/sandbox
```

1. Remove images:

```sh
sandbox$ docker-compose rm
```
