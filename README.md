# ClimaX Edge — Monitoramento Climático Urbano com Arduino Mega

## Descrição do Projeto

O **ClimaX Edge** é o módulo de edge computing da plataforma ClimaX — uma solução inteligente de monitoramento climático urbano. Este módulo é responsável pela coleta local de dados ambientais por meio de sensores IoT conectados a um Arduino Mega, realizando o processamento na borda (edge) para calcular em tempo real o **IRC (Índice de Risco Climático Urbano)**.

O sistema integra múltiplos sensores de temperatura, umidade, luminosidade e simulações de cobertura vegetal (NDVI) e focos de queimada, gerando uma classificação de risco que pode ser utilizada por órgãos públicos, Defesa Civil e comunidades para tomadas de decisão preventivas diante de eventos climáticos extremos.

O projeto faz parte da Global Solution da FIAP e aborda as ODS 11 (Cidades Sustentáveis), ODS 13 (Ação Contra a Mudança do Clima) e ODS 10 (Redução das Desigualdades).

## Objetivo da Solução

Desenvolver um sistema embarcado capaz de:

- Monitorar condições climáticas urbanas locais em tempo real (temperatura, umidade, luminosidade)
- Estimar cobertura vegetal e presença de queimadas/poluição na região
- Calcular um índice composto de risco climático (IRC) com base em múltiplas variáveis ambientais
- Classificar automaticamente o nível de risco em **baixo**, **moderado** ou **crítico**
- Emitir alertas visuais (LED RGB) e sonoros (buzzer) proporcionais ao nível de risco
- Fornecer dados processados na borda para integração futura com dashboards e sistemas de alerta centralizados

## Componentes Utilizados

| Componente | Função | Pino |
|---|---|---|
| Arduino Mega 2560 | Microcontrolador principal | — |
| DHT22 | Sensor de temperatura e umidade do ar | D2 |
| DS18B20 | Sensor de temperatura de superfície | D4 |
| NTC (Thermistor) | Sensor complementar de temperatura | A1 |
| LDR (Fotoresistor) | Sensor de luminosidade | A0 |
| Potenciômetro (NDVI) | Simula índice de vegetação | A2 |
| Potenciômetro (Queimadas) | Simula focos de queimada/poluição | A3 |
| LED RGB (cátodo comum) | Indicador visual de risco | D9 / D10 / D11 |
| Buzzer | Alerta sonoro para risco crítico | D8 |
| Resistores 220Ω (x3) | Limitadores de corrente do LED RGB | — |
| Resistor 4.7kΩ | Pull-up do barramento OneWire (DS18B20) | — |

## Explicação do Funcionamento

### Coleta de Dados

O sistema realiza a leitura contínua de sensores a cada 2 segundos:

1. **Temperatura do ar** — DHT22 (sensor digital de alta precisão)
2. **Temperatura de superfície** — DS18B20 (sensor digital OneWire, simula temperatura do solo/concreto)
3. **Temperatura complementar** — NTC (termistor analógico, cálculo via equação de Steinhart-Hart simplificada com β = 3950)
4. **Umidade relativa do ar** — DHT22
5. **Luminosidade** — LDR (fotoresistor analógico)
6. **NDVI simulado** — Potenciômetro mapeado para faixa 0.10–0.80 (quanto menor, menos vegetação)
7. **Queimadas simuladas** — Potenciômetro mapeado para 0–20 focos

### Cálculo do IRC (Índice de Risco Climático Urbano)

O IRC é um índice composto de 0 a 10 calculado pela fórmula:

```
IRC = (0.40 × rTemp) + (0.30 × rVeg) + (0.20 × rUmid) + (0.10 × rLuz) + penalidade_queimadas
```

Onde cada componente é normalizado para uma escala de 0 a 10:

