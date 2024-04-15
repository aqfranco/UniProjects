;Alunos-
;Gabriel Ferreira, n.102747
;Andre Franco, n.102635

DISPLAYS   EQU 0A000H  ; endereco dos displays de 7 segmentos (periferico POUT-1)
TEC_LIN    EQU 0C000H  ; endereco das linhas do teclado (periferico POUT-2)
TEC_COL    EQU 0E000H  ; endereco das colunas do teclado (periferico PIN)
DEFINE_LINHA    		EQU 600AH      ; endereço do comando para definir a linha
DEFINE_COLUNA   		EQU 600CH      ; endereço do comando para definir a coluna
DEFINE_PIXEL    		EQU 6012H      ; endereço do comando para escrever um pixel
REP_SOM                 EQU 605AH      ; endereço do comando para reproduzir som
PARA_SOM EQU 605EH
APAGA_AVISO     		EQU 6040H      ; endereço do comando para apagar o aviso de nenhum cenário selecionado
APAGA_ECRÃ	 		EQU 6002H      ; endereço do comando para apagar todos os pixels já desenhados
SELECIONA_CENARIO_FUNDO  EQU 6042H      ; endereço do comando para selecionar uma imagem de fundo
ATRASO EQU 60H                     ; atraso para limitar a velocidade do rover
MIN_COLUNA EQU 0000H                ; limite esquerdo 
MAX_COLUNA EQU 0040H                ; limite direito
LINHA        		EQU  31        ; linha do rover (a meio do ecrã))
COLUNA			EQU  30        ; coluna do rover (a meio do ecrã)
COLUNA_M	EQU 7			   ; coluna para desenhar o meteoro
LINHA_MISSIL EQU 30
COLUNA_MISSIL EQU 30
VALOR_INI EQU 105             ; valor inicial da energia
LARGURA		EQU	5			; largura do rover
PASSO		EQU	4			; 
COR_PIXEL		EQU	0FE0FH		; cor do pixel: vermelho em ARGB (opaco e vermelho no máximo, verde e azul a 0)
COR_INICIAL EQU 0F888H
COR_MISSIL   EQU 0F0FFH
COR_BOM		EQU 0F00FH
COR_MAU    EQU 0FF00H
INATIVO     EQU  0
ATIVO       EQU 1
ALCANCE_MISSIL   EQU 18
MASCARA    EQU 0FH     ; para isolar os 4 bits de menor peso, ao ler as colunas do teclado

PLACE		1200H				

SP_inicial:

ENERGIA:
	WORD VALOR_INI
NUM_METEOROS:
	WORD 0

LINHAS_METEOROS:
	WORD -1

COLUNAS_METEOROS: ;nenhum destes valores é utilizado, são placeholders
	WORD COLUNA

COR_METEOROS:
	WORD 0

DEF_ROVER:					; tabela que define o rover (cor, largura, pixels)
	WORD		LARGURA
	WORD		COR_PIXEL, 0, COR_PIXEL, 0, COR_PIXEL		

INTERRUPTS:
	WORD desce_meteoro
	WORD move_missil
	WORD perde_energia


MISSIL:
	WORD LINHA_MISSIL
	WORD COLUNA_MISSIL ; este valor não é utilizado
	WORD INATIVO

PLACE      0
inicio:		
; inicializacoes
    MOV  SP, SP_inicial
    MOV  [APAGA_AVISO], R6	; apaga o aviso de nenhum cenário selecionado (o valor de R1 não é relevante)
    MOV  [APAGA_ECRÃ], R6	; apaga todos os pixels já desenhados (o valor de R1 não é relevante)
	MOV	 R6, 0			; cenário de fundo número 0
    MOV  [SELECIONA_CENARIO_FUNDO], R6	; seleciona o cenário de fundo
    MOV  R2, LINHA            ; cópia do valor da linha do rover
    MOV  R3, COLUNA           ; cópia do valor da coluna do rover
    MOV  R4, 8         ; valor da ultima linha (4ª linha, 1000b)
    MOV  R5, ATRASO    ; valor do atraso para regular a velocidade do rover
    MOV  R10, 0         ; este registo guarda o valor da tecla anterior

