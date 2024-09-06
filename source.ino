#include <WiFi.h>         // Biblioteca para utilizar o Wi-Fi no ESP32.
#include <PubSubClient.h> // Biblioteca para utilizar o protocolo MQTT.

// Configurações - variáveis editáveis
const char* default_SSID = "Wokwi-GUEST"; // Nome da rede Wi-Fi.
const char* default_PASSWORD = "";        // Senha da rede Wi-Fi.
const char* default_BROKER_MQTT = "";     // Endereço IP do Broker MQTT.
const int default_BROKER_PORT = 1883;     // Porta de conexão do Broker MQTT (1883 é o padrão).
const char* default_TOPICO_SUBSCRIBE = "/TEF/lamp001/cmd";   // Tópico para receber comandos.
const char* default_TOPICO_PUBLISH_1 = "/TEF/lamp001/attrs"; // Tópico para enviar dados.
const char* default_TOPICO_PUBLISH_2 = "/TEF/lamp001/attrs/l"; // Tópico para enviar dados de luminosidade.
const char* default_ID_MQTT = "fiware_001"; // ID único do cliente MQTT.
const int default_D4 = 2; // Pino do LED onboard no ESP32.

// Declaração da variável para o prefixo do tópico
const char* topicPrefix = "lamp001"; // Prefixo para formar os tópicos de comando.

// Variáveis para configurações editáveis
// Essas variáveis podem ser alteradas ao longo do programa.
char* SSID = const_cast<char*>(default_SSID);
char* PASSWORD = const_cast<char*>(default_PASSWORD);
char* BROKER_MQTT = const_cast<char*>(default_BROKER_MQTT);
int BROKER_PORT = default_BROKER_PORT;
char* TOPICO_SUBSCRIBE = const_cast<char*>(default_TOPICO_SUBSCRIBE);
char* TOPICO_PUBLISH_1 = const_cast<char*>(default_TOPICO_PUBLISH_1);
char* TOPICO_PUBLISH_2 = const_cast<char*>(default_TOPICO_PUBLISH_2);
char* ID_MQTT = const_cast<char*>(default_ID_MQTT);
int D4 = default_D4; // Pino do LED onboard.

WiFiClient espClient; // Cria um cliente Wi-Fi.
PubSubClient MQTT(espClient); // Cria um cliente MQTT usando o cliente Wi-Fi.
char EstadoSaida = '0'; // Variável para armazenar o estado do LED (ligado/desligado).

// Função para iniciar a comunicação serial.
void initSerial() {
    Serial.begin(115200);
}

// Função para conectar ao Wi-Fi.
void initWiFi() {
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    reconectWiFi(); // Chama a função que reconecta ao Wi-Fi.
}

// Função para configurar o MQTT (Broker e callback).
void initMQTT() {
    MQTT.setServer(BROKER_MQTT, BROKER_PORT); // Configura o servidor MQTT.
    MQTT.setCallback(mqtt_callback); // Configura a função de callback para receber mensagens.
}

// Função de setup do ESP32.
void setup() {
    InitOutput();  // Inicializa o estado do LED onboard.
    initSerial();  // Inicia a comunicação serial.
    initWiFi();    // Conecta ao Wi-Fi.
    initMQTT();    // Configura o MQTT.
    delay(5000);   // Espera 5 segundos antes de enviar a primeira mensagem.
    MQTT.publish(TOPICO_PUBLISH_1, "s|on"); // Publica uma mensagem no tópico para indicar que está online.
}

// Função principal que será executada repetidamente.
void loop() {
    VerificaConexoesWiFIEMQTT(); // Verifica a conexão com Wi-Fi e MQTT.
    EnviaEstadoOutputMQTT();     // Envia o estado do LED ao broker MQTT.
    handleLuminosity();          // Lê e envia o valor da luminosidade.
    MQTT.loop();                 // Mantém o cliente MQTT ativo.
}

// Função para reconectar ao Wi-Fi.
void reconectWiFi() {
    if (WiFi.status() == WL_CONNECTED)
        return; // Se já estiver conectado, sai da função.

    WiFi.begin(SSID, PASSWORD); // Conecta ao Wi-Fi.
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP()); // Mostra o IP obtido.

    // Garante que o LED onboard inicie desligado.
    digitalWrite(D4, LOW);
}

// Função chamada ao receber mensagens MQTT.
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) {
        char c = (char)payload[i];
        msg += c; // Converte o payload em uma string.
    }
    Serial.print("- Mensagem recebida: ");
    Serial.println(msg);

    // Forma o padrão de tópico para comparar com as mensagens recebidas.
    String onTopic = String(topicPrefix) + "@on|";
    String offTopic = String(topicPrefix) + "@off|";

    // Liga o LED caso a mensagem seja de ligar.
    if (msg.equals(onTopic)) {
        digitalWrite(D4, HIGH);
        EstadoSaida = '1';
    }

    // Desliga o LED caso a mensagem seja de desligar.
    if (msg.equals(offTopic)) {
        digitalWrite(D4, LOW);
        EstadoSaida = '0';
    }
}

// Verifica se o Wi-Fi e o MQTT estão conectados.
void VerificaConexoesWiFIEMQTT() {
    if (!MQTT.connected())
        reconnectMQTT(); // Se o MQTT não estiver conectado, tenta reconectar.
    reconectWiFi();       // Verifica a conexão Wi-Fi.
}

// Função para enviar o estado do LED ao broker MQTT.
void EnviaEstadoOutputMQTT() {
    if (EstadoSaida == '1') {
        MQTT.publish(TOPICO_PUBLISH_1, "s|on"); // Publica que o LED está ligado.
        Serial.println("- Led Ligado");
    }

    if (EstadoSaida == '0') {
        MQTT.publish(TOPICO_PUBLISH_1, "s|off"); // Publica que o LED está desligado.
        Serial.println("- Led Desligado");
    }
    Serial.println("- Estado do LED onboard enviado ao broker!");
    delay(1000); // Espera 1 segundo antes de enviar o próximo dado.
}

// Inicializa o pino de saída (LED) e pisca o LED para indicar o início.
void InitOutput() {
    pinMode(D4, OUTPUT); // Configura o pino D4 como saída.
    digitalWrite(D4, HIGH); // Inicialmente, liga o LED.
    boolean toggle = false;

    for (int i = 0; i <= 10; i++) { // Pisca o LED 10 vezes.
        toggle = !toggle;
        digitalWrite(D4, toggle);
        delay(200);
    }
}

// Função para reconectar ao Broker MQTT.
void reconnectMQTT() {
    while (!MQTT.connected()) { // Tenta conectar enquanto não estiver conectado.
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) { // Conecta ao broker MQTT.
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE); // Se conecta ao tópico de comandos.
        } else {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Haverá nova tentativa de conexão em 2s");
            delay(2000); // Espera 2 segundos antes de tentar novamente.
        }
    }
}

// Função que lida com a luminosidade, lendo de um pino analógico e enviando via MQTT.
void handleLuminosity() {
    const int potPin = 34; // Pino onde está conectado o sensor de luminosidade.
    int sensorValue = analogRead(potPin); // Lê o valor do sensor (0 a 4095).
    int luminosity = map(sensorValue, 0, 4095, 0, 100); // Mapeia o valor para uma escala de 0 a 100.
    String mensagem = String(luminosity); // Converte o valor em uma string.
    Serial.print("Valor da luminosidade: ");
    Serial.println(mensagem.c_str()); // Exibe o valor da luminosidade.
    MQTT.publish(TOPICO_PUBLISH_2, mensagem.c_str()); // Envia o valor via MQTT.
}
