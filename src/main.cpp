#include <Arduino.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>
#include <ESP8266WiFi.h>

#define RELAYPIN_1 D1

#define WIFI_SSID  ""
#define WIFI_PASS  ""
#define APP_KEY    ""    // Deve parecer com "de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx"
#define APP_SECRET "" // Deve parecer como "5f36xxxx-x3x7-4x3x-xexe-e86724a9xxxx-4c4axxxx-3x3x-x5xe-x9x3-333d65xxxxxx"
#define INTERVALO_ENVIO_THINGSPEAK 3000 // Intervalo entre envios de dados ao ThingSpeak (em ms)

float valor_anterior = 0;

struct RelayInfo {
  String deviceId;
  String name;
  int pin;
};

std::vector<RelayInfo> relays = {
    {"", "Relay 1", RELAYPIN_1}};      // deviceId do relay 1 deve parecer 635bexxxxxxxxxxxxxxxxxxx
                                                                           
// constantes e variáveis globais 
char endereco_api_thingspeak[] = "api.thingspeak.com";
String chave_escrita_thingspeak = "";  // Coloque aqui sua chave de escrita do seu canal
unsigned long last_connection_time;
WiFiClient client;

/*
* Implementações
*/
 
/* Função: envia informações ao ThingSpeak
* Parâmetros: String com a informação a ser enviada
*/
void envia_informacoes_thingspeak(String string_dados)
{
    if (client.connect(endereco_api_thingspeak, 80))
    {
        // faz a requisição HTTP ao ThingSpeak
        client.print("POST /update HTTP/1.1\n");
        client.print("Host: api.thingspeak.com\n");
        client.print("Connection: close\n");
        client.print("X-THINGSPEAKAPIKEY: "+chave_escrita_thingspeak+"\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: ");
        client.print(string_dados.length());
        client.print("\n\n");
        client.print(string_dados);
         
        last_connection_time = millis();
        Serial.println("- Informações enviadas ao ThingSpeak!");
    }
}

bool onPowerState(const String &deviceId, bool &state) {
  for (auto &relay : relays) {                                                            // para cada configuração de relay
    if (deviceId == relay.deviceId) {                                                       // check se o deviceId bate
      Serial.printf("Device %s turned %s\r\n", relay.name.c_str(), state ? "on" : "off");     // printa o nome do relay e o estado no terminal
      digitalWrite(relay.pin, state);                                                         // set estado para digital pin / gpio
      return true;                                                                            // retorna com sucesso true
    }
  }
  return false; // se nenhuma configuração do relay for encontrada, retornar false
}

// Função para setar os pinos do relay como saida
void setupRelayPins() {
  for (auto &relay : relays) {    // para cara configuração de relay
    pinMode(relay.pin, OUTPUT);     // set pinMode para OUTPUT
  }
}

// Função: Configura WiFi
void setupWiFi() {
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);             // iniciando conexão com a rede

  while (WiFi.status() != WL_CONNECTED) {       // se não conectado
    Serial.printf(".");                         //
    delay(250);                                 // delay até nova tentativa de conexão
  }
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
}

void setupSinricPro() {
  for (auto &relay : relays) {                             // para cada configuração de relay
    SinricProSwitch &mySwitch = SinricPro[relay.deviceId];   // cria um novo dispositivo com deviceId para relay configuração
    mySwitch.onPowerState(onPowerState);                     // atrela onPowerState callback para o novo dispositivo
  }

  SinricPro.onConnected([]() { Serial.printf("Connected to SinricPro\r\n"); });           // quando conectar
  SinricPro.onDisconnected([]() { Serial.printf("Disconnected from SinricPro\r\n"); });   // quando desconectar

  SinricPro.begin(APP_KEY, APP_SECRET);       // inicio conexão com SinsicPro
}

void setup() {
  Serial.begin(115200);       // velocidade monitor serial
  last_connection_time = 0;

  setupRelayPins();           // setup para definir pins como OUTPUT
  setupWiFi();                // setup conexão WiFi
  setupSinricPro();           // setup conexão SinricPro
}

//loop principal
void loop() {
  SinricPro.handle();         // mantem comunicação com SinricPro

  char fields_a_serem_enviados[100] = {0};
  float Ligado_Desligado = digitalRead(RELAYPIN_1);
  // Verifica se é o momento de enviar dados para o ThingSpeak
  if( millis() - last_connection_time > INTERVALO_ENVIO_THINGSPEAK )
  {
    if(Ligado_Desligado != valor_anterior && Ligado_Desligado != NULL){
      sprintf(fields_a_serem_enviados,"field1=%.1f", Ligado_Desligado);
      envia_informacoes_thingspeak(fields_a_serem_enviados);
      valor_anterior = Ligado_Desligado;
    }
  }
}