; corpo principal do programa

ativa_interrupcoes:
	MOV  BTE, INTERRUPTS
	EI0
	EI1
	EI2
	EI

display:               ;inicia o display
    CALL hexa_ini

posição_rover:
    MOV  R2, LINHA			; linha do rover
    MOV  R3, COLUNA		; coluna do rover

desenho_inicial:        ; faz o desenho inicial do rover e do meteoro
	CALL desenha_rover

detecta_tecla:        ; deteta a tecla
    MOV  R1, R4       ; testar a linha
    CALL teclado      
    CMP	 R0, 0         
    JZ ciclo_fim       ; se nenhuma tecla premida
    SHL  R1, 4         ; coloca linha no nibble high
    OR   R1, R0        ; junta coluna (nibble low)
    MOV  R7, R1        ; guarda valor da tecla
    JMP  testa_tecla_inicio           ; salta para fora depois de obter a tecla

ciclo_fim:
    SHR R4, 1          ; define a linha como a prévia
    CMP R4, 0          ; compara caso a linha seja 0
    JNZ detecta_tecla  ; se nao, mantem o valor atual da linha
    MOV R4, 8          ; se for, reinicia o valor da linha
	CMP R8, 1		 ; testa o valor alterado caso o programa tenha chegado ao fim
	JNZ testa_tecla_inicio
	JMP detecta_tecla	;caso sim, repete o ciclo

testa_tecla_inicio:
	MOV R1, 0081H
	CMP R7, R1 ; verifica se a tecla premida é 0
	JNZ testa_energia ; caso nao, testa a energia restante
	MOV R8, 0 ; indica que o programa recomecou 
	MOV	 R6, 0			; cenário de fundo número 0
    MOV  [SELECIONA_CENARIO_FUNDO], R6	; seleciona o cenário de fundo
	MOV  R6, VALOR_INI ;seleciona valor inicial
	MOV  [ENERGIA], R6 ;guarda o valor da energia
	JMP ativa_interrupcoes

testa_energia:
	MOV R6, [ENERGIA]
	CMP R6, 0 
	JNZ eh_mov
	JMP fim_jogo_1 ; salta para a funcao fim caso nao tenha mais energia

eh_mov:               ;verifica se a tecla premida corresponde a um movimento do rover
	MOV R1, 0011H     
	CMP R7, R1        ; verifica se a tecla premida é 0
	JZ ciclo_atraso
	MOV R1, 0014H      ; verifica se a tecla premida é 2
	CMP R1, R7
	JNZ cria_meteoro_ini           ; caso não seja nenhuma das duas salta todo o código referente ao movimento do rover

ciclo_atraso:
	SUB R5, 1  
	JNZ detecta_tecla            ; salta o movimento enquanto o tempo de ATRASO não acaba
	CALL apaga_rover

escolhe_mov:             ; seleciona o movimento correspondente à tecla
	MOV R1, 0011H          
	CMP R7, R1           ; verifica se a tecla é 0
	JZ move_para_esquerda; quando a tecla é 0 move o rover para a esquerda
	MOV R1, 0014H
	CMP R7, R1           ; verifica se a tecla é 2
	JZ move_para_direita ; quando a tecla é 2 move o rover para a direita

move_para_direita:
	MOV	R1, 1			      ; desloca-se para a direita
	JMP	testa_limite_direito  ; vai testar se pode avançar para a esquerda sem sair do ecrã
move_para_esquerda:
	MOV	R1, -1			      ; desloca-se para a esquerda
    JMP testa_limite_esquerdo ; vai testar se pode avançar para a direita sem sair do ecrã

testa_limite_esquerdo:  ; vê se o rover chegou ao limite esquerdo
    PUSH R5
    PUSH R6
	MOV	R5, MIN_COLUNA
	CMP	R3, R5
    POP R6
    POP R5
	JLE	para             ; caso tenha chegado, o rover permanece parado
    JMP coluna_seguinte
