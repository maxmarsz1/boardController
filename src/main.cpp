/*
 * This ESP32 code is created by esp32io.com
 *
 * This ESP32 code is released in the public domain
 *
 * For more detail (instruction and wiring diagram), visit https://esp32io.com/tutorials/esp32-ws2812b-led-strip
 */

#include <Adafruit_NeoPixel.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define PIN_WS2812B 13  // The ESP32 pin GPIO16 connected to WS2812B
#define NUM_PIXELS 64   // The number of LEDs (pixels) on WS2812B LED strip

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

Adafruit_NeoPixel ws2812b(NUM_PIXELS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);

enum HoldState { none = 0, all, feet, start, end };

uint32_t getHoldColor(HoldState state){
  switch (state) {
    case HoldState::all:
    return ws2812b.Color(0, 127, 255);
    
    case HoldState::feet:
    return ws2812b.Color(248, 255, 0);
    
    case HoldState::start:
    return ws2812b.Color(0, 255, 0);

    case HoldState::end:
    return ws2812b.Color(187, 0, 255);

    default:
    return ws2812b.Color(0, 0, 0);
  }
}

class Board{
  private:
    int rows;
    int cols;
    HoldState *boardState;

  public:
    Board(int r, int c) {
      ws2812b.begin();
      ws2812b.setBrightness(1);
      
      rows = r;
      cols = c;

      Serial.begin(115200);
      Serial.printf("Initializing boardState %dx%d\n", rows, cols);

      boardState = new HoldState [rows*cols];
      clearBoard();
    }

    void lightHold(int row, int col, HoldState state){
      int pixelId = (row * rows) + col;
      boardState[pixelId] = state;
      ws2812b.setPixelColor(pixelId, getHoldColor(state));
    }

    void debugBoard(){
      for(int i = 0; i < rows*cols; i++){
        Serial.printf("%d ", boardState[i]);
        if((i + 1) % rows == 0){
          Serial.print("\n");
        }
      }
      Serial.print("\n");
    }

    void drawBoard(){
      ws2812b.show();
    }

    void clearBoard(){
      for(int i = 0; i < rows*cols; i++){
        boardState[i] = HoldState::none;
      }
    }

};

void setup() {
  BLEDevice::init("Board67");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE
  );

  pCharacteristic->setValue("Hello World says Neil");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();


  Board board = Board(8, 8);
  board.debugBoard();
  board.lightHold(2, 2, HoldState::all);
  board.lightHold(5, 3, HoldState::feet);
  board.lightHold(7, 7, HoldState::start);
  board.lightHold(1, 6, HoldState::end);
  board.debugBoard();
  board.drawBoard();
}

void loop() {

}
