//データ受信側（スレイブ）
#include <M5StickC.h>
#include <esp_now.h>
#include <WiFi.h>

#include "Ambient.h"    //Ambient

const char* ssid     = "test.lab";//WIFI名
const char* password = "akatuki00";//wifiパスワード
unsigned int channelId = 38698; // AmbientチャネルID
const char* writeKey = "bcc550e6d9ce9eba"; // Ambientライトキー

bool ledState = false;
float vbat = 0;//本体電圧

float Data[10];
float env = 0;

WiFiClient client;
Ambient ambient;

int RNo;//送信機番号
void toggleLed()    //LEDの状態切り替え関数
{
    if (ledState)
    {
        ledState = false;   //LEDステータスをfalseに
       digitalWrite(GPIO_NUM_10, LOW); //LEDON
    }
    else
    {
        ledState = true;    //LEDステータスをtrueに
        digitalWrite(GPIO_NUM_10, HIGH); //LEDOFF
    }
}

void onReceive(const uint8_t* mac_addr, const uint8_t* ReceiveData, int ReceiveData_len)//受信アドレス 受信データ（配列数） 受信データ数(配列数)
{
    M5.Lcd.fillScreen(BLACK);//LCDを黒で塗りつぶし
    m5.Lcd.setCursor(2,0);//LCDのカーソルを移動
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);  //配列macStrにmac_addrのデータを格納
    Serial.println();

    M5.lcd.setCursor(0,0);//LCDのカーソルを移動
    M5.Lcd.printf("Last Packet Recv from: %s\n", macStr);   //最後のパケット受信元を表示
    M5.Lcd.printf("Last Packet Recv data(%d): ", ReceiveData_len); //最後の受信データ数を表示

    if (ReceiveData[0] <= 9)//機器判別番号が10（所定数）以下
    {
        
        RNo = ReceiveData[0];//送信機番号
        if(ReceiveData[1]==0)//符号データが0（－）の時
        {
            env = ((float)ReceiveData[2]+((float)ReceiveData[3]/100))*(-1);
            
        }
        else
        {
            env = (float)ReceiveData[2]+((float)ReceiveData[3]*(float)0.01);
            Data[RNo]=(float)env;
        }
     }
    
    for (int i = 0; i < ReceiveData_len; i++)
    {
        M5.Lcd.print(ReceiveData[i]); //受信データの表示
        M5.Lcd.print(" ");
        if (ReceiveData[i] == 222)//現状data[1]までは他のデータのため2からに
        {
            toggleLed();//LED状態切り替え
        }
    }

    m5.Lcd.setCursor(2,40);//LCD
    //受信データを表示
    M5.Lcd.print("Recv data: \n"); 
    for (int i = 0; i < 10; i++)
    {
        M5.Lcd.print(Data[i]);
        M5.Lcd.print(" ");
    }

}

void setup()
{
    M5.begin();
    Serial.begin(115200);
    M5.Lcd.setRotation(1);                        //テキストサイズを1に
    M5.Lcd.setTextColor(YELLOW,BLACK);            //テキストの色を黄色に変更
    pinMode(GPIO_NUM_10, OUTPUT);
    M5.Lcd.println(WiFi.macAddress()); // このアドレスを送信側へ登録します

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    if (esp_now_init() == ESP_OK)
    {
        m5.Lcd.println("ESP-Now Init Success");
    }
    esp_now_register_recv_cb(onReceive);//esp_now_register_recv_cb(受信時に実行する関数);
    digitalWrite(GPIO_NUM_10, HIGH); //LEDOFF
}

void ambient_access(void)       //アンビエントアクセス関数
{
    vbat = M5.Axp.GetBatVoltage();//バッテリ電圧取得
    ambient.begin(channelId, writeKey, &client); //チャネルIDとライトキーを指定してAmbientの初期化

    //Ambientに送信するデータをセット
    for(int i=1;i<7;i++)
    {
        ambient.set(i, Data[i-1]);
    }

    ambient.send();                   //ambientにデータを送信
}

void wifi_conect(void)  //wifi接続関数
{
    WiFi.begin(ssid, password);           //  Wi-Fi APに接続

    while (WiFi.status() != WL_CONNECTED) //  Wi-Fi AP接続待ち
    {
       delay(500);
    }

    Serial.print("WiFi connected\r\nIP address: ");
    Serial.println(WiFi.localIP());
}


void loop()
{
    M5.update();        //ボタン情報の更新
    if ( M5.BtnA.wasPressed() )//LCD下部のボタンが押されたとき
    {
      wifi_conect();//wifiに接続
      ambient_access();//ambientにデータ送信
      WiFi.disconnect();  //wifi切断
      delay(100);
    }
    delay(1);//0.1秒待機
}
