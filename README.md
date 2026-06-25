# 📡 Rede de Sensores IoT com Zigbee (ESP32-C6 & BeagleBone)

> Projeto final da disciplina de Internet das Coisas (IoT) do curso de Engenharia de Computação no IFPB (Campus Campina Grande).

Este projeto implementa uma rede de sensores sem fio distribuída utilizando o protocolo **Zigbee**. A arquitetura consiste em nós sensores baseados no **ESP32-C6** que coletam dados de Temperatura e Umidade e os transmitem de forma eficiente para um Gateway Coordenador rodando em uma **BeagleBone Black (BBB)**.

## 🏗️ Arquitetura do Sistema

O sistema é dividido em duas partes principais:

1. **End Device (Nó Sensor):**
   - Desenvolvido em **C** utilizando o framework **ESP-IDF** e **FreeRTOS**.
   - Utiliza o microcontrolador ESP32-C6 (com rádio 802.15.4 nativo).
   - Implementa clusters ZCL (Zigbee Cluster Library) padrão para medição de Temperatura e Umidade.
   - Possui rotina de reconexão automática e gerenciamento de energia.

2. **Coordinator (Gateway):**
   - Roda na **BeagleBone Black (BBB)** baseada em Linux.
   - Desenvolvido em **Python**.
   - Responsável por formar a rede Zigbee (Network Steering), parear os dispositivos e registrar os dados recebidos via interface serial/rádio.

## 🛠️ Tecnologias e Dependências

* **Linguagens:** C, Python 3
* **Hardware:** ESP32-C6, BeagleBone Black
* **Frameworks:** ESP-IDF (v5.3.5), FreeRTOS
* **Bibliotecas:** `espressif/esp-zigbee-lib` (v1.6.8), `pyserial`

---

## 🚀 Como Preparar o Ambiente e Executar

Siga a sequência de comandos abaixo no seu terminal para configurar o ESP32-C6 e, em seguida, preparar o servidor na BeagleBone:

```bash
# ==========================================
# 1. CONFIGURANDO O NÓ SENSOR (ESP32-C6)
# ==========================================
# Clone o repositório e entre na pasta
git clone [https://github.com/Rodolfilho/IoT-2026_1_projeto_final.git](https://github.com/Rodolfilho/IoT-2026_1_projeto_final.git)
cd IoT-2026_1_projeto_final

# Defina o chip alvo e adicione a dependência do Zigbee
idf.py set-target esp32c6
idf.py add-dependency "espressif/esp-zigbee-lib^1.6.8"

# Limpe o cache, compile e grave no ESP32-C6
idf.py fullclean
idf.py build flash monitor

# ==========================================
# 2. CONFIGURANDO O GATEWAY (BEAGLEBONE BLACK)
# ==========================================
# (Acesse a BBB via SSH antes de rodar os comandos abaixo)

# Atualize os pacotes do Linux na BBB
sudo apt update && sudo apt upgrade -y

# Instale o Python 3 e o gerenciador de pacotes (pip)
sudo apt install python3 python3-pip -y

# Instale as dependências do servidor serial
pip3 install pyserial

# Dê permissão ao seu usuário para acessar a porta serial
sudo usermod -a -G dialout $USER

# Inicie o servidor Python
cd servidorbbb
python3 servidor_gateway.py

```
| Nome | Matrícula / GitHub |
| :--- | :--- |
| José Rodolfo Rocha Filho | [@Rodolfilho](https://github.com/Rodolfilho) |
| Matheus Monteiro Maciel | [@MatheusMonteiro10](https://github.com/MatheusMonteiro10) |
| Nyedson Lorran Queiroz Barros | [@NyedsonLorran](https://github.com/NyedsonLorran) |
| Vinicius Gabriel Xavier Basilio | [@ViniciusGbasilio](https://github.com/ViniciusGbasilio) |
