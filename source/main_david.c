#include <16F877a.h>
#include <stdio.h>
#use delay (clock = 20000000)
#fuses HS, NOWDT, NOPROTECT, NOPUT, NOBROWNOUT, NOLVP
#use rs232 (baud = 9600, bits = 8, stream=PEND, uart1, errors) //configurações para a porta serial xmit = pin_c6, rcv = pin_c7


typedef struct packet
{
   int posicao;
   int tempo_pos;
   int angulo;
   int tempo_ang;
   int estado_carro;
   int estado_pendulo;
}ReceivePacket;

typedef enum{
   ESPERANDO_H,
   ESPERANDO_POS,
   ESPERANDO_TEMPO_POS,
   ESPERANDO_ANG,
   ESPERANDO_TEMPO_ANG,
   ESPERANDO_ESTADO_CARRO,
   ESPERANDO_ESTADO_PENDULO,
   ESPERANDO_BARRA_N
}EstadoRecepcao;

int StringToInt(char* string, int size);
void decode();


BOOLEAN recebeuTudo;
int indexString;
char string_recebida[50];
ReceivePacket pacote;

char numeroRecebido[3];
int indiceNumeroRecebido;
EstadoRecepcao estadoAtual = ESPERANDO_H;


#int_rda
void reception ()
{
   char temp;
   temp = getch();
   
   string_recebida[ indexString++ ] = temp;
   
   if(temp == '\n'){
      recebeuTudo = TRUE;
   }
   
}


//O protocolo é: H,<pos_carro>,<tempo_carro>,<angulo>,<tempo_angulo>,<estado_carro>,<estado_pendulo>,\n 

void decode(){

   char i, temp;
   for(i = 0 ; i < indexString ; i++){
      temp = string_recebida[i];
      
      if(estadoAtual == ESPERANDO_H){
         if(temp == ','){
            estadoAtual = ESPERANDO_POS;
         }else{
            estadoAtual = ESPERANDO_H;
         }
      }else if(estadoAtual == ESPERANDO_POS){
         if(temp == ','){
            pacote.posicao = StringToInt(numeroRecebido, indiceNumeroRecebido - 1);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_TEMPO_POS;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_TEMPO_POS){
         if(temp == ','){
            pacote.tempo_pos = StringToInt(numeroRecebido, indiceNumeroRecebido - 1);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_ANG;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_ANG){
         if(temp == ','){
            pacote.angulo = StringToInt(numeroRecebido, indiceNumeroRecebido - 1);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_TEMPO_ANG;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_TEMPO_ANG){
         if(temp == ','){
            pacote.tempo_ang = StringToInt(numeroRecebido, indiceNumeroRecebido - 1);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_ESTADO_CARRO;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_ESTADO_CARRO){
         if(temp == ','){
            pacote.tempo_ang = StringToInt(numeroRecebido, indiceNumeroRecebido - 1);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_ESTADO_PENDULO;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_ESTADO_PENDULO){
         if(temp == ','){
            pacote.tempo_ang = StringToInt(numeroRecebido, indiceNumeroRecebido - 1);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_BARRA_N;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_BARRA_N){
         if(temp == '\n'){
            recebeuTudo = TRUE;
            estadoAtual = ESPERANDO_H;
         }
      }
   }
   
}

//Assumindo string nnn
int StringToInt(char* string, int size)
{   
   char offset = '0';
   int value = 0;
   
   if(size == 3)
   {
      value += (string[0]-offset)*100;
      value += (string[1]-offset)*10;
      value += (string[2]-offset);
   }
   else if (size == 2)
   {
      value += (string[0]-offset)*10;
      value += (string[1]-offset);
   }
   else if (size == 1)
   {
      value += (string[0]-offset);
   }

   return value;
}



void main ()
{
   enable_interrupts(global);     //habilita a interrupção global
   enable_interrupts(int_rda);    //habilita a interrupção de recepção de caractere pela porta serial

   pacote.posicao = -1;
   pacote.tempo_pos = -1;
   pacote.angulo = -1;
   pacote.tempo_ang = -1;
   pacote.estado_carro = -1;
   pacote.estado_pendulo = -1;
   
   while (true)            //loop de repetição do código principal
   {   
      
      if(recebeuTudo)
      {
         decode();
         
         printf("%d, ", pacote.posicao);
         printf("%d, ", pacote.tempo_pos);
         printf("%d, ", pacote.angulo);
         printf("%d, ", pacote.tempo_ang);
         printf("%d, ", pacote.estado_carro);
         printf("%d\n", pacote.estado_pendulo);

         recebeuTudo = FALSE;
      }
      
   }   // fim do while
}      // fim do main
