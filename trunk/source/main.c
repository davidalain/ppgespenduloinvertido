#include <16F877a.h>
#include <stdio.h>
#use delay (clock = 20000000)
#fuses HS, NOWDT, NOPROTECT, NOPUT, NOBROWNOUT, NOLVP
#use rs232 (baud = 9600, bits = 8, parity = N, uart1, errors) //configurações para a porta serial xmit = pin_c6, rcv = pin_c7

#define BUFFER_SIZE 20

typedef struct packet
{
   long posicao;
   long tempo_pos;
   long angulo;
   long tempo_ang;
   long estado_carro;
   long estado_pendulo;
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

long StringToInt(char* string, int size);
void decode();


BOOLEAN recebeuTudo;
int indexString;
char string_recebida[BUFFER_SIZE];
ReceivePacket pacote;

char numeroRecebido[3];
int indiceNumeroRecebido;
EstadoRecepcao estadoAtual = ESPERANDO_H;
char temp;

#int_rda
void reception ()
{   
   temp = getch();
   
   string_recebida[ indexString++ ] = temp; 
   
   printf(" i=%d c=%c \n", (indexString-1), temp);
  
   if(temp == '\n'){
      recebeuTudo = TRUE;
   }
   
}

//Assumindo string nnn
long StringToInt(char* string, int size)
{   
   char offset = '0';
   int i, index;
   BOOLEAN ehNegativo = FALSE;
   long value = 0;
   
   index = 0;
   
   if(string[0] == '-')
   {
		index += 1;
		size -= 1;
		ehNegativo = TRUE;
   }
   
	if(size == 3)
   {
	  value += (string[index++]-offset)*100;
	  value += (string[index++]-offset)*10;
	  value += (string[index]-offset);
   }
   else if (size == 2)
   {
	  value += (string[index++]-offset)*10;
	  value += (string[index]-offset);
   }
   else if (size == 1)
   {
	  value += (string[index]-offset);
   }
   
   if(ehNegativo)
   {
		value *= -1;
		size++;
   }
   
   printf("string : ");
   for(i=0; i<size; i++)
   {
      printf("%c ", string[i]);
   }
   
  
   printf("\nvalue = %Ld \n", value);
   return value;
}


//O protocolo é: H,<pos_carro>,<tempo_carro>,<angulo>,<tempo_angulo>,<estado_carro>,<estado_pendulo>,\n 

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
            pacote.posicao = StringToInt(numeroRecebido, indiceNumeroRecebido);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_TEMPO_POS;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_TEMPO_POS){
         if(temp == ','){
            pacote.tempo_pos = StringToInt(numeroRecebido, indiceNumeroRecebido);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_ANG;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_ANG){
         if(temp == ','){
            pacote.angulo = StringToInt(numeroRecebido, indiceNumeroRecebido);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_TEMPO_ANG;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_TEMPO_ANG){
         if(temp == ','){
            pacote.tempo_ang = StringToInt(numeroRecebido, indiceNumeroRecebido);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_ESTADO_CARRO;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_ESTADO_CARRO){
         if(temp == ','){
            pacote.estado_carro = StringToInt(numeroRecebido, indiceNumeroRecebido);
            indiceNumeroRecebido = 0;
            estadoAtual = ESPERANDO_ESTADO_PENDULO;
         }else{
            numeroRecebido[ indiceNumeroRecebido++ ] = temp;
         }
      }else if(estadoAtual == ESPERANDO_ESTADO_PENDULO){
         if(temp == ','){
            pacote.estado_pendulo = StringToInt(numeroRecebido, indiceNumeroRecebido);
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
         
         printf("%Ld, ", pacote.posicao);
         printf("%Ld, ", pacote.tempo_pos);
         printf("%Ld, ", pacote.angulo);
         printf("%Ld, ", pacote.tempo_ang);
         printf("%Ld, ", pacote.estado_carro);
         printf("%Ld\n", pacote.estado_pendulo);

         recebeuTudo = FALSE;
       indexString = 0;
      }
      
   }   // fim do while
}      // fim do main