testa_limite_direito:
    PUSH R5
    PUSH R6		        ; vê se o rover chegou ao limite direito
	MOV	R6, [DEF_ROVER]	; obtém a largura do rover (primeira WORD da tabela)
	ADD	R6, R3			; posição a seguir ao extremo direito do rover
	MOV	R5, MAX_COLUNA
	CMP	R6, R5
    POP R6
    POP R5
	JGE	para            ; caso tenha chegado, o rover permanece parado
    JMP coluna_seguinte
	
para:                    ; mantém o rover parado
	MOV R1, 0

coluna_seguinte:
	ADD	R3, R1			  ; para desenhar objeto na coluna seguinte (direita ou esquerda)
    MOV R5, ATRASO        ; reinicia o valor do tempo de atraso
	CALL	desenha_rover ; vai desenhar o rover de novo
	JMP cria_meteoro_ini

cria_meteoro_ini:
    PUSH R0
	PUSH R1
	PUSH R2
	PUSH R3
	PUSH R4
	PUSH R5
	PUSH R6
	PUSH R7
	PUSH R8
	PUSH R9
cria_meteoro:
	MOV R1, NUM_METEOROS
	MOV R2, [NUM_METEOROS]
	CMP R2, 1 ; testa se o numero maximo de meteoros foi criado
	JZ meteoro_sai ; se sim, sai da funcao
	MOV R3, TEC_COL ; cria a funcionalidade pseudorandom do programa
	MOVB R4, [R3]   ; obtem um numero aleatório de 8 bits, para escolher um conjunto de 8 colunas
	SHR R4, 5		; divide esse numero por 2^5 para obter um valor entre 0 e 7
	MOVB R5, [R3]   ; repete processo para escolher uma coluna nesse conjunto
	SHR R5, 5
	MOV R7, 8
	MUL R4, R7
	ADD R4, R5      ; obtem a coluna 
    ADD R4, PASSO
    MOV R0, MAX_COLUNA
    CMP R4, R0		; verifica se o meteoro não transcende as barreiras do ecrã qualquer que seja o seu tamanho
    JGE cria_meteoro; caso contrário procura nova coluna aleatória
    SUB R4, PASSO
	SHL R2, 1       ; multiplica por 2 pois uma word tem 2 bytes
	MOV R9, R2      ; guarda o valor no contador
	MOV R6, COLUNAS_METEOROS
testa_repetida:     ;testa se uma coluna é repetida, não é necessário com apenas um meteoro mas para vários é
    CMP R2, 0		
    JZ continua_c_meteoro
	ADD R6, R9
	MOV R8, [R6]
	SUB R6, R9
	SUB R9, 2
	CMP R8, R4
	JZ cria_meteoro ; caso a coluna seja repetida procura uma nova coluna aleatória
	CMP R9, 0
	JGE testa_repetida ; caso seja repetida, gera uma nova coluna
continua_c_meteoro:
	ADD R6,R2			
	MOV [R6], R4		; atualiza a coluna do meteoro
	SHR R2, 1
	ADD R2, 1
	MOV [R1], R2		; incrementa o valor do numero de meteoros
    SUB R2, 1
	MOVB R7, [R3]		; gera um novo valor aleatório para escolher se é bom ou mau
	SHR R7, 5
	CMP R7, 1			; como o valor aleatório é entre 0 e 7, existe uma probabilidade de 1/4 de ele ser 0 ou 1
	JLE bom				; por isso só nesse caso o meteoro é bom
mau: ; define as propriedades (cor) do meteoro mau
	SHL R2, 1
	MOV R8, COR_METEOROS ;endereço da tabela que define a cor do meteoro
	ADD R8, R2
	MOV R9, COR_MAU 
	MOV [R8], R9 ;escreve o valor da cor na tabela
	JMP meteoro_sai
