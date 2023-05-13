
#include <WiFi.h>
#include <WiFiClient.h>

const char* ssid = "ibrar";
const char* password = "ibrarahmad";


#include <ESPAsyncWebServer.h>
AsyncWebServer server(80);
#include "web_index.h"


#include <SPI.h>
#include <SPIFFS.h>


File file;
const char filename[] = "/recording.wav";
const int headerSize = 44;


#include <driver/i2s.h>
#define I2S_WS 33
#define I2S_SD 35
#define I2S_SCK 32
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (16)
#define I2S_READ_LEN      (16 * 1024)
#define RECORD_TIME       (20) //Seconds
#define I2S_CHANNEL_NUM   (1)
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)

void setup()
{
  Serial.begin(115200);

  
  if (String(WiFi.SSID()) != String(ssid))
  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
  }
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  
  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  {
    
    if (!SPIFFS.begin(true))
    {
      Serial.println("SPIFFS initialisation failed!");
      while (1) yield();
    }

    
    SPIFFS.remove(filename);

    
    file = SPIFFS.open(filename, FILE_WRITE);
    if (!file)
    {
      Serial.println("File is not available!");
    }

   
    byte header[headerSize];
    wavHeader(header, FLASH_RECORD_SIZE);

    file.write(header, headerSize);

  
    listSPIFFS();
  }

 
  {
    i2s_config_t i2s_config =
    {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = I2S_SAMPLE_RATE,
      .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
      .intr_alloc_flags = 0,
      .dma_buf_count = 64,
      .dma_buf_len = 1024,
      .use_apll = 1
    };

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

    const i2s_pin_config_t pin_config =
    {
      .bck_io_num = I2S_SCK,
      .ws_io_num = I2S_WS,
      .data_out_num = -1,
      .data_in_num = I2S_SD
    };

    i2s_set_pin(I2S_PORT, &pin_config);
  }

  
  server.on("/", HTTP_GET, get_html);
  server.on(filename, HTTP_GET, capture_handler1);
  server.begin();

}

void loop()
{
  
  {
    int i2s_read_len = I2S_READ_LEN;
    int flash_wr_size = 0;
    size_t bytes_read;

    char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));

    
    i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);

    Serial.println(" *** Recording Start *** ");
    while (flash_wr_size < FLASH_RECORD_SIZE)
    {
      
      i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
      
      
      for (int i = 0; i < bytes_read; i += 2)
      {
       
        int16_t value = ( i2s_read_buff[i + 1] << 8 ) | i2s_read_buff[i];
       
        value = value/32;
        
        i2s_read_buff[i] = value >> 8;
        i2s_read_buff[i+1] = value;
      }
      file.write((const byte*) i2s_read_buff, i2s_read_len);
      flash_wr_size += i2s_read_len;
      Serial.print(flash_wr_size * 100 / FLASH_RECORD_SIZE);
      Serial.println("%");
    }
    file.close();

    free(i2s_read_buff);
    i2s_read_buff = NULL;

    listSPIFFS();
  }
  while (1);
}

void wavHeader(byte* header, int wavSize)
{
  header[0] = 'R';
  header[1] = 'I';
  header[2] = 'F';
  header[3] = 'F';
  unsigned int fileSize = wavSize + headerSize - 8;
  header[4] = (byte)(fileSize & 0xFF);
  header[5] = (byte)((fileSize >> 8) & 0xFF);
  header[6] = (byte)((fileSize >> 16) & 0xFF);
  header[7] = (byte)((fileSize >> 24) & 0xFF);
  header[8] = 'W';
  header[9] = 'A';
  header[10] = 'V';
  header[11] = 'E';
  header[12] = 'f';
  header[13] = 'm';
  header[14] = 't';
  header[15] = ' ';
  header[16] = 0x10;  // linear PCM
  header[17] = 0x00;
  header[18] = 0x00;
  header[19] = 0x00;
  header[20] = 0x01;  // linear PCM
  header[21] = 0x00;
  header[22] = 0x01;  // 1 Channel
  header[23] = 0x00;
  header[24] = 0x80;  // sampling rate 16000
  header[25] = 0x3E;
  header[26] = 0x00;
  header[27] = 0x00;
  header[28] = 0x00;  // BYTE RATE
  header[29] = 0x7D;
  header[30] = 0x00;
  header[31] = 0x00;
  header[32] = 0x02; // BlockAlign
  header[33] = 0x00;
  header[34] = 0x10; // Bits Per Sample
  header[35] = 0x00;
  header[36] = 'd';
  header[37] = 'a';
  header[38] = 't';
  header[39] = 'a';
  header[40] = (byte)(wavSize & 0xFF);
  header[41] = (byte)((wavSize >> 8) & 0xFF);
  header[42] = (byte)((wavSize >> 16) & 0xFF);
  header[43] = (byte)((wavSize >> 24) & 0xFF);

}


void listSPIFFS(void)
{
  Serial.println(F("\r\nListing SPIFFS files:"));
  static const char line[] PROGMEM =  "=================================================";

  Serial.println(FPSTR(line));
  Serial.println(F("  File name                              Size"));
  Serial.println(FPSTR(line));

  fs::File root = SPIFFS.open("/");
 
  if (!root)
  {
    Serial.println(F("Failed to open directory"));
    return;
  }

  if (!root.isDirectory())
  {
    Serial.println(F("Not a directory"));
    return;
  }

  fs::File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("DIR : ");
      String fileName = file.name();
      Serial.print(fileName);
    }
    else
    {
      String fileName = file.name();
      Serial.print("  " + fileName);
      // File path can be 31 characters maximum in SPIFFS
      int spaces = 33 - fileName.length(); // Tabulate nicely
      if (spaces < 1) spaces = 1;
      while (spaces--) Serial.print(" ");
      String fileSize = (String) file.size();
      spaces = 10 - fileSize.length(); // Tabulate nicely
      if (spaces < 1) spaces = 1;
      while (spaces--) Serial.print(" ");
      Serial.println(fileSize + " bytes");
    }

    file = root.openNextFile();
  }

  Serial.println(FPSTR(line));
  Serial.println();
  delay(1000);
}


static void get_html(AsyncWebServerRequest *request)
{
  request->send_P(200, "text/html", index_html);
}

static bool capture_handler1(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginResponse(SPIFFS, filename, "audio/wav");
  request->send(response);
  return true;
}
