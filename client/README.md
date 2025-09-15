# 車載データロガー

二つのマイコンを用いてセンサーの値を読み、車載データサーバへ送る

## build

[pico-sdk](https://github.com/raspberrypi/pico-sdk)に依存しているので事前にインストールしておくこと。

```sh
cmake -G Ninja -B build -D CMAKE_BUILD_TYPE=Release
ninja -C build
```

## flash

以下二つのファイルをフラッシュする。  
Bootボタンを押しながらPicoを接続するとUSBマスストレージとして認識されるので以下のファイルを入れる。

- `build/front.uf2`
- `build/rear.uf2`

## debug

OpenOCDとSWD, GDBを使ってデバッグをすることができる。  
Raspberry Pi Debug Probeを使うと簡単に接続できる。  
OpenOCDは以下のコマンドで起動できる。

```sh
openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000"
```

起動に失敗する場合は、Pico本体に電源が供給されていることを確認する。

## develop

フォーマッタの設定がしてある。  
以下のコードでフォーマットができる。  
cmake-formatはpyyamlのインストールも必須。

```sh
clang-format -i --verbose src/**/*.{c,cpp} include/**/*.{h,hpp}
cmake-format -i CMakeLists.txt
```

## ファイル構成

```
client
├── include
│   ├── bme280.h
│   ├── bno055.h
│   ├── json.hpp
│   ├── lwipopts.h
│   ├── mcp3204.h
│   ├── mcp3208.h
│   ├── meter.hpp
│   ├── shift_out.h
│   └── uart_tx.h
├── libs
│   └── cjson
│       ├── cJSON.c
│       └── cJSON.h
└── src
    ├── bme280.c
    ├── bno055.c
    ├── front.cpp
    ├── main.cpp
    ├── mcp3204.c
    ├── mcp3208.c
    ├── meter.cpp
    ├── rear.cpp
    ├── shift_out.c
    ├── shift_out.pio
    ├── uart_tx.c
    └── uart_tx.pio
```

### bme280

以下のファイルが該当

- `include/bme280.h`
- `src/bme280.c`

BOSCHのBME280という気温、気圧、湿度を測るセンサのためのドライバ。  
タッパー内の値を測定しても仕方ないということで使用中止。

### bno055

以下のファイルが該当

- `include/bno055.h`
- `src/bno055.c`

BOSCHのBNO055という9軸フュージョンセンサのためのドライバ。  
おそらく振動により破壊されたので使用中止。

### mcp3204/mcp3208

以下のファイルが該当

- `include/mcp3204.h`
- `include/mcp3208.h`
- `src/mcp3204.c`
- `src/mcp3208.c`

MicrochipのMCP3204/MCP3208という12bitA/Dコンバータのためのドライバ。

### shift_out

以下のファイルが該当

- `include/shift_out.h`
- `src/shift_out.pio`
- `src/shift_out.c`

74HC595などのシフトレジスタのためのドライバ。  
PIOを用いているので使用に注意すること。

### uart_tx

以下のファイルが該当

- `include/uart_tx.h`
- `src/uart_tx.pio`
- `src/uart_tx.c`

RP2040内臓のUARTデバイスを用いず、PIOを利用しUART出力を出すドライバ。  
uart1を出力デバイスにしたため、使用を放棄。

### json

以下のファイルが該当

- `include/json.hpp`
- `libs/cjson/cJSON.c`
- `libs/cjson/cJSON.h`

cJSONを用いたJSONのパース、シリアライズ。  
`json.hpp`はメインループ内で扱いやすいようにcjsonをラップしたもの。

### meter

以下のファイルが該当

- `include/meter.hpp`
- `src/meter.cpp`

ステアリングのメータに関するコード。  
UARTに流すデータからギアポジションなどを拾ったり、シフトレジスタで送るデータを組み立てたりしている。

### front

以下のファイルが該当

- `src/front.cpp`

フロントのマイコンのメインコード。  
以下のことを行っている。  

- センサーから値を取得する。
- リアから来たデータを受け取る。
- 車載データベースへ上記二つのデータを渡す。
- ステアリングのメーターへデータを流す。

### rear

以下のファイルが該当

- `src/rear.cpp`

リアのマイコンのメインコード。  
センサーの値を取得し、フロントへ流している。

### 旧メインコード

以下のファイルが該当

- `include/lwipopts.h`
- `src/main.cpp`

最初期にマイコン一つでロガーを構成しようとしていた時のメインコード。  
Raspberry Pi Pico Wの使用を前提としている。  
構成変更に伴い放棄。