- **rTemp (peso 40%)** — Risco por temperatura: função exponencial que penaliza temperaturas acima de 20°C, usando a média dos três sensores
- **rVeg (peso 30%)** — Risco por baixa vegetação: função logarítmica inversa do NDVI (quanto menor a cobertura verde, maior o risco)
- **rUmid (peso 20%)** — Risco por umidade: desvio quadrático em relação à umidade ideal de 65%
- **rLuz (peso 10%)** — Risco por radiação solar intensa: normalização linear da leitura do LDR
- **Penalidade queimadas (0–2 pontos extras)** — Função logarítmica baseada no número de focos simulados

### Classificação de Risco

| IRC | Classificação | LED | Buzzer |
|---|---|---|---|
| 0.0 – 3.4 | Baixo Risco | 🟢 Verde | Desligado |
| 3.5 – 6.4 | Risco Moderado | 🟡 Amarelo | Desligado |
| 6.5 – 10.0 | Risco Crítico | 🔴 Vermelho | Ativado (1200 Hz) |

## Estrutura do Circuito

```
                    +5V ────────────────────────────────────────────┐
                     │                                              │
              ┌──────┼──────┬──────────┬───────────┬───────────┐    │
              │      │      │          │           │           │    │
           [DHT22] [DS18B20] [NTC]    [LDR]    [Pot NDVI] [Pot Fire]
              │      │   │    │          │           │           │
            DATA    DQ  4.7kΩ OUT       AO         SIG         SIG
              │      │   │    │          │           │           │
              D2     D4  +5V  A1        A0          A2          A3
              │      │        │          │           │           │
              └──────┴────────┴──────────┴───────────┴───────────┘
                              ARDUINO MEGA 2560
              ┌──────┬──────────┬──────────────────────────────────┐
              │      │          │                                    │
              D9     D10        D11                                 D8
              │      │          │                                    │
           [220Ω] [220Ω]    [220Ω]                              [Buzzer]
              │      │          │                                    │
              R      G          B                                    │
              └──────┴──────────┘                                   │
                  [LED RGB]                                          │
                     │                                              │
                    GND ────────────────────────────────────────────┘
```

**Observações:**
- O DS18B20 utiliza resistor pull-up de 4.7kΩ entre VCC e a linha de dados (protocolo OneWire)
- O LED RGB é do tipo cátodo comum (compartilha GND)
- Cada canal do RGB possui resistor de 220Ω para limitar corrente
- Todos os sensores são alimentados por 5V do Arduino Mega

## Bibliotecas Utilizadas

- `DHT sensor library` — Leitura do sensor DHT22
- `Adafruit Unified Sensor` — Dependência da biblioteca DHT
- `OneWire` — Comunicação com protocolo OneWire (DS18B20)
- `DallasTemperature` — Leitura simplificada do DS18B20

## Instruções de Execução

### Simulação no Wokwi

1. Acesse [wokwi.com](https://wokwi.com)
2. Importe os arquivos `sketch.ino` e `diagram.json` do repositório
3. Clique em **Start Simulation**
4. Ajuste os potenciômetros e valores dos sensores para simular diferentes cenários climáticos
5. Observe o Serial Monitor para acompanhar os dados e o IRC calculado

### Hardware Físico

1. Monte o circuito conforme a tabela de pinos e o diagrama acima
2. Instale as bibliotecas necessárias pela Arduino IDE:
   - Vá em **Sketch > Incluir Biblioteca > Gerenciar Bibliotecas**
   - Pesquise e instale: `DHT sensor library`, `Adafruit Unified Sensor`, `OneWire`, `DallasTemperature`
3. Selecione a placa **Arduino Mega 2560** em **Ferramentas > Placa**
4. Selecione a porta COM correta em **Ferramentas > Porta**
5. Faça upload do `sketch.ino`
6. Abra o **Serial Monitor** (9600 baud) para visualizar os dados em tempo real

## Integrantes

| Nome | RM |
|---|---|
| Alisson Gomes Pereira | 573681 |
| Guilherme Vidichosqui Men | 570269 |
| Gustavo Teotônio Silva | 572803 |
| Matheus Ferraz Stenzl | 572238 |
| Thiago Vitelo do Nascimento | 569726 |