bom: ; define as propriedades (cor) do meteoro bom
	SHL R2, 1
	MOV R8, COR_METEOROS ;endereço da tabela que define a cor do meteoro
	ADD R8, R2
	MOV R9, COR_BOM
	MOV [R8], R9 ;escreve o valor da cor na tabela
meteoro_sai:
	POP R9
	POP R8
	POP R7
	POP R6
	POP R5
	POP R4
	POP R3
	POP R2
	POP R1
    POP R0
dispara: ;dispara um missil
	PUSH R1
	PUSH R2
	PUSH R4
	PUSH R5
	PUSH R6
	MOV R1, 0012H 
	CMP R7, R1 ;testa se a tecla 1 esta a ser premida
	JNZ d_sai ;se nao, sai da funcao
	MOV R2, MISSIL ;endereço da tabela que define o missil
	ADD R2, 4
	MOV R4, [R2] ;obtem a atividade/inatividade do missil
	CMP R4, ATIVO ; verifica se ja existe um missil ativo
	JZ d_sai ; se sim, sai da funcao
	SUB R2, 2 ;obtem coluna do missil
	MOV R5, R3 ; obtem a coluna do ponto de referencia do rover
	ADD R5, 2 ; obtem o centro do rover
	MOV [R2], R5 ;guarda a coluna do missil
	ADD R2, 2
	MOV R4, ATIVO ; define o missil como estando ativo
	MOV [R2], R4 ;guarda na tabela que o missil esta ativo
	MOV R6, 0
	MOV [REP_SOM], R6 ;reproduz som de disparo de missil
	MOV R6, [ENERGIA] ; reduz o valor de energia por 5
	SUB R6, 5
	MOV [ENERGIA], R6
	CALL hexa_ini ; atualiza o valor da energia no display
d_sai:
	POP R6
	POP R5
	POP R4
	POP R2
	POP R1

fim:
	MOV R4, 8
	MOV R10, R7          ; guarda o valor atual da tecla
    MOV R7, 0            ; reinicia para guardar o próximo valor da tecla
	JMP detecta_tecla    ; volta a tentar detetar nova tecla

teclado:
	PUSH	R2         
	PUSH	R3
	PUSH	R4
	MOV  R2, TEC_LIN   ; endereço do periférico das linhas
	MOV  R3, TEC_COL   ; endereço do periférico das colunas
	MOV  R4, MASCARA   ; para isolar os 4 bits de menor peso, ao ler as colunas do teclado
	MOVB [R2], R1      ; escrever no periférico de saída (linhas)
	MOVB R0, [R3]      ; ler do periférico de entrada (colunas)
	AND  R0, R4        ; elimina bits para além dos bits 0-3
	POP	R4
	POP	R3
	POP	R2
	RET
apaga_rover:       		; desenha o rover a partir da tabela
    PUSH R4
    PUSH R5
    PUSH R6
	PUSH R8
	MOV	R6, R3			; cópia da coluna do rover
	MOV	R4, DEF_ROVER		; endereço da tabela que define o rover
	MOV	R5, [R4]			; obtém a largura do rover

apaga_pixels:       		; desenha os pixels do rover a partir da tabela
	MOV	R8, 0			; para apagar, a cor do pixel é sempre 0
	MOV  [DEFINE_LINHA], R2	; seleciona a linha
	MOV  [DEFINE_COLUNA], R6	; seleciona a coluna
	MOV  [DEFINE_PIXEL], R8	; altera a cor do pixel na linha e coluna selecionadas
     ADD  R6, 1               ; próxima coluna
     SUB  R5, 1			; menos uma coluna para tratar
     JNZ  apaga_pixels		; continua até percorrer toda a largura do objeto
	 POP R8
    POP R6
     POP R5
     POP R4
	RET
desenha_rover:       		; desenha o boneco a partir da tabela
    PUSH R4
    PUSH R5
    PUSH R6
	PUSH R8
	MOV	R6, R3				   ; cópia da coluna do boneco
	MOV	R4, DEF_ROVER		   ; endereço da tabela que define o boneco
	MOV	R5, [R4]			   ; obtém a largura do boneco
	ADD	R4, 2			       ; endereço da cor do 1º pixel (2 porque a largura é uma word)
