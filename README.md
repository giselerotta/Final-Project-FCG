# Final-Project-FCG
Projeto Final de Fundamentos de Computação Gráfica - INF01047
- Professor: Eduardo Gastal
- Integrantes: Gisele Rotta e Isadora Vidor

## Pré-Requisitos Técnicos

### Malhas Poligonais Complexas
Utilizamos modelos geométricos complexos para representar as estruturas do arqueiro, dos alvos e da flecha.

### Transformações Geométricas
Aplicamos transformações geométricas de rotação, translação e escala em cada um dos alvos. Elas podem ser controladas pelo usuário a partir do teclado.

### Câmeras Virtuais
Nosso jogo possui as câmeras do tipo look-at e free. A câmera do tipo look-at foi implementada com o objetivo de visualizar as movimentações do arqueiro como um telespectador. Já a câmera do tipo free se comporta como uma câmera em primeira pessoa, no ponto de vista do arqueiro.

### Instâncias de Objetos
Aplicamos 3 instâncias do mesmo objeto para implementar os nossos alvos, em que cada um dele possui uma matriz de modelagem diferente, o que permite posicioná-los em diferentes lugares na cena.

### Interseccção entre Objetos
No arquivo `collisions.cpp` calculamos as bounding boxes de cada objeto para que fosse possível verificar as colisões entre eles. Também criamos uma função que verifica intersecções entre pontos e cubos, utilizada para checar a intersecção da flecha com os alvos.

### Modelos de Iluminação 
Para o arqueiro utilizamos o modelo de iluminação de Phong e o modelo de interpolação de Gouraud. Para os outros objetos da cena foram utilizados o modelo de iluminação de Lambert e o modelo de interpolação de Phong.

### Mapeamento de Texturas
As texturas do arqueiro, flecha e alvos foram aplicadas através do arquivo `.mtl` que foi desenvolvido junto do arquivo `.obj`. As texturas dos planos foram escolhidas e aplicadas por nós.

### Curvas de Bézier
Utilizamos uma curva de Bézier de terceiro grau para simular a movimentação da flecha quando ela é solta pelo arqueiro.

### Animação de Movimento
Tanto a movimentação do arqueiro quanto a da flecha são realizadas baseada na variável `g_DeltaTime`, que computa o intervalo de tempo entre os frames.

## Funcionalidades

### Teclas A,W,S,D
Utilizadas para a movimentação do arqueiro:
- `A`: Esquerda
- `W`: Frente
- `S`: Trás
- `D`: Direita

### Tecla `Space`
Utilizada para soltar a flecha.

### Teclas E,R,T
Utilizadas para as transformações geométricas:
- `E`: Escalamento
- `R`: Rotação
- `T`: Translação

### Tecla F
Utilizada para mudar da câmera look-at para free e vice-versa.


