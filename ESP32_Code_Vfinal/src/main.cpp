#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>


// Definição das credenciais da rede Wi-Fi
const char* ssid = "NomeDaRede";
const char* password = "SenhaDaRede";


// Definição do endereço e porta do servidor MQTT
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;


// Definição dos pinos utilizados


// Módulo Conversor tensão para corrente
const int potPin = 39; // Pino analógico conectado ao potenciômetro
const int modulo1 = 26; // Pino DAC conectado ao módulo de conversão de tensão para corrente


// Módulo Conversor corrente para tensão 
const int modulo2 = 32; // Pino analógico conectado ao módulo de conversão de corrente para tensão


// Módulo amplificador 
const int onda = 25;  // Pino DAC conectado ao módulo amplificador LM358


// Módulo MOSFET PWM
const int output_pin = 14; // Pino PWM da ESP32
const int potentiometer_pin = 34; // Pino do potenciômetro
const int freq = 5000; // Frequência PWM definida para 5000Hz


// Módulo SSR
const int pinRele = 5; // Pino de controle do relé


// Tabela de formas de onda
#define Num_Samples  112  // Número de amostras por forma de onda
#define MaxWaveTypes 4    // Total de tipos de formas de onda
byte wave_type = 0;       // Tipo de onda inicial (senoidal)
int i = 0;                // Índice para percorrer a tabela de formas de onda


// Tabela de amostras para diferentes formas de onda
static byte WaveFormTable[MaxWaveTypes][Num_Samples] = {
   // Onda senoidal
   { 
    0x80, 0x83, 0x87, 0x8A, 0x8E, 0x91, 0x95, 0x98, 0x9B, 0x9E, 0xA2, 0xA5, 0xA7, 0xAA, 0xAD, 0xAF,
    0xB2, 0xB4, 0xB6, 0xB8, 0xB9, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xBF, 0xBF, 0xC0, 0xBF, 0xBF, 0xBF,
    0xBE, 0xBD, 0xBC, 0xBB, 0xB9, 0xB8, 0xB6, 0xB4, 0xB2, 0xAF, 0xAD, 0xAA, 0xA7, 0xA5, 0xA2, 0x9E,
    0x9B, 0x98, 0x95, 0x91, 0x8E, 0x8A, 0x87, 0x83, 0x80, 0x7C, 0x78, 0x75, 0x71, 0x6E, 0x6A, 0x67,
    0x64, 0x61, 0x5D, 0x5A, 0x58, 0x55, 0x52, 0x50, 0x4D, 0x4B, 0x49, 0x47, 0x46, 0x44, 0x43, 0x42,
    0x41, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x41, 0x42, 0x43, 0x44, 0x46, 0x47, 0x49, 0x4B,
    0x4D, 0x50, 0x52, 0x55, 0x58, 0x5A, 0x5D, 0x61, 0x64, 0x67, 0x6A, 0x6E, 0x71, 0x75, 0x78, 0x7C
   },
   // Onda triangular
   {
     0x80, 0x84, 0x89, 0x8D, 0x92, 0x96, 0x9B, 0x9F, 0xA4, 0xA8, 0xAD, 0xB2, 0xB6, 0xBB, 0xBF, 0xC4,
     0xC8, 0xCD, 0xD1, 0xD6, 0xDB, 0xDF, 0xE4, 0xE8, 0xED, 0xF1, 0xF6, 0xFA, 0xFF, 0xFA, 0xF6, 0xF1,
     0xED, 0xE8, 0xE4, 0xDF, 0xDB, 0xD6, 0xD1, 0xCD, 0xC8, 0xC4, 0xBF, 0xBB, 0xB6, 0xB2, 0xAD, 0xA8,
     0xA4, 0x9F, 0x9B, 0x96, 0x92, 0x8D, 0x89, 0x84, 0x7F, 0x7B, 0x76, 0x72, 0x6D, 0x69, 0x64, 0x60,
     0x5B, 0x57, 0x52, 0x4D, 0x49, 0x44, 0x40, 0x3B, 0x37, 0x32, 0x2E, 0x29, 0x24, 0x20, 0x1B, 0x17,
     0x12, 0x0E, 0x09, 0x05, 0x00, 0x05, 0x09, 0x0E, 0x12, 0x17, 0x1B, 0x20, 0x24, 0x29, 0x2E, 0x32,
     0x37, 0x3B, 0x40, 0x44, 0x49, 0x4D, 0x52, 0x57, 0x5B, 0x60, 0x64, 0x69, 0x6D, 0x72, 0x76, 0x7B
  },


   // Onda dente de serra
   {
     0x00, 0x02, 0x04, 0x06, 0x09, 0x0B, 0x0D, 0x10, 0x12, 0x14, 0x16, 0x19, 0x1B, 0x1D, 0x20, 0x22,
     0x24, 0x27, 0x29, 0x2B, 0x2D, 0x30, 0x32, 0x34, 0x37, 0x39, 0x3B, 0x3E, 0x40, 0x42, 0x44, 0x47,
     0x49, 0x4B, 0x4E, 0x50, 0x52, 0x54, 0x57, 0x59, 0x5B, 0x5E, 0x60, 0x62, 0x65, 0x67, 0x69, 0x6B, 
     0x6E, 0x70, 0x72, 0x75, 0x77, 0x79, 0x7C, 0x7E, 0x80, 0x82, 0x85, 0x87, 0x89, 0x8C, 0x8E, 0x90,
     0x93, 0x95, 0x97, 0x99, 0x9C, 0x9E, 0xA0, 0xA3, 0xA5, 0xA7, 0xA9, 0xAC, 0xAE, 0xB0, 0xB3, 0xB5,
     0xB7, 0xBA, 0xBC, 0xBE, 0xC0, 0xC3, 0xC5, 0xC7, 0xCA, 0xCC, 0xCE, 0xD1, 0xD3, 0xD5, 0xD7, 0xDA,
     0xDC, 0xDE, 0xE1, 0xE3, 0xE5, 0xE8, 0xEA, 0xEC, 0xEE, 0xF1, 0xF3, 0xF5, 0xF8, 0xFA, 0xFC, 0xFE
   },
   // Onda quadrada
   {
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
   }
};
WiFiClient espClient;
PubSubClient client(espClient);