desenha_pixels_rover:          ; desenha os pixels do boneco a partir da tabela
	MOV	R8, [R4]			   ; obtém a cor do próximo pixel do boneco
	MOV  [DEFINE_LINHA], R2	   ; seleciona a linha
	MOV  [DEFINE_COLUNA], R6   ; seleciona a coluna
	MOV  [DEFINE_PIXEL], R8	   ; altera a cor do pixel na linha e coluna selecionadas
	ADD	R4, 2				   ; endereço da cor do próximo pixel (2 porque cada cor de pixel é uma word)
     ADD  R6, 1                ; próxima coluna
     SUB  R5, 1				   ; menos uma coluna para tratar
     JNZ  desenha_pixels_rover ; continua até percorrer toda a largura do objeto
	 POP R8
     POP R6
     POP R5
     POP R4
    RET

apaga_meteoro:       		; desenha o meteoro a partir da tabela
    PUSH R2                 
    PUSH R3
    PUSH R4
    PUSH R5
	PUSH R6  
    PUSH R7
    PUSH R8
    PUSH R9
    PUSH R10
	MOV	R2, LINHAS_METEOROS	; endereço da tabela onde esta guardada a linha do meteoro
    ADD R2, R1
    MOV R3, [R2] ; obtem valor da linha onde esta o meteoro
    MOV R2, COLUNAS_METEOROS ; endereço da tabela onde esta guardada a coluna do meteoro
    ADD R2, R1
    MOV R4, [R2] ; obtem valor da coluna onde esta o meteoro
	MOV R10, R4
	MOV R5, R3	; aqui começamos a obter o tamanho do meteoro
    MOV R6, 3	
    DIV R5, R6  ; aumenta de tamanho a cada 3 linhas
	ADD R5, 1	; adiciona o tamanho inicial
    CMP R5, 5	; vê se já atingiu o valor máximo
    JLE contadores_a
    MOV R5, 5 
contadores_a: ; inicializa os contadores dos ciclos abaixo
	MOV R7, R5
    MOV R8, R5
apaga_pixels_meteoro:       	; apaga os pixels do meteoro
	MOV	R9, 0			        ; para apagar, a cor do pixel é sempre 0
	MOV  [DEFINE_LINHA], R3	    ; seleciona a linha
	MOV  [DEFINE_COLUNA], R4	; seleciona a coluna
	MOV  [DEFINE_PIXEL], R9	    ;altera a cor do pixel na linha e coluna selecionadas
     ADD  R4, 1                 ;próxima coluna
     SUB  R5, 1			        ; menos uma coluna para tratar
     JNZ  apaga_pixels_meteoro  ; continua até percorrer toda a largura do objeto
	 MOV R5, R7
	 MOV R4, R10			; cópia da coluna do meteoro
	 ADD R3, 1
	 SUB R8, 1	                ; menos uma linha para tratar
	 JNZ apaga_pixels_meteoro
    POP R10
    POP R9
    POP R8
    POP R7
    POP R6
    POP R5
    POP R4
    POP R3
    POP R2
    RET

desenha_meteoro_ini:       		; desenha o meteoro a partir da tabela
    PUSH R0
    PUSH R2                 
    PUSH R4
    PUSH R5
	PUSH R6  
    PUSH R7
    PUSH R8
    PUSH R9
	PUSH R10
	PUSH R3
	MOV R11, 0; inicializa se o valor deste registo a 0 para verificar se o 
				;meteoro colidiu com um missil ou ultrapassou os limites do ecra
	MOV	R2, LINHAS_METEOROS	; endereço da tabela onde esta guardada a linha do meteoro
    ADD R2, R1
    MOV R3, [R2] ; obtem valor da linha onde esta o meteoro
	ADD	R3, PASSO ; coloca o ponto de referencia no final do meteoro
	MOV R0, LINHA 
	CMP	R3, R0 ; compara se a linha onde esta o final do meteoro coincide com o final do ecra 
	JGE elimina_meteoro ;caso sim, elimina o meteoro
    SUB R3, PASSO ; volta a colocar o ponto de referencia no inicio do Meteoro
    MOV R2, COLUNAS_METEOROS; endereço da tabela onde esta guardada a coluna do meteoro
    ADD R2, R1
    MOV R4, [R2]; obtem valor da coluna onde esta o meteoro
	MOV R10, R4
    MOV R2, COR_METEOROS ;endereço da tabela que define a cor do meteoro
    ADD R2, R1
    MOV R5, [R2]; obtem valor da cor do meteoro
	CMP R3, 6 ; verifica se o tamanho do meteoro e inferior a 3
	JGE desenho_meteoro
	MOV R5, COR_INICIAL; caso sim, coloca a sua cor a cinzento
