#클라이언트 코드
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <BLEDevice.h>

#define WIDTH 128
#define HEIGHT 64
Adafruit_SSD1306 display(WIDTH, HEIGHT, &Wire, -1);  // OLED display

// BLE UUIDs
BLEUUID serviceUUID("91bad492-b950-4226-aa2b-4ede9fa42f59");
BLEUUID tempUUID("cba1d466-344c-4be3-ab3f-189f80dd7518");
BLEUUID humUUID("ca73b3ba-39f6-4ab3-91ae-186dc9577d99");

char temp[16] = "-", hum[16] = "-";  // Buffers for received values
bool newData = false, doConnect = false, connected = false;
BLEAddress *foundAddr;               // Found server address
BLERemoteCharacteristic *tempChar, *humChar;

// Notify callback function
void onNotify(BLERemoteCharacteristic *c, uint8_t *data, size_t len, bool) {
  char *dst = (c->getUUID() == tempUUID) ? temp : hum;
  strncpy(dst, (char*)data, len); dst[len] = 0;
  newData = true;
}

// BLE advertising scan callback
class AdvCB : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice d) override {
    if (d.haveServiceUUID() && d.isAdvertisingService(serviceUUID)) {
      foundAddr = new BLEAddress(d.getAddress());
      BLEDevice::getScan()->stop(); doConnect = true;
    }
  }
};

// Attempt to connect to server
bool connectToServer() {
  auto client = BLEDevice::createClient();
  if (!client->connect(*foundAddr)) return false;

  auto svc = client->getService(serviceUUID);
  if (!svc) return false;

  tempChar = svc->getCharacteristic(tempUUID);
  humChar  = svc->getCharacteristic(humUUID);

  if (tempChar->canNotify()) tempChar->registerForNotify(onNotify);
  if (humChar->canNotify())  humChar->registerForNotify(onNotify);

  return connected = true;
}

// Display values on OLED
void draw() {
  display.clearDisplay();
  display.setTextSize(1); display.setCursor(0,0); display.print("Temp:");
  display.setTextSize(2); display.setCursor(0,10); display.print(temp); display.print(" C");

  display.setTextSize(1); display.setCursor(0,40); display.print("Hum:");
  display.setTextSize(2); display.setCursor(0,50); display.print(hum); display.print(" %");

  display.display(); newData = false;
}

void setup() {
  Serial.begin(115200);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); display.clearDisplay();  // Initialize OLED

  BLEDevice::init("");
  auto scan = BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new AdvCB());
  scan->setInterval(1349); scan->setWindow(449); scan->setActiveScan(true);
  scan->start(5, false);  // Start advertising scan
}

void loop() {
  if (doConnect) {
    connectToServer() ? Serial.println("Connected") : Serial.println("Failed");
    doConnect = false;
  }
  if (connected && newData) draw();  // Display on OLED if new data received
}
