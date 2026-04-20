#include <WiFi.h>
#include <WebServer.h>
#include "SPIFFS.h"

const char* ssid = "ESP32_UPLOAD";
const char* password = "12345678";

WebServer server(80);
File uploadFile;

/* ================= SPIFFS ================= */
void initFS()
{
  if(!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS FAIL");
    while(1);
  }
}

/* ================= HOME PAGE ================= */
void handleRoot()
{
  server.send(200,"text/html",
  "<h2>Upload ANY file</h2>"
  "<form method='POST' action='/upload' enctype='multipart/form-data'>"
  "<input type='file' name='data'>"
  "<input type='submit' value='Upload'>"
  "</form>"
  "<br><a href='/list'>List files</a>");
}

/* ================= UPLOAD ================= */
void handleUpload()
{
  HTTPUpload& upload = server.upload();

  if(upload.status == UPLOAD_FILE_START)
  {
    String filename = "/" + upload.filename;

    Serial.print("Uploading: ");
    Serial.println(filename);

    uploadFile = SPIFFS.open(filename, FILE_WRITE);
  }
  else if(upload.status == UPLOAD_FILE_WRITE)
  {
    if(uploadFile)
      uploadFile.write(upload.buf, upload.currentSize);
  }
  else if(upload.status == UPLOAD_FILE_END)
  {
    if(uploadFile)
      uploadFile.close();

    Serial.println("Upload DONE");
  }
}

/* ================= LIST FILE ================= */
void handleList()
{
  File root = SPIFFS.open("/");

  String html="<h3>Files:</h3>";

  File file = root.openNextFile();

  while(file)
  {
    html += String(file.name());
    html += "<br>";
    file = root.openNextFile();
  }

  server.send(200,"text/html",html);
}

/* ================= SETUP ================= */
void setup()
{
  Serial.begin(115200);

  initFS();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid,password);

  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/",handleRoot);

  server.on("/upload",
            HTTP_POST,
            [](){ server.send(200,"text/plain","Upload Success"); },
            handleUpload);

  server.on("/list",handleList);

  server.begin();
}

/* ================= LOOP ================= */
void loop()
{
  server.handleClient();
}