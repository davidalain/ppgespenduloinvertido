#include <16F877a.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#use delay (clock = 20000000)
#fuses HS, NOWDT, NOPROTECT, NOPUT, NOBROWNOUT, NOLVP
#use rs232 (baud = 9600, bits = 8, parity = N, uart1, errors) //configurações para a porta serial xmit = pin_c6, rcv = pin_c7
// 9600 8N1

#define BUFFER_SIZE 30

typedef struct packet
{

   int16 posicao;
   unsigned int16 tempo_pos;
   signed int16 angulo;
   unsigned int16 tempo_ang;
   int8 estado_carro;
   int8 estado_pendulo;
}ReceivePacket;

typedef enum{
   ESPERANDO_H,
   ESPERANDO_POS,
   ESPERANDO_TEMPO_POS,
   ESPERANDO_ANG,
   ESPERANDO_TEMPO_ANG,
   ESPERANDO_ESTADO_CARRO,
   ESPERANDO_ESTADO_PENDULO
}EstadoRecepcao;

typedef enum{
   ESTADO_ESPERANDO_PACOTE,
   ESTADO_RECEBENDO_PACOTE
}EstadoPacote;

void decode();


BOOLEAN recebeuTudo;
int indexString;
char string_recebida[BUFFER_SIZE];
volatile ReceivePacket pacote;
EstadoPacote estado = ESTADO_ESPERANDO_PACOTE;

char numeroRecebido[5];
int indiceNumeroRecebido;
EstadoRecepcao estadoAtual = ESPERANDO_H;
char temp;

#int_rda
void reception ()
{
   while (kbhit()) {
      temp = getc();

     if(estado == ESTADO_ESPERANDO_PACOTE){
         if(temp == 'H'){
            estado = ESTADO_RECEBENDO_PACOTE;
         }
     }

     if(estado == ESTADO_RECEBENDO_PACOTE){
                // Se o buffer estiver cheio descarta os bytes lidos
         if (indexString < BUFFER_SIZE)
          string_recebida[ indexString++ ] = temp;

         if(temp == '\n'){
            decode();
            indexString = 0;
            recebeuTudo = TRUE;

            estado = ESTADO_ESPERANDO_PACOTE;
         }
     }
  }

}
//O protocolo é: H,<pos_carro>,<tempo_carro>,<angulo>,<tempo_angulo>,<estado_carro>,<estado_pendulo>\n
void decode(){

   char i, temp;




   for(i = 0 ; i < indexString ; i++){
      temp = string_recebida[i];

      if(estadoAtual == ESPERANDO_H){
         if(temp == ','){
            estadoAtual = ESPERANDO_POS;
         }
      }else if(estadoAtual == ESPERANDO_POS){
         if(temp == ','){
         numeroRecebido[ indiceNumeroRecebido++ ] = '\0';
            pacote.posicao = atol(numeroRecebido);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_TEMPO_POS;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_TEMPO_POS){
         if(temp == ','){
            //numeroRecebido[ indiceNumeroRecebido++ ] = '\0';
            //pacote.tempo_pos = atol(numeroRecebido);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_ANG;
         }else{
            //numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_ANG){
         if(temp == ','){
                  numeroRecebido[ indiceNumeroRecebido++ ] = '\0';
            pacote.angulo = atol(numeroRecebido);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_TEMPO_ANG;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_TEMPO_ANG){
         if(temp == ','){
            //numeroRecebido[ indiceNumeroRecebido++ ] = '\0';
            //pacote.tempo_ang = atol(numeroRecebido);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_ESTADO_CARRO;
         }else{
            //numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_ESTADO_CARRO){
         if(temp == ','){
                 numeroRecebido[ indiceNumeroRecebido++ ] = '\0';
            pacote.estado_carro = atol(numeroRecebido);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_ESTADO_PENDULO;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_ESTADO_PENDULO){
         if(temp == '\n'){
            numeroRecebido[ indiceNumeroRecebido++ ] = '\0';
            pacote.estado_pendulo = atol(numeroRecebido);
            indiceNumeroRecebido = 0;
            recebeuTudo = TRUE;
            estadoAtual = ESPERANDO_H;
            indexString = 0;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }
   }

}

signed int16 velocity = 0;
signed int16 previousVelocity = 0;
signed int16  n;
signed int16 alfa;
signed int16 beta;
 int16 direcao;
 
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
        alfa = pacote.angulo % 2400;
        if(pacote.angulo < 0){
         alfa = 2400 - alfa;
        }
         beta = 300;
         
         if( (abs(alfa) < beta) || abs(alfa - 1200) < beta || abs(alfa - 2400) < beta){
            direcao = 0;
         }else if(alfa > 1200 && alfa < 2400){
            direcao = 1;
         }else if(alfa > 0 && alfa < 1200){
            direcao = -1;
         }

      velocity = 140 * direcao;
      if(velocity != previousVelocity){
                  printf("%ld\n", velocity);
                  previousVelocity = velocity;
               }
      recebeuTudo = FALSE;

 //        printf("%ld,", pacote.posicao);
//         printf("%ld,", pacote.tempo_pos);
//         printf("%ld,", pacote.angulo);
//         printf("%ld,", pacote.tempo_ang);
//         printf("%ld,", pacote.estado_carro);
//         printf("%ld\n", pacote.estado_pendulo);
      }

   }   // fim do while

}      // fim do main
