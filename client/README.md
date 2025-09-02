# 車載データロガー

センサーで受け取った値をUARTを用いて車載サーバーへ送ります

## build

```sh
cmake -G Ninja -B build -D CMAKE_BUILD_TYPE=Release
ninja -C build
```

このコマンドで生成される以下二つのファイルをそれぞれ焼いてください。

- `build/front.uf2`
- `build/rear.uf2`

bootボタンを押したままPCに接続するとUSBマスストレージとして認識されるのでそこにUF2ファイルを入れてください。

