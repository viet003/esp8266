#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// Thông tin Wifi
#define WIFI_SSID "P501"
#define WIFI_PASSWORD "0984378397"

// Cấu hình dữ liệu Json
#define JSON_DOC_SIZE 2048
#define MSG_SIZE 256

// cấu hình địa chỉ Nodeserver
#define WS_HOST "192.168.51.182"
#define WS_PORT 8080

// WebSocket Client kết nối tới máy chủ trên cổng 81
WebSocketsClient webSocket;

// Gửi thông báo lỗi tới server
void sendErrorMessage(const char *error) {
  char msg[MSG_SIZE];
  sprintf(msg, "{\"action\":\"msg\",\"type\":\"error\",\"body\":\"%s\"}", error);
  webSocket.sendTXT(msg);  // Gửi tin nhắn tới máy chủ
}

// Gửi thông báo thành công tới server
void sendOkMessage(const char *type, const char *body) {
  char msg[MSG_SIZE];
  sprintf(msg, "{\"sender\":\"esp8266\",\"type\":\"%s\",\"body\":\"%s\"}", type, body);
  webSocket.sendTXT(msg);
}

// Gửi yêu cầu kiểm tra thẻ
void sendCheckMessage(const char *type, const char *body) {
  char msg[MSG_SIZE];
  sprintf(msg, "{\"sender\":\"esp8266\",\"type\":\"%s\",\"body\":{\"id\":\"%s\"}}", type, body);
  webSocket.sendTXT(msg);
}

// Xử lý thông điệp từ server
void handleMessage(uint8_t *payload) {
  StaticJsonDocument<JSON_DOC_SIZE> doc;

  DeserializationError error = deserializeJson(doc, payload);

  // Kiểm tra nếu phân tích JSON không thành công
  if (error) {
    Serial.print(F("deserializeJson() thất bại: "));
    Serial.println(error.f_str());
    sendErrorMessage(error.c_str());
    return;
  }

  if (!doc["sender"].is<const char *>()) {
    sendErrorMessage("Không biết rõ người gửi!");
    return;
  }

  // Xử lý message từ webclient
  if (strcmp(doc["sender"], "react") == 0) {
    sendOkMessage("cmd", "Thao tác thành công!");
  }

  // xử lý message từ esp8266
  if (strcmp(doc["sender"], "esp8266") == 0) {
    sendErrorMessage("Định dạng loại tin nhắn không hợp lệ");
  }

  // xử lý message từ server
  if (strcmp(doc["sender"], "server") == 0) {
    sendErrorMessage("Định dạng loại tin nhắn không hợp lệ");
  }
}

// Sự kiện WebSocket Client
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("Đã kết nối tới server WebSocket");
      sendCheckMessage("check","GHFFFD");
      break;

    case WStype_DISCONNECTED:
      Serial.println("Đã ngắt kết nối từ server WebSocket");
      break;

    case WStype_TEXT:
      Serial.printf("Tin nhắn từ server: %s\n", String((char *)payload).c_str());
      handleMessage(payload);
      break;
  }
}

void setup() {
  Serial.begin(9600);

  // Kết nối WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Đang kết nối tới WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nĐã kết nối tới WiFi");
  Serial.print("Địa chỉ IP: ");
  Serial.println(WiFi.localIP());

  // Khởi động WebSocket Client và thiết lập sự kiện
  webSocket.begin(WS_HOST, WS_PORT, "/");  // Thay đổi địa chỉ IP máy chủ và cổng nếu cần
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  // Duy trì hoạt động của WebSocket Client
  webSocket.loop();
}