desenho_meteoro:; faz o calculo automatico do meteoro em relacao a linha que esta
	MOV R6, R3	
    MOV R7, 3
    DIV R6, R7
    ADD R6, 1
    CMP R6, 5
    JLE contadores_d
    MOV R6, 5
contadores_d:
    MOV R8, R6
    MOV R9, R6
	MOV R11, 1 ; define que o meteoro foi explodido com um missil
    JMP testa_missil ; testa se o meteoro foi explodido por um missil
desenha_pixels_meteoro:       	; apaga os pixels do meteoro 
	MOV  [DEFINE_LINHA], R3	    ; seleciona a linha
	MOV  [DEFINE_COLUNA], R4	; seleciona a coluna
	MOV  [DEFINE_PIXEL], R5	    ;altera a cor do pixel na linha e coluna selecionadas
     ADD  R4, 1                 ;próxima coluna
     SUB  R6, 1			        ; menos uma coluna para tratar
     JNZ  desenha_pixels_meteoro  ; continua até percorrer toda a largura do objeto
	 MOV R6, R8
	 MOV R4, R10			; cópia da coluna do meteoro
	 ADD R3, 1
	 SUB R9, 1	                ; menos uma linha para tratar
	 JNZ desenha_pixels_meteoro
desenha_meteoro_sai:
	POP R3
desenha_meteoro_ok:
	POP R10
    POP R9
    POP R8
    POP R7
    POP R6
    POP R5
    POP R4
    POP R2
    POP R0
    RET

elimina_meteoro:
	MOV R6, [NUM_METEOROS] ; obtem o numero de meteoros guardados numa tabela
 	SUB R6, 1 ; subtrai o numero de meteoros existentes
    MOV [NUM_METEOROS], R6 ; guarda esse valor na tabela
	MOV R3, -1 ;reinicia o valor da linha
	MOV [LINHAS_METEOROS], R3 ; e guarda esse valor na tabela que define a linha do meteoro
	MOV R2, COR_METEOROS
    ADD R2, R1
    MOV R5, [R2]
	MOV R7, COR_BOM 
	CMP R5, R7 ;verifica se e um meteoro bom
	JZ elimina_meteoro_bom
	JMP elimina_meteoro_mau

elimina_meteoro_bom:
	CMP R11, 1; verifica se o meteoro colidiu com um missil
	JZ desenha_meteoro_sai ;se sim, sai pois a energia nao aumenta
 	JMP meteoro_colisao

elimina_meteoro_mau:
	CMP R11, 0; verifica se o Meteoro ultrapassou os limites do ecra
	JZ meteoro_colisao ;se sim, testa se colidiu com o rover ou nao
	MOV R6, [ENERGIA] ;aumenta o valor da energia
	ADD R6, 5
	MOV [ENERGIA], R6
	CALL hexa_ini; e atualiza-o no display
	JMP desenha_meteoro_sai; sai da funcao

