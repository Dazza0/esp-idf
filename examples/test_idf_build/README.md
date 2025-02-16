# Notes

- IDF as external library build command

```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$IDF_PATH/tools/cmake/toolchain-esp32.cmake -DTARGET=esp32 -GNinja
```
