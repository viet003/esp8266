#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <FirebaseESP8266.h>
#include <ArduinoJson.h>

// Thông tin Wifi
#define WIFI_SSID "P501"
#define WIFI_PASSWORD "0984378397"

// Cấu hình dữ liệu Json
#define JSON_DOC_SIZE 2048
#define MSG_SIZE 256

// Thông tin Firebase
#define FIREBASE_HOST "severiot-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "AIzaSyBwprPbSzq_OJmvEnOCZJTrdGxGLovqm0I"

// Cấu hình IP tĩnh, Gateway, SubnetMarks
IPAddress local_IP(192, 168, 51, 220);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Tạo cấu trúc FirebaseConfig và FirebaseAuth
FirebaseConfig config;
FirebaseAuth auth;
FirebaseData firebaseData;

// Máy chủ WebSocket trên cổng 81
WebSocketsServer wsServer = WebSocketsServer(81);

// Gửi thông báo lỗi tới client
void sendErrorMessage(uint8_t clientID, const char *error) {
  char msg[MSG_SIZE];
  sprintf(msg, "{\"action\":\"msg\",\"type\":\"error\",\"body\":\"%s\"}", error);
  wsServer.sendTXT(clientID, msg);  // Gửi tin nhắn tới client cụ thể
}

// Gửi thông báo thành công
void sendOkMessage(uint8_t clientID) {
  wsServer.sendTXT(clientID, "{\"action\":\"msg\",\"type\":\"status\",\"body\":\"ok\"}");
}

// Sử lý thông điệp từ client
void handleMessage(uint8_t clientID, uint8_t *payload) {
  StaticJsonDocument<JSON_DOC_SIZE> doc;

  DeserializationError error = deserializeJson(doc, payload);

  // Kiểm tra nếu phân tích JSON không thành công
  if (error) {
    Serial.print(F("deserializeJson() thất bại: "));
    Serial.println(error.f_str());
    sendErrorMessage(clientID, error.c_str());
    return;
  }

  if (!doc["type"].is<const char *>()) {
    sendErrorMessage(clientID, "Định dạng loại tin nhắn không hợp lệ");
    return;
  }

  // Thêm logic xử lý khác ở đây
  if (strcmp(doc["type"], "cmd") == 0) {
    sendOkMessage(clientID);
  } else {
    sendErrorMessage(clientID, "Loại tin nhắn không được hỗ trợ");
  }
}

void onWSEvent(uint8_t clientID, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("Client %u đã kết nối\n", clientID);
      break;

    case WStype_DISCONNECTED:
      Serial.printf("Client %u đã ngắt kết nối\n", clientID);
      break;

    case WStype_TEXT:
      Serial.printf("Tin nhắn từ client %u: %s\n", clientID, String((char *)payload).c_str());
      handleMessage(clientID, payload);
      break;
  }
}

// Gửi dữ liệu JSON với ID tự động được tạo bởi Firebase
void sendVehicle(String UserId, String entryTime, String exitTime, String status) {
  FirebaseJson json;

  // Đặt giá trị JSON cho xe
  json.set("UserId", UserId);
  json.set("entry_time", entryTime);
  json.set("exit_time", exitTime);
  json.set("status", status);

  // Sử dụng push() để tạo ID tự động
  if (Firebase.push(firebaseData, "/vehicles", json)) {
    Serial.println("Dữ liệu đã được gửi thành công với ID tự động: " + firebaseData.pushName());
  } else {
    Serial.println("Lỗi gửi dữ liệu: " + firebaseData.errorReason());
  }
}

// Hàm cập nhật trạng thái của xe trong Firebase
void updateVehicleStatus(String vehicleID, String exitTime, String status) {
  FirebaseJson json;

  // Đặt giá trị JSON để cập nhật
  json.set("exit_time", exitTime);
  json.set("status", status);

  // Đường dẫn đến bản ghi của xe
  String path = "/vehicles/" + vehicleID;

  // Cập nhật dữ liệu trên Firebase
  if (Firebase.updateNode(firebaseData, path, json)) {
    Serial.println("Dữ liệu đã được cập nhật thành công.");
  } else {
    Serial.println("Lỗi cập nhật dữ liệu: " + firebaseData.errorReason());
  }
}

void setup() {
  Serial.begin(9600);

  // Cấu hình IP tĩnh cho chế độ Station Mode (STA)
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("Cấu hình IP tĩnh thất bại.");
  }
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Đang kết nối tới WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nĐã kết nối tới WiFi");
  Serial.print("Địa chỉ IP tĩnh: ");
  Serial.println(WiFi.localIP());

  // Cấu hình Firebase và khởi tạo
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.reconnectWiFi(true);


  // Khởi động Websocket Server và thiết lập sự kiện
  wsServer.begin();
  wsServer.onEvent(onWSEvent);
}

void loop() {
  // Duy trì hoạt động của máy chủ WebSocket
  wsServer.loop();
}