// Limites de corrente para conversão
const float MIN_CURRENT = 4.0;  // Corrente mínima em mA 
const float MAX_CURRENT = 20.0; // Corrente máxima em mA 


// Função para configurar a conexão Wi-Fi
void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Conectando à rede ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi conectado");
    Serial.println("Endereço IP: ");
    Serial.println(WiFi.localIP());
}
// Função para reconectar ao servidor MQTT
void reconnect() {
    while (!client.connected()) {
        Serial.print("Tentando se reconectar ao MQTT Broker...");
        if (client.connect("ESP32Client")) {
            Serial.println("Conectado");
            client.subscribe("tcc/onda"); // Inscreve-se no tópico para receber o tipo de onda
            client.subscribe("tcc/carga"); // Inscreve-se no tópico para receber o comando pro SSR
        } else {
            Serial.print("Falhou, rc=");
            Serial.print(client.state());
            Serial.println(" Tentando novamente em 5 segundos");
            delay(5000);
        }
    }}
// Callback para tratamento de mensagens MQTT recebidas
void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Mensagem recebida no tópico: ");
    Serial.println(topic);


    // Converte o payload para string
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    // Determina o tipo de onda com base na mensagem recebida
    if (message.equals("senoidal")) {
        wave_type = 0;
    } else if (message.equals("triangular")) {
        wave_type = 1;
    } else if (message.equals("dente-de-serra")) {
        wave_type = 2;
    } else if (message.equals("quadrada")) {
        wave_type = 3;
    }
// Módulo SSR
    if (strcmp(topic, "tcc/carga") == 0) {
    if (payload[0] == '0') {
      digitalWrite(pinRele, LOW); // Liga a carga
      client.publish("status/carga", "Carga acionada");
    } else if (payload[0] == '1') {
      digitalWrite(pinRele, HIGH); // Desliga a carga
      client.publish("status/carga", "Carga desativada");
    }
  }
}
// Função para converter leitura analógica em corrente
float analogToCurrent(float value) {
    float current = MIN_CURRENT + ((MAX_CURRENT - MIN_CURRENT) / (3.3 - 0)) * (value - 0); 
    return current;
}
// Configurações iniciais do dispositivo
void setup() {
    Serial.begin(115200);
    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    pinMode(output_pin, OUTPUT); // Configura o pino de saída PWM
    pinMode(pinRele, OUTPUT); // Configura o pino de saída pro rele SSR
}
// Loop principal do programa
void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    // Leitura e processamento dos módulos


    // Módulo Conversor tensão para corrente
    int potValue = analogRead(potPin);
    float potVoltage = (3.3 / 4095.0) * potValue;
    float current_modulo1 = analogToCurrent(potVoltage);
    dacWrite(modulo1, potVoltage / 3.3 * 255);
    client.publish("tcc/potenciometro", String(potVoltage).c_str());
    client.publish("tcc/corrente1", String(current_modulo1).c_str());


    // Módulo Conversor corrente para tensão
    int voltage_value = analogRead(modulo2);
    float voltage_modulo2 = (3.3 / 4095.0) * voltage_value;
    float current_modulo2 = analogToCurrent(voltage_modulo2);
    client.publish("tcc/modulo2", String(voltage_modulo2).c_str());
    client.publish("tcc/corrente2", String(current_modulo2).c_str());


    // Módulo Amplificador de Ganho de Sinal para Geração de Funções
    dacWrite(onda, WaveFormTable[wave_type][i]);
    i++;
    if (i >= Num_Samples) i = 0;


    // Módulo MOSFET PWM
    int potValuePWM = analogRead(potentiometer_pin);
    int dutyCycle = map(potValuePWM, 0, 4095, 0, 1024);
    
    ledcAttachPin(output_pin, 0); // Associa o canal PWM ao pino de saída
    ledcSetup(0, freq, 10);        // Configura o canal PWM
    ledcWrite(0, dutyCycle);      // Define o valor do duty cycle


    float percentage = (float)dutyCycle / 1024.0 * 100.0;
    client.publish("tcc/duty", String(percentage).c_str());
    client.publish("tcc/freq", String(freq).c_str());


    delay(100); // Aguarda um curto intervalo antes da próxima iteração
}
