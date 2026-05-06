# Fluke Logger

Programa simples em C++ para ler medidas de corrente e tensão de um multímetro Fluke via porta serial e salvar os dados em CSV.

O fluxo do projeto e bem direto:

- abre a porta serial informada pelo usuário
- envia o comando `MEAS?` periodicamente
- le a resposta do multímetro
- grava as amostras em um arquivo CSV dentro da pasta `output/`
- tenta se reconectar automaticamente se a comunicação USB/serial cair

## Requisitos

- Windows
- MSYS2 com ambiente UCRT64

## Compilação

No diretório pai, use o comando abaixo para gerar o executável:

```bash
g++ src/log_fluke_v1.cpp -o log_fluke_v1.exe -lws2_32
```

Durante a execução, o programa pede a porta e cria automaticamente um arquivo CSV em `output/` com nome baseado na porta e na data/hora.

## Formato da saída

O arquivo gerado usa separador `;` e grava os campos:

- Date
- Time
- Voltage (V)
- Current (A)

Na primeira gravação do arquivo, o programa adiciona o cabeçalho CSV para facilitar a abertura no Excel.

## Comportamento do programa

- taxa serial configurada: 9600
- tamanho do byte: 8
- paridade: none
- stop bits: one
- intervalo de leitura: 1 segundo

Se a conexão for interrompida, o programa mostra uma mensagem de desconexão, tenta reabrir a porta em loop e continua registrando no mesmo processo assim que a comunicação voltar.

## Estrutura do projeto

- `src/log_fluke_v1.cpp`: versão principal do logger
- `output/`: pasta dos CSVs gerados

## Observações

- O nome do arquivo de saída é automático e inclui a porta serial e o timestamp de criação.
- O projeto foi pensado para ser simples e fácil de compilar com o `g++` do MSYS2 UCRT64.
- Se o multímetro retornar apenas uma leitura isolada, o programa ainda tenta registrar a linha no CSV e mostra a leitura no console.
