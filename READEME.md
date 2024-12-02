# Tomasulo Simulator

Este é um simulador do algoritmo Tomasulo, implementado em C++. O simulador processa instruções e simula unidades funcionais com latência configurável.

## Compilando o Código

### Usando G++

Compile o código com o seguinte comando:

```bash
g++ -o tomasulo main.cpp
```

### Usando Nix

Este projeto suporta o uso do Nix para configurar o ambiente de desenvolvimento. Para compilar com Nix, siga os passos abaixo:

#### Entre no ambiente de desenvolvimento:

```bash
nix develop
```

#### Compile o código:

```bash
compile
```

#### Executando o Simulador

Execute o programa com o comando abaixo:

```bash
./tomasulo <input_file_path> <output_file_path>
```

- <input_file_path>: Caminho para o arquivo de entrada contendo as instruções.
- <output_file_path>: Caminho onde os resultados da execução serão salvos.

## Nosso Simulador

### Instruções Implementadas

- Unidade de Lógica Aritmética (ULA ADD)

  Instruções: add, sub

- Unidade de Multiplicação (ULA MUL)

  Instruções: mul, div

- Unidade de Armazenamento (Storage Unit)

  Instruções: lw, sw

### Constantes do Simulador

As seguintes constantes configuram os parâmetros do simulador:

#### Ciclos por unidade funcional:

- ADD_UNIT_CLOCK: ciclos para operações de soma/subtração.
- MUL_UNIT_CLOCK: ciclos para operações de multiplicação/divisão.
- SW_UNIT_CLOCK: ciclos para operações de armazenamento.

#### Quantidade de unidades funcionais:

- ADD_UNIT_QNT: unidades de soma/subtração.
- MUL_UNIT_QNT: unidades de multiplicação/divisão.
- SW_UNIT_QNT: unidades de armazenamento.
  Quantidade de registradores:
- REGISTER_QNT: registradores disponíveis.