meteoro_colisao:
    PUSH R2
    MOV R2, COLUNAS_METEOROS
    ADD R2, R1
    MOV R4, [R2]
    POP R2
    POP R3
    SUB R4, R3; subtrai o valor da coluna do meteoro com a do rover
    CMP R4, 5
    JGE desenha_meteoro_ok ; se for superior ou igual a 5, sai da funcao
    CMP R4, -5
    JLE desenha_meteoro_ok ; se for inferior ou igual a -5, sai da funcao
	MOV R2, COR_METEOROS
    ADD R2, R1
    MOV R5, [R2]
	MOV R7, COR_BOM
	CMP R5, R7 ;verifica se o meteoro tem cor boa
	JNZ fim_jogo_2 ; caso nao tenha, finaliza-se o programa
	MOV R6, 3 ;reproducao de som de colisao do meteoro bom com o rover
	MOV [REP_SOM], R6 
	MOV R5, 10 
	MOV R6, [ENERGIA]
	ADD R6, R5 ;aumento da energia
	MOV [ENERGIA], R6
	CALL hexa_ini ; atualizacao no display
	JMP desenha_meteoro_ok ;sai da funcao

fim_jogo:
	MOV R8, 1 ; forma de detetar se o programa chegou ao fim ou nao
	JMP detecta_tecla

fim_jogo_1:
	CALL apaga_rover
	CALL apaga_meteoro
	MOV	 R6, 1
    MOV  [SELECIONA_CENARIO_FUNDO], R6 ; troca de cenario de fundo
	MOV R6, 2
	MOV [REP_SOM], R6 ; reproduz som de falta de energia
	DI0
	DI1
	DI2
	DI
	JMP fim_jogo

fim_jogo_2:
	CALL apaga_rover
	MOV	 R6, 2
    MOV [SELECIONA_CENARIO_FUNDO], R6 ; troca de cenario de fundo
	MOV R6, 1
	MOV [REP_SOM], R6 ; reproduz som de falta de energia
	DI0
	DI1
	DI2
	DI
	JMP fim_jogo

testa_missil:    ; verifica se o missil atingiu o meteoro
    PUSH R5
    PUSH R6
    PUSH R7
    PUSH R8
    MOV R5, MISSIL
    MOV R6, [R5]	; obtem a linha do missil
    ADD R5, 2 
    MOV R7, [R5]	;obtem a coluna do missil
    ADD R5, 2		
    MOV R8, [R5]	; obtem o estado de atividade do missil
    ADD R3, PASSO	; outra ponta do meteoro, tem sempre tamanho 5 quando é atingido
    ADD R4, PASSO   ; lado direito
    CMP R8, ATIVO	; verifica se o missil está ativo
    JNZ testa_missil_fim
    CMP R3, R6		; caso esteja verifica se está en contacto com a linha abaixo do meteoro
    JN testa_missil_fim
    CMP R4, R7		; verifica se não passa do lado direito do meteoro
    JN  testa_missil_fim
    MOV R6, R4		
    SUB R6, PASSO
    CMP R7, R6		; verifica se não passa do lado esquerdo
    JN  testa_missil_fim
    MOV R0, 1       ; bit indica que o missil atinge
testa_missil_fim:
    SUB R3, PASSO	; restora os valores iniciais
    SUB R4, PASSO	
    POP R8
    POP R7
    POP R6
    POP R5
    CMP R0, 1       
    JZ elimina_meteoro ; caso o missil tenha atingido o meteoro é eliminado
    JMP desenha_pixels_meteoro

desce_meteoro: 
	PUSH R1
    PUSH R2
    PUSH R4
    MOV R1, 1 ;numero de meteoros
    SUB R1, 1
	SHL R1, 1
    MOV R2, LINHAS_METEOROS ; endereço da tabela onde esta guardada a linha do meteoro
ciclo_desce_meteoro:
    ADD R2, R1
    MOV R4, [R2] ;move o valor da linha para R4
    CALL apaga_meteoro ;apaga o meteoro da posicao antiga
    ADD R4, 1 ; adiciona uma linha
    MOV [R2], R4 ; move o valor de R4 para a tabela
    SUB R2, R1
	CALL desenha_meteoro_ini ; desenha o meteoro na nova posicao
    SUB R1, 2
    CMP R1, 0 ; verifica se todos os meteoros foram percorridos
    JGE ciclo_desce_meteoro
