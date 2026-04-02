#include <Adafruit_NeoPixel.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define PIN_WS2812B 13  // The ESP32 pin GPIO16 connected to WS2812B

#define ROWS 8
#define COLS 8

#define DEVICE_NAME "Board67"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define LAYOUT_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CONTROL_CHARACTERISTIC_UUID "77fb628a-5f65-4c9d-aacc-73f499bae991"

Adafruit_NeoPixel ws2812b(ROWS* COLS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);

bool deviceConnected = false;

enum HoldState { none = 0, all, feet, start, end };

uint32_t getHoldColor(HoldState state) {
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

class Board {
 private:
  int rows;
  int cols;
  HoldState* boardState;

 public:
  Board(int r, int c) {
    ws2812b.begin();
    ws2812b.setBrightness(1);

    rows = r;
    cols = c;

    Serial.printf("Initializing boardState %dx%d\n", rows, cols);

    boardState = new HoldState[rows * cols];
    clearBoard();
  }

  void lightHold(int row, int col, HoldState state) {
    int pixelId = (row * rows) + col;
    boardState[pixelId] = state;
    ws2812b.setPixelColor(pixelId, getHoldColor(state));
  }

  void lightBoard(std::string input) {
    // accepts variable length string
    // if longer than all available pixels it doesn't care but doesn't go over
    // it each char represents hold state first char is 0x0, second 0x1, etc.
    int inputInt = 0;
    int row = 0;
    int col = 0;
    HoldState holdState;
    int loopLength = input.size() > ROWS * COLS ? ROWS * COLS : input.size();
    for (int i = 0; i < loopLength; i++) {
      inputInt = input[i] - '0';
      holdState = static_cast<HoldState>(inputInt);
      col = i % ROWS;
      if (i != 0 && i % COLS == 0) {
        row++;
      }
      lightHold(row, col, holdState);
    }
    drawBoard();
    debugBoard();
  }

  void debugBoard() {
    for (int i = 0; i < rows * cols; i++) {
      Serial.printf("%d ", boardState[i]);
      if ((i + 1) % rows == 0) {
        Serial.print("\n");
      }
    }
    Serial.print("\n");
  }

  void drawBoard() { ws2812b.show(); }

  void clearBoard() {
    for (int i = 0; i < rows * cols; i++) {
      boardState[i] = HoldState::none;
    }
  }
};

class MyServerCallback : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.print("Device connected\n");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.print("Device disconnected\n");
  }
};

class BLEConnection {
 private:
  Board* board = nullptr;

  class ControlCharacteristicCallback : public BLECharacteristicCallbacks {
   private:
    BLEConnection* pParent;

   public:
    ControlCharacteristicCallback(BLEConnection* parent) : pParent(parent) {
      Serial.print("BLEConnection address: ");
      Serial.println((uint32_t)parent, HEX);
    }
    void onWrite(BLECharacteristic* pControlChar) {
      std::string pControlChar_value_stdstr = pControlChar->getValue();
      String pControlChar_value_string =
          String(pControlChar_value_stdstr.c_str());
      Serial.println(pControlChar_value_string);
      pParent->getBoard()->lightBoard(pControlChar_value_stdstr);
    }
  };

 public:
  BLEConnection() {};
  void init(Board* newBoard) {
    board = newBoard;
    Serial.print("Board address: ");
    Serial.println((uint32_t)board, HEX);
    BLEDevice::init(DEVICE_NAME);
    BLEServer* pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallback());
    BLEService* pService = pServer->createService(SERVICE_UUID);
    BLECharacteristic* pLayoutCharacteristic = pService->createCharacteristic(
        LAYOUT_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ);
    BLEDescriptor layoutDescriptor(BLEUUID((uint16_t)0x2902));
    layoutDescriptor.setValue("Layout info");
    pLayoutCharacteristic->addDescriptor(&layoutDescriptor);

    BLECharacteristic* pControlCharacteristic = pService->createCharacteristic(
        CONTROL_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
    BLEDescriptor controlDescriptor(BLEUUID((uint16_t)0x2902));
    controlDescriptor.setValue("Control characteristic");
    pControlCharacteristic->addDescriptor(&controlDescriptor);

    pControlCharacteristic->setCallbacks(
        new ControlCharacteristicCallback(this));

    pLayoutCharacteristic->setValue("8x8");
    pService->start();

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(
        0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
  }

  Board* getBoard() { return board; }
};

Board* globalBoard = nullptr;
BLEConnection globalBle;

void setup() {
  Serial.begin(115200);
  globalBoard = new Board(ROWS, COLS);
  globalBle.init(globalBoard);

  // ble.setControlCallback(board.lightHold);
  globalBoard->debugBoard();
  globalBoard->lightHold(2, 2, HoldState::all);
  globalBoard->lightHold(6, 6, HoldState::feet);
  globalBoard->lightHold(7, 7, HoldState::start);
  globalBoard->lightHold(1, 6, HoldState::end);
  globalBoard->debugBoard();
  globalBoard->drawBoard();
}

void loop() {}
