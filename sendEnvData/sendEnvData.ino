//送信側（マスター）
#include <M5StickC.h>
#include <esp_now.h>

#include <WiFi.h>
#include <Wire.h>

#include "Adafruit_Sensor.h"

#include "SHT3X.h"   //M5Stack用環境センサユニット ver.2
#include <Adafruit_BMP280.h>

uint8_t ReceiveAddress[] = { 0x94, 0xB9, 0x7E, 0x92, 0xE7, 0xCC }; // 受信機のMACアドレスに書き換える macアドレスはM5.Lcd.println(WiFi.macAddress())で確認する
//uint8_tのため0~255までしか送れない

float tmp = 0.0;      //温度変数
float hum = 0.0;      //湿度変数
int i=0;
SHT3X sht30;        //M5Stack用環境センサユニット ver.2
Adafruit_BMP280 bme;


void onSend(const uint8_t* mac_addr, esp_now_send_status_t status)//送信が完了
{
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    //M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(5, 10);
    M5.Lcd.println(macStr); //受信機のmacアドレスを表示
    M5.Lcd.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");//送信成功or失敗を表示
}


void setup()
{
    M5.begin();
    Wire.begin();               // I2Cを初期化する
    M5.Lcd.setRotation(1);  //LCDを横向きに
    M5.Lcd.fillScreen(BLACK);//LCD黒で塗りつぶし
    M5.Lcd.setRotation(1);                        //テキストサイズを1に
    M5.Lcd.setTextColor(YELLOW,BLACK);            //テキストの色を黄色に変更

    WiFi.mode(WIFI_STA);                          //wifiモードをSTA（子機）モードに変更
    WiFi.disconnect();                            //WIFIから切断

    if (esp_now_init() == ESP_OK)                 //ESPNOWの初期化成功時
    {
        Serial.println("ESPNow Init Success");  //ESPNOW初期化成功
    }

    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, ReceiveAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)  //ペア追加が失敗したとき
    {
        Serial.println("Failed to add peer");
        return;
    }


    esp_now_register_send_cb(onSend);   //送信が完了したときのイベント
}

void loop()
{

    uint8_t data[5] = { 0 ,0, 0,0 , 222 }; //送信データ
    //float data[3] = { 0 ,0, ,222 }; //送信データ

    M5.update();        //ボタン情報の更新

    if ( M5.BtnA.wasPressed() )//LCD下部のボタンが押されたとき
    {
        if(sht30.get() == 0)
        {
            tmp = sht30.cTemp;              //温度取り込み
        }
        float decimal;
        if(tmp<0)//tmpがマイナスの時
        {
            data[1] = 0;
        }
        else
        {
            data[1] = 1;
        }

        data[2] = tmp; //送信データ
        data[3] = (tmp - (int)tmp)*100;//小数点以下を入れる処理tmp//小数点以下を送る
        esp_err_t result = esp_now_send(ReceiveAddress, data, sizeof(data));//データ送信(受信側のアドレス、符号、データ、データ（小数点以下）、データサイズ),送信ステータスの獲得

        Serial.print("Send Status: ");//送信状態の表示
        switch (result)
        {
        case ESP_OK:
            M5.Lcd.println("Success");              //成功
            break;
        case ESP_ERR_ESPNOW_NOT_INIT:
            M5.Lcd.println("ESPNOW not Init.");     //ESPNOWが初期化されていない
            break;
        case ESP_ERR_ESPNOW_ARG:
            M5.Lcd.println("Invalid Argument");     //無効な引数
            break;
        case ESP_ERR_ESPNOW_INTERNAL:
            M5.Lcd.println("Internal Error");       //内部エラー
            break;
        case ESP_ERR_ESPNOW_NO_MEM:
            M5.Lcd.println("ESP_ERR_ESPNOW_NO_MEM");//メモリー不足
            break;
        case ESP_ERR_ESPNOW_NOT_FOUND:
            M5.Lcd.println("Peer not found.");      //ペアが見つからない
            break;

        default:
            M5.Lcd.println("Not sure what happened");//不明
            break;
        }
      M5.Lcd.println(tmp);
    for (int i = 0; i < 6; i++)
    {
        M5.Lcd.print(data[i]);
        M5.Lcd.print(" ");
    }
    }


    delay(1);
}