fim_desce_m:
    POP R4
    POP R2
    POP R1
	RFE

move_missil:
	PUSH R1
	PUSH R3
	PUSH R4
	PUSH R5
	PUSH R6
	PUSH R7
	PUSH R8
	PUSH R9
	PUSH R11
	MOV R4, MISSIL
	MOV R5, [R4]  ; obtem linha do missil
	MOV R8, R5	
	ADD R8, 1	  ; passa para a linha seguinte
	ADD R4, 2
	MOV R6, [R4]  ; obtem a coluna
	ADD R4, 2
	MOV R3, [R4]  ; obtem o estado de atividade
	MOV R9, 31
	CMP R8, R9
	JZ desenha_missil  ; salta o processo de apagar caso o missil esteja no inicio do seu percurso
	MOV [DEFINE_LINHA], R8   ; seleciona a linha
	MOV [DEFINE_COLUNA], R6  ; seleciona a coluna
	MOV R1, 0               ; para apagar a cor é 0
	MOV [DEFINE_PIXEL], R1

desenha_missil:
	CMP R3, ATIVO         ; caso o missil já esteja ativo esta rotina não o desenha
	JNZ r_l_missil
	MOV R11, ALCANCE_MISSIL
	CMP R5, R11			  ; verifica se o alcance máximo do missilfoi atingido
	JZ r_l_missil2		  ; caso sim não desenha o missil
	MOV [DEFINE_LINHA], R5 ; seleciona a linha
	MOV [DEFINE_COLUNA], R6 ; seleciona a coluna
	MOV R1, COR_MISSIL
	MOV [DEFINE_PIXEL], R1 ; desenha o missil
	SUB R5, 1
	SUB R4, 4
	MOV [R4], R5           ; atualiza o valor da linha
	JMP r_l_missil       

r_l_missil2:
	PUSH R10
	PUSH R5
	MOV R10, INATIVO        ; quando o missil chega ao alcance final ele fica inativo
	MOV [R4], R10
	MOV R5, 30
	SUB R4, 4
	MOV [R4], R5            ; mantém o valor da linha caso não tenha sido desenhado
	POP R5
	POP R10
r_l_missil:
	POP R11
	POP R9
	POP R8
	POP R7
	POP R6
	POP R5
	POP R4
	POP R3
	POP R1
	RFE

perde_energia:
	PUSH R6
	MOV R6, [ENERGIA]
    CMP R6, 0
    JLE perde_energia_final           ; se já atingiu o valor mínimo não diminui a energia
    SUB R6, 5           ; diminui a energia
	MOV [ENERGIA], R6
	CALL hexa_ini
perde_energia_final:
	POP R6
    RFE

hexa_ini:       ;rotina que converte numeros hexadecimais em decimais, faz com que o valor mostrado no ecrã apareça em decimal
	PUSH R1
	PUSH R2
	PUSH R3
	PUSH R4
	PUSH R5
	PUSH R7
	MOV R5, 0 
	CMP R6, 0   ; vê se o valor é 0
	JZ hexa_sai ; caso seja a sua representação em decimal é a mesma
	MOV R1, R6 
	MOV R7, 10 
	MOV R0, 1
	MOV R4, 16
hexa:          
	MOV R2, R1
	MOD R2, R7 ; obtem o ultimo digito da energia
	MUL R2, R0 ; multiplica-o por 16^ordem_do_digito_a_contar_do_fim
	MUL R0, R4
	DIV R1, R7 ; remove o ultimo digito
	ADD R5, R2 ; adiciona o resultado da multiplicação acima à representação em decimal
	CMP R1, 0  
	JNZ hexa   ; enquanto houverem digitos o ciclo repete-se
hexa_sai:
	MOV [DISPLAYS], R5 ;escreve o valor da energia em decimal nos displays
	POP R7
	POP R5
	POP R4
	POP R3
	POP R2
	POP R1
	RET
