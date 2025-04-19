# Bitmap with roaring

## Build

This project uses vcpkg.

```bash
cd build
cmake --build .
```

## Test

```bash
ctest --output-on-failure
```

## Usage

```bash
tagsfor <doc_id>
query <tag1> <tag2> ... <OPERATION: AND, OR>
quit
```